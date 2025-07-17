// Need this define to use operators on ImGui vector types
#define IMGUI_DEFINE_MATH_OPERATORS
#include <imgui.h>
#include <imgui_internal.h>
#include "imgui_utils.hxx"
#include <nfd.h> // Cross-platform native file dialog
#include <cglm/struct.h>

#include <common/int.h>
#include <common/vfile.h>

#include "formats/alr.h"
#include "alr_texture.hxx"
#include "editor_alr.hxx"
#include "polaris.hxx"
#include "scope_timer.hxx"

bool polaris::validate(std::string& output) const noexcept {
    const scope_timer draw_timer(timer_map, "polaris_validate");

    bool result = true;
    std::optional<al::resource::chunk> header_chunk;
    for (const al::resource::chunk& chunk : this->alr.chunks) {
        result &= chunk.validate(this->alr, output, headless);
        if (chunk.id == 0x11) {
            header_chunk = chunk;
        }
    }

    // We expect that the header chunk is always present
    if (!header_chunk.has_value()) {
        str_format_append(output, "ALR header is missing!\n");
        return false;
    }

    if (header_chunk->offset != 0) {
        str_format_append(output, "Expected header @ offset 0 [found @ 0x%x]!\n", header_chunk->offset);
        result = false;
    }

    // Get vfile for header offsets
    vfile alr_vf = vfile_open(this->alr.data, this->alr.alr_size);
    vfile header_vf = alr_vf;
    const auto header = VFILE_READ(chunk_layout, &header_vf);
    const s32* offsets = (s32*)vfile_cur(header_vf);

    const s64 size_mismatch = alr.alr_size - (header.texbuf_offset + header.texbuf_size);
    if (size_mismatch < 0) {
        str_format_append(output,
                          "Header claims resbuf is 0x%X bytes @ 0x%X, but ALR is only 0x%X bytes (off by 0x%X)\n",
                          header.texbuf_size, header.texbuf_offset, alr.alr_size, abs(size_mismatch));
        result = false;
    } else if (size_mismatch > 0) {
        str_format_append(output,
                          "Header claims resbuf is 0x%X bytes @ 0x%X, leaving 0x%X bytes extra\n",
                          header.texbuf_size, header.texbuf_offset, size_mismatch);
        result = false;
    }

    s32 prev_offset = offsets[0];
    u32 chunk_idx = 0;

    for (u32 i = 1; i < header.offset_array_size; i++) {
        const s32 cur_offset = offsets[i];
        if (cur_offset < 0) {
            str_format_append(output, "Header offset #%d is negative [%d]!\n", i, cur_offset);
            continue;
        }

        // The first 0x00 chunk we find after the previous offset.
        std::optional<al::resource::chunk> terminator;
        // The last chunk before we hit the current offset
        al::resource::chunk last(0xFF, 0, 0);

        // Skip up to the last chunk before the current offset
        while (chunk_idx < alr.chunks.size()) {
            const al::resource::chunk chunk = alr.chunks.at(chunk_idx);

            // Save the first 0x00 chunk we find
            if (chunk.id == 0x00 && !terminator.has_value()) {
                terminator = chunk;
            }

            // We've hit the current offset
            if (chunk.offset >= cur_offset) {
                break;
            }

            // If we got here, this chunk is still before the current offset.
            last = chunk;
            chunk_idx++;
        }

        // Unless our assumptions break or the file is wrong, the terminator
        // should always be the last chunk.
        if (!terminator.has_value()) {
            str_format_append(output, "Chunk series @ offset 0x%x missing a null terminator!\n", cur_offset);
            result = false;
        }
        else if (last.offset != terminator->offset) {
            str_format_append(output, "Chunk series @ offset 0x%x has terminator @ 0x%x, but last chunk @ 0x%x!\n", cur_offset, terminator->offset, last.offset);
            result = false;
        }

        // Update previous offset
        prev_offset = cur_offset;
    }

    // Verify that the offset table is in order (aside from negative entries)
    s32 temp = -1;
    for (s32 i = 0; i < header.offset_array_size; i++) {
        const s32 offset = offsets[i];
        if (offset <= temp) {
            str_format_append(output, "Offset %d [0x%x] <= offset %d [0x%x]\n", i, offset, i - 1, temp);
            result = false;
        }
        temp = offset;
    }

    return result;
}

polaris::polaris() noexcept {
    // TODO: Add an option to commit on reserve in bobtail
    // TODO: Look into MEM_RESET to reduce impact on page file?
}

void polaris::handle_input_suppression() noexcept {
    if (ImGui::GetIO().WantCaptureMouse) {
        // ImGui wants control of the mouse (it's probably over a window),
        // so we'll suppress the real mouse state this frame.
        input.cursor = prev_input.cursor;
        input.scroll = prev_input.scroll;
        input.click_left = prev_input.click_left;
        input.click_right = prev_input.click_right;
        input.click_middle = prev_input.click_middle;
        input.mouse_button_4 = prev_input.mouse_button_4;
        input.mouse_button_5 = prev_input.mouse_button_5;
    }

    if (ImGui::GetIO().WantCaptureKeyboard) {
        // Save non-keyboard input
        const vec2s cursor = input.cursor;
        const vec2s scroll = input.scroll;
        const bool click_left = input.click_left;
        const bool click_right = input.click_right;
        const bool click_middle = input.click_middle;
        const bool mouse_4 = input.mouse_button_4;
        const bool mouse_5 = input.mouse_button_5;

        const vec2s LS = input.LS;
        const vec2s RS = input.RS;
        const float LT = input.LT;
        const float RT = input.RT;
        const gamepad_t gp = input.gp;

        // Copy over all keyboard input
        input = prev_input;

        // Restore non-keyboard input
        input.cursor = cursor;
        input.scroll = scroll;
        input.LS = LS;
        input.RS = RS;
        input.LT = LT;
        input.RT = RT;
        input.gp = gp;
        input.click_left = click_left;
        input.click_right = click_right;
        input.click_middle = click_middle;
        input.mouse_button_4 = mouse_4;
        input.mouse_button_5 = mouse_5;
    }
}

void polaris::unload_gl_textures() noexcept {
    if (headless) {
        return;
    }
    glDeleteTextures(gl_textures.size(), gl_textures.data());
    gl_textures.clear();
}

void polaris::do_menu_bar() noexcept {
    ImGuiViewport* viewport = ImGui::GetMainViewport();
    const float height = ImGui::GetFrameHeight();
    const ImGuiWindowFlags flags = ImGuiWindowFlags_NoScrollbar | ImGuiWindowFlags_MenuBar;

    const bool ctrl_pressed = ImGui::IsKeyDown(ImGuiKey_LeftCtrl) || ImGui::IsKeyDown(ImGuiKey_RightCtrl);
    bool load_alr = ctrl_pressed && ImGui::IsKeyPressed(ImGuiKey_L, false);
    bool save_alr = ctrl_pressed && ImGui::IsKeyPressed(ImGuiKey_S, false);

    bool load_layout = false;

    if (ImGui::BeginViewportSideBar("MainMenu", viewport, ImGuiDir_Up, height, flags)) {
        if (ImGui::BeginMenuBar()) {
            if (ImGui::BeginMenu("File")) {
                load_alr |= ImGui::MenuItem("Load ALR", "Ctrl-L");
                save_alr |= ImGui::MenuItem("Save ALR", "Ctrl-S");
                load_layout |= ImGui::MenuItem("Load .dat");
                ImGui::EndMenu();
            }

            if (ImGui::BeginMenu("View")) {
                ImGuiIO& io = ImGui::GetIO();
                ImGui::InputFloat("Font Size", &io.FontGlobalScale, 0.1f);
                ImGui::EndMenu();
            }

            if (ImGui::BeginMenu("Windows")) {
                ImGui::MenuItem("Viewport", nullptr, &this->viewport.enabled);
                ImGui::MenuItem("Viewport Editor", nullptr, &this->viewport.editor_enabled);
                ImGui::MenuItem("Performance Timers", nullptr, &this->show_timers);
                ImGui::MenuItem("ImGui Demo Window", nullptr, &this->show_demo);
                ImGui::EndMenu();
            }

            ImGui::EndMenuBar();
        }
        ImGui::End();
    }

    if (load_alr) {
        // Display the file picker and load the ALR if a file is picked
        char* path = nullptr;
        const nfdu8filteritem_t filters[] = { { "AL Resource", "alr"} };
        nfdresult_t result = NFD_OpenDialogU8(&path, filters, ARRAY_SIZE(filters), nullptr);
        if (result == NFD_OKAY && path != nullptr) {
            this->alr.load(path);
        }
        free(path);
    }

    if (save_alr) {
        // Display the file picker and save the ALR if a file is picked
        nfdu8filteritem_t filters[] = { { "AL Resource", "alr"} };
        char* path = nullptr;
        nfdresult_t result = NFD_SaveDialogU8(&path, filters, ARRAY_SIZE(filters), nullptr, nullptr);
        if (result == NFD_OKAY && path != nullptr) {
            this->alr.save(path);
        }
        free(path);
    }

    if (load_layout) {
        // Display the file picker and load if a file is picked
        nfdu8filteritem_t filters[] = { { "AL Layout", "dat"} };
        char* path = nullptr;
        nfdresult_t result = NFD_OpenDialogU8(&path, filters, ARRAY_SIZE(filters), nullptr);
        if (result == NFD_OKAY && path != nullptr) {
            this->map = mapdata(path);
        }
        free(path);
    }
}

bool load_gl_textures(polaris* pol) {
    assert(!pol->headless && "Can't load textures in headless mode!");
    assert(pol->alr.resbuf_offset != 0 && "Can't load textures without resbuf offset!");

    al::resource::chunk texture_chunk(0, 0, 0);
    al::resource::chunk atlas_chunk(0, 0, 0);

    // Try to find texture and texture atlas metadata, we need both to make a
    // good guess about dimensions.
    for (al::resource::chunk chunk : pol->alr.chunks) {
        if (chunk.id == 0x15) {
            texture_chunk = chunk;
        }
        if (chunk.id == 0x10) {
            atlas_chunk = chunk;
        }
    }

    // Read texture chunk data
    vfile vf = vfile_open(pol->alr.data + texture_chunk.offset, texture_chunk.size);
    // Skip over the ID and size fields we already have
    vfile_seek(&vf, sizeof(chunk_generic));
    const u32 num_entries = VFILE_READ(u32, &vf);
    texture_entry* tex_entries = (texture_entry*)vfile_cur(vf);

    // Save the offset of the newly loaded ALR's "texture #0".
    pol->alr.cur_alr_texture_0 = MAX((s32)pol->gl_textures.size(), 0);

    // Read atlas chunk data
    atlas_entry* atlas_entries = nullptr;
    atlas_name* atlas_names = nullptr;
    atlas_header header_atlas = {0};
    if (atlas_chunk.size > 0) {
        vf = vfile_open(pol->alr.data + atlas_chunk.offset, atlas_chunk.size);

        // Skip over the ID and size fields we already have
        vfile_seek(&vf, sizeof(chunk_generic));
        header_atlas = VFILE_READ(atlas_header, &vf);

        // Skip over names
        atlas_names = (atlas_name*)vfile_cur(vf);
        vfile_seek(&vf, sizeof(atlas_name) * header_atlas.atlas_count);

        atlas_entries = (atlas_entry*)vfile_cur(vf);
    }

    bool result = true;
    for (u32 i = 0; i < num_entries; i++) {
        // Convert the ALR texture data to our standard texture struct
        texture cur_tex = convert_tex(pol->alr.resource_buffer(), tex_entries[i]);

        if (atlas_entries != nullptr && header_atlas.atlas_count > i) {
            atlas_entry entry = atlas_entries[i];
            // We get better dimension info from the atlas headers, use it!
            // Dimensions from the atlas headers are almost always more
            // accurate, so we always use them unless they're obviously wrong.

            const u32 too_small = 0;
            const u32 too_big = 8192;
            if (entry.width > too_small && entry.width < too_big) {
                cur_tex.width = entry.width;
            }
            if (entry.height > too_small && entry.height < too_big) {
                cur_tex.height = entry.height;
            }
        }

        gl_obj gl_tex_id = 0;
        glGenTextures(1, &gl_tex_id);
        update_gl_tex(cur_tex, gl_tex_id);

        result &= (gl_tex_id != 0);
        pol->gl_textures.push_back(gl_tex_id);
    }

    return result;
}

void polaris::do_gui(GLFWwindow* window) noexcept {
    const scope_timer draw_timer(timer_map, "main_draw");

    // Make the entire window a giant docking space
    ImGui::DockSpaceOverViewport();

    if (!headless && alr.textures_need_reload) {
        load_gl_textures(this);
        alr.textures_need_reload = false;
    }

    // We have to wait until we know the graphics context has been created to do
    // graphics-related initialization (since the program may run in headless
    // mode with no graphics context).
    if (!viewport.initialized) {
        // Have the viewport render in full resolution, it'll be downscale when
        // rendered as a texture by ImGui::Image
        int width = 0;
        int height = 0;
        glfwGetFramebufferSize(window, &width, &height);
        viewport.setup(width, height);
    } else {
        // TODO: Is there a good way to get a const& to ourselves?
        if (!viewport.render_contents(window, this)) {
            // We don't want to supress input if the viewport needs it
            this->handle_input_suppression();
        }
    }

    this->do_menu_bar();

    if (this->show_demo) {
        ImGui::ShowDemoWindow(&this->show_demo);
    }

    if (this->show_timers) {
        ImGui::Begin("Performance Timers", &show_timers);
        for (std::pair<const char*, double> entry : timer_map) {
            ImGui::Text("%s: %.2lfms", entry.first, entry.second * 1000);
        }
        ImGui::End();
    }

    this->alr.draw(viewport);
    this->map.do_gui();

    // It's the end of the frame for us, save the current input
    prev_input = input;
}
