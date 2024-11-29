#include <cstdlib>
#include <string>

#include <glad/glad.h>
#include <ImGuiFileDialog.h>
#include "alr_texture.hxx"
#include "polaris.hxx"

extern "C" {
    #include <cglm/struct.h>
    #include <common/vfile.h>
    #include <common/file.h>
    #include <common/vmem.h>
    #include <common/logging.h>
    #include <formats/alr.h>
    #include <formats/pd_common.h>
}

// Minor helper functions for ImGui
namespace ImGui {
    void BeginChildFitContent(const char* id, float width_percent) {
        ImGui::BeginChild(id, ImVec2(ImGui::GetContentRegionAvail().x * width_percent, 260), ImGuiChildFlags_ResizeX | ImGuiChildFlags_ResizeY);
    }
}

void polaris::chunk::dump_idx_buf(polaris *pol, FILE* out) const {
    vfile vf = vfile_open(pol->alr_data + offset, size);

    // Skip over chunk header
    vfile_seek(&vf, sizeof(chunk_generic));
    const idx_buf_header header = VFILE_READ(idx_buf_header, &vf);

    const s32 num_tris = (size - vf.pos) / (3 * sizeof(u16));
    for (u32 i = 0; i < num_tris - 1; i++) {
        u16 idx1 = VFILE_READ(u16, &vf);
        u16 idx2 = VFILE_READ(u16, &vf);
        u16 idx3 = VFILE_READ(u16, &vf);

        // OBJ indices start at 1 :(
        idx1++;
        idx2++;
        idx3++;

        fprintf(out, "f %d %d %d\n", idx1, idx2, idx3);
    }

}

void polaris::chunk::chunk_0x2(polaris *pol) {
    char dialog_key[0x20] = {0};
    snprintf(dialog_key, sizeof(dialog_key), "chooseOBJ_idx##%lu", offset);
    if (ImGui::Button("Append indices to OBJ")) {
        // All we can do this frame is open the dialog
        ImGuiFileDialog::Instance()->OpenDialog(dialog_key, "Choose OBJ File", ".obj", {});
    }

    // Display file dialog if appropriate
    if (ImGuiFileDialog::Instance()->Display(dialog_key)) {
        // If user cancels, we can't load anything
        if (ImGuiFileDialog::Instance()->IsOk()) {
            const std::string path = ImGuiFileDialog::Instance()->GetFilePathName();

            // Dump to OBJ
            FILE* out = fopen(path.c_str(), "ab");
            if (out == nullptr) {
                return;
            }

            this->dump_idx_buf(pol, out);
            fclose(out);
        }

        // Close the dialog
        ImGuiFileDialog::Instance()->Close();
    }

    // Index buffer hex editor
    u8* ptr = pol->alr_data + offset + sizeof(chunk_generic);
    hex_edit.DrawContents(ptr, size - sizeof(chunk_generic));
}

void polaris::chunk::chunk_0x3(polaris *pol) {
    if (id != 0x3) {
        return;
    }

    vfile vf = vfile_open(pol->alr_data + offset, size);
    vfile_seek(&vf, sizeof(chunk_generic)); // Skip ID & size

    const u32 num_matrices = (size - sizeof(chunk_generic) - sizeof(chunk_transform)) / sizeof(mat4);
    const u16 num_non_identity = VFILE_READ(u16, &vf);
    const u16 unk = VFILE_READ(u16, &vf);
    const u32 pad = VFILE_READ(u32, &vf);
    auto *matrices = (mat4 *) vfile_cur(vf);

    const u32 min = 0;
    const u32 max = MAX(num_matrices - 1, 0);
    ImGui::Checkbox("Use slider", &window_0x3.mat_slider);
    if (window_0x3.mat_slider) {
        ImGui::SliderScalar("Selected Matrix", ImGuiDataType_S32, &window_0x3.selected_mat, &min, &max);
    } else {
        ImGui::InputScalar("Selected Matrix", ImGuiDataType_S32, &window_0x3.selected_mat);
    }
    // Don't allow out of bounds index
    window_0x3.selected_mat = CLAMP(min, window_0x3.selected_mat, max);

    ImGui::Text("%d matrices [%d identity]", num_matrices, num_matrices - num_non_identity);

    if (ImGui::BeginTabBar("Matrix Editing")) {
        if (ImGui::BeginTabItem("Raw editor")) {

            // Matrix inputs
            ImGui::PushItemWidth(200.0f); // Make inputs narrower
            ImGui::Text("Matrix Editor");
            for (u32 j = 0; j < 4; j++) {
                for (u32 k = 0; k < 4; k++) {
                    char buf[0x20] = {0};
                    snprintf(buf, sizeof(buf), "##%d%d", j, k);
                    ImGui::InputFloat(buf, &matrices[window_0x3.selected_mat][j][k]);
                    ImGui::SameLine();
                }
                ImGui::Text(" "); // Cause a new line
            }
            ImGui::PopItemWidth();
            ImGui::EndTabItem();
        }

        if (ImGui::BeginTabItem("Hex Editing")) {
            // Show hex editor
            hex_edit.DrawContents(&matrices[window_0x3.selected_mat], sizeof(matrices[window_0x3.selected_mat]));

            ImGui::EndTabItem();
        }

        ImGui::EndTabBar();
    }
}

void polaris::chunk::chunk_0x10(polaris *pol) {
    if (id != 0x10) {
        // Exit if we were called by mistake
        return;
    }

    // We use the vfile API to handle the chunk data
    vfile vf = vfile_open(pol->alr_data + offset, size);
    // Skip over the ID and size fields we already have
    vfile_seek(&vf, sizeof(chunk_generic));

    // We use pointers instead of reading into stack copies, so we can edit the
    // data directly. I'm not usually a big fan of using auto, but it doesn't
    // hide the real data type so I think it's fine here.
    auto* header = (texture_metadata_header*) vfile_cur(vf);
    vfile_seek(&vf, sizeof(*header));

    // Read surface names
    auto* atlas_names = (atlas_name*) vfile_cur(vf);
    vfile_seek(&vf, sizeof(*atlas_names) * header->atlas_count);

    // Read surface metadata
    auto* atlases = (atlas_info*) vfile_cur(vf);
    vfile_seek(&vf, sizeof(*atlases) * header->atlas_count);

    // Read texture metadata
    auto* textures = (tex_info *) vfile_cur(vf);
    vfile_seek(&vf, sizeof(*textures) * header->texture_count);

    ImGui::BeginChildFitContent("Atlases", 0.3f);
    ImGui::Text("%d Atlases for %.*s:", header->atlas_count, (int)sizeof(header->alr_name), header->alr_name);
    for (u32 i = 0; i < header->atlas_count; i++) {
        char buf[sizeof(atlas_names[i].name) + 0x20] = {0};
        snprintf(buf, sizeof(buf) - 1, "%s", atlas_names[i].name);

        if (ImGui::Selectable(buf, window_0x10.selected_atlas == i)) {
            window_0x10.selected_atlas = i;
        }
    }
    ImGui::EndChild();
    ImGui::SameLine();


    // The currently selected texture might be in a different atlas, which would
    // display a strange & unintuitive result. We find the first and last index
    // of textures in the atlas, and make sure the selected texture is always
    // a child of the selected atlas.
    u32 first_idx_in_atlas = 0;
    u32 last_idx_in_atlas = 0;
    for (u32 i = 0; i < header->texture_count; i++) {
        if (textures[i].index != window_0x10.selected_atlas) {
            continue;
        }

        if (first_idx_in_atlas == 0) {
            first_idx_in_atlas = i;
        } else {
            last_idx_in_atlas = i;
        }
    }
    if (textures[window_0x10.selected_atlas_texture].index != window_0x10.selected_atlas) {
        // The selected texture doesn't belong to the current atlas, so select
        // the first one that does
        window_0x10.selected_atlas_texture = first_idx_in_atlas;
    }

    // Display textures in the selected atlases
    ImGui::BeginChildFitContent("Textures", 0.3f);
    for (u32 i = 0; i < header->texture_count; i++) {
        const tex_info tex = textures[i];
        // Only list textures belonging to the selected atlases
        if (tex.index != window_0x10.selected_atlas) {
            continue;
        }

        char buf[sizeof(textures[i].filename) + 0x20] = {0};
        snprintf(buf, sizeof(buf) - 1, "%s", tex.filename);

        if (ImGui::Selectable(buf, window_0x10.selected_atlas_texture == i)) {
            window_0x10.selected_atlas_texture = i;
            // Once we have a mechanism to find the atlases' position in the
            // texture buffer, clicking on a texture should set it as the
            // active texture and display it.
        }
    }
    ImGui::EndChild();
    // ImGui::SameLine();

    ImGui::BeginChild("texInfo");
    const atlas_name aName = atlas_names[window_0x10.selected_atlas];
    const atlas_info atlas = atlases[window_0x10.selected_atlas];
    ImGui::Text("\nAtlas info for \"%.*s\":", (int)sizeof(aName.name), aName.name);
    ImGui::Text("%dx%d pixels, contains %d texture(s)", atlas.height, atlas.width, atlas.mipmap_count);
    ImGui::Text("Texture index %d (see 0x15 chunk for offset)", window_0x10.selected_atlas);

    const tex_info tex = textures[window_0x10.selected_atlas_texture];
    ImGui::Text("\nTexture info for \"%.*s\":", (int)sizeof(tex.filename), tex.filename);
    ImGui::Text("%dx%d pixels, UV coords (%.3f, %.3f)", tex.width, tex.height, tex.atlas_texcoords[0], tex.atlas_texcoords[1]);
    ImGui::EndChild();
}

void polaris::chunk::chunk_0x11(polaris *pol) {
    if (id != 0x11) {
        return;
    }

    vfile vf = vfile_open(pol->alr_data + offset, size);
    auto* layout = (chunk_layout*)vfile_cur(vf);
    vfile_seek(&vf, sizeof(*layout));
    auto* offsets = (u32*)vfile_cur(vf);

    ImGui::Text("Texture buffer @ 0x%X [%d bytes]", layout->texbuf_offset, layout->texbuf_size);
    ImGui::Text("%d offsets in array:\n", layout->offset_array_size);

    if (ImGui::BeginListBox("Offsets")) {
        for (u32 i = 0; i < layout->offset_array_size; i++) {
            ImGui::Text("0x%X", offsets[i]);
        }
        ImGui::EndListBox();
    }
}

void polaris::chunk::import_dds_0x15(polaris* pol, const char* path, u32 num_entries, resource_entry* entries) {
    if (id != 0x15) {
        return;
    }

    const resource_entry cur = entries[window_0x15.selected_texture];
    const resource_entry next = entries[window_0x15.selected_texture + 1];
    s64 tex_size = 0;
    if (window_0x15.selected_texture >= num_entries) {
        // This is the last entry, so the best guess is that it takes up the
        // rest of the file
        tex_size = pol->alr_size - cur.data_ptr;
    } else {
        // The most likely texture size is the distance betwen this texture and
        // the next
        tex_size = next.data_ptr - cur.data_ptr;
    }

    window_0x15.tex = image_buf_load(path, window_0x15.tex.data, tex_size);
}

void polaris::chunk::chunk_0x15(polaris *pol) {
    if (id != 0x15) {
        // Exit if we were called by mistake
        return;
    }

    // We use the vfile API to handle the chunk data
    vfile vf = vfile_open(pol->alr_data + offset, size);
    // Skip over the ID and tex_size fields we already have (both 32-bit)
    vfile_seek(&vf, sizeof(chunk_generic));

    const u32 num_entries = VFILE_READ(u32, &vf);
    auto* entries = (resource_entry*)vfile_cur(vf);

    ImGui::BeginChildFitContent("Textures", 0.3f);
    for (u32 i = 0; i < num_entries; i++) {
        char buf[0x30] = {0};
        char name[PD_ENCODED_CHAR_COUNT + 1] = {0};
        decode_single32(name, entries[i].text1);
        decode_single32(&name[6], entries[i].text2);

        snprintf(buf, sizeof(buf) - 1, "#%d \"%s\" @ resbuf+0x%X", i, name, entries[i].data_ptr);

        if (ImGui::Selectable(buf, window_0x15.selected_texture == i)) {
            window_0x15.selected_texture = i;
        }
    }
    ImGui::EndChild();
    ImGui::SameLine();

    ImGui::BeginChildFitContent("Texture Details", 1.0f);
    resource_entry* entry = &entries[window_0x15.selected_texture];
    char name[PD_ENCODED_CHAR_COUNT + 1] = {0};
    decode_single32(name, entry->text1);
    decode_single32(&name[6], entry->text2);

    texture cur_tex = convert_tex(pol->alr_data + pol->resbuf_offset, *entry);
    ImGui::Text("Warning: These pixel counts are guesses.\nIf they look wrong, trust your own judgement\nand the 0x10 (texture atlas) window.\n\n");
    ImGui::Text("\"%s\" is %dx%d pixels @ resbuf+0x%X\n", name, cur_tex.height, cur_tex.width, entry->data_ptr);

    const char* format = texformat_str((alr_pixel_format)entry->pixel_format);
    ImGui::Text("Suspected format: %s (code 0x%X)", format, entry->pixel_format);

    if (window_0x15.gl_tex_id == 0) {
        // Create & upload initial texture state
        glGenTextures(1, &window_0x15.gl_tex_id);
        if (window_0x15.gl_tex_id == 0) {
            LOG_MSG(error, "Failed to create OpenGL texture for \"%s\"\n", name);
        }

        // Wrapping & filtering settings
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_BORDER);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_BORDER);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST_MIPMAP_NEAREST);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);

        update_gl_tex(cur_tex, window_0x15.gl_tex_id);
        window_0x15.tex = cur_tex;
        window_0x15.view_width = window_0x15.view_height = 512;
    }
    else if (memcmp(&cur_tex, &window_0x15.tex, sizeof(cur_tex)) != 0) {
        // The texture changed since last frame, update the OpenGL state
        update_gl_tex(cur_tex, window_0x15.gl_tex_id);
        window_0x15.tex = cur_tex;
    }

    ImGui::Text("2^(resolution power) = width = height");
    const u32 step_pwr = 1; // Step for the resolution power input
    ImGui::InputScalar("Resolution power", ImGuiDataType_U16, &entry->resolution_pwr, &step_pwr);
    // This limits resolution to 4096^2, which is plenty for our use case
    entry->resolution_pwr = MIN(entry->resolution_pwr, 12);

    if (ImGui::Button("Import DDS")) {
        ImGuiFileDialog::Instance()->OpenDialog("chooseDDS_import0x15", "Choose DDS File", ".dds", {});
    }
    ImGui::SameLine();
    if (ImGui::Button("Export DDS")) {
        // All we can do this frame is open the dialog
        ImGuiFileDialog::Instance()->OpenDialog("chooseDDS_export0x15", "Choose DDS File", ".dds", {});
    }
    // Display file dialog if appropriate
    if (ImGuiFileDialog::Instance()->Display("chooseDDS_export0x15")) {
        if (ImGuiFileDialog::Instance()->IsOk()) {
            const std::string path = ImGuiFileDialog::Instance()->GetFilePathName();
            img_write(window_0x15.tex, path.c_str());
        }
        ImGuiFileDialog::Instance()->Close();
    }

    if (ImGuiFileDialog::Instance()->Display("chooseDDS_import0x15")) {
        if (ImGuiFileDialog::Instance()->IsOk()) {
            const std::string path = ImGuiFileDialog::Instance()->GetFilePathName();
            this->import_dds_0x15(pol, path.c_str(), num_entries, entries);
        }
        ImGuiFileDialog::Instance()->Close();
    }


    ImGui::Text("\nRendering settings (doesn't affect ALR data):");
    ImGui::Checkbox("Render @ actual size", &window_0x15.use_actual_size);
    const u32 step = 16;
    ImGui::InputScalar("Visual Width", ImGuiDataType_U16, &window_0x15.view_width, &step);
    ImGui::InputScalar("Visual Height", ImGuiDataType_U16, &window_0x15.view_height, &step);

    // At this point we're pretty sure the texture is correctly formatted,
    // so we can render it.
    ImVec2 view_size = ImVec2(window_0x15.view_width, window_0x15.view_height);
    if (window_0x15.use_actual_size) {
        view_size = ImVec2(window_0x15.tex.width, window_0x15.tex.height);
    }
    // TODO: Look into showing mipmap contents
    // TODO: See if we can scale the image relative to window size
    ImGui::Image(window_0x15.gl_tex_id, view_size);
    ImGui::EndChild();
}

void polaris::chunk::chunk_0x16(polaris *pol) {
    if (id != 0x16) {
        // Exit if we were called by mistake
        return;
    }

    // We use the vfile API to handle the chunk data
    vfile vf = vfile_open(pol->alr_data + offset, size);
    // Skip over the ID and size fields we already have (both 32-bit)
    vfile_seek(&vf, sizeof(chunk_generic));

    const u32 num_entries = VFILE_READ(u32, &vf);
    auto* entries = (resource_entry_0x16*)vfile_cur(vf);

    ImGui::BeginChild("Vertex Buffers", ImVec2(300, 0));
    for (u32 i = 0; i < num_entries; i++) {
        char buf[0x30] = {0};
        snprintf(buf, sizeof(buf) - 1, "0x%X verts @ 0x%X", entries[i].vertex_count, entries[i].data_ptr);

        const bool is_selected = window_0x16.selected_vertex_buf == i;
        if (ImGui::Selectable(buf, is_selected)) {
            window_0x16.selected_vertex_buf = i;
        }
    }
    ImGui::EndChild();
    ImGui::SameLine();

    ImGui::BeginChild("Vertex Buffer Settings", ImVec2(600, 0));
    if (ImGui::Button("Dump to OBJ")) {
        // All we can do this frame is open the dialog
        ImGuiFileDialog::Instance()->OpenDialog("chooseOBJ", "Choose OBJ File", ".obj", {});
    }

    hex_edit.DrawContents(&entries[window_0x16.selected_vertex_buf], sizeof(*entries));

    // Display file dialog if appropriate
    if (ImGuiFileDialog::Instance()->Display("chooseOBJ")) {
        // If user cancels, we can't load anything
        if (ImGuiFileDialog::Instance()->IsOk()) {
            const std::string path = ImGuiFileDialog::Instance()->GetFilePathName();

            // Dump to OBJ
            resource_entry_0x16 entry = entries[window_0x16.selected_vertex_buf];
            FILE *out = fopen(path.c_str(), "wb");
            if (out != nullptr) {
                vf = vfile_open(pol->alr_data, pol->alr_size);

                // Jump to resource buffer
                vfile_seek(&vf, pol->resbuf_offset);
                for (u32 i = 0; i < entry.vertex_count; i++) {
                    // Read our vertex positions, which always come first
                    const vec3s vert = VFILE_READ(vec3s, &vf);
                    fprintf(out, "v %f %f %f\n", vert.x, vert.y, vert.z);

                    // There might be some data left over, for now we just skip
                    // over it.
                    const u32 skip = entry.vertex_size - (sizeof(vert));
                    vfile_seek(&vf, skip);
                }

                // Vertices are dumped, now for indices
                for (chunk idx_chunk : pol->chunks) {
                    if (idx_chunk.id != 0x2) {
                        // We only want index buffer chunks
                        continue;
                    }

                    if (idx_chunk.offset < offset) {
                        continue;
                    }

                    // Skip to idx_chunk and skip header
                    vf.pos = idx_chunk.offset;
                    vfile_seek(&vf, sizeof(chunk_generic));
                    const idx_buf_header header = VFILE_READ(idx_buf_header, &vf);

                    // We only want index buffers meant for this vertex buffer
                    if (header.vertex_buf != window_0x16.selected_vertex_buf) {
                        continue;
                    }

                    this->dump_idx_buf(pol, out);
                }

                // Cleanup
                fclose(out);
            }
        }

        // Close the dialog
        ImGuiFileDialog::Instance()->Close();
    }

    ImGui::EndChild();
}

void polaris::chunk::draw(polaris *pol) {
    if (pol->alr_data == nullptr || pol->alr_size == 0) {
        // There's no data to work on, we can't display any useful data.
        return;
    }

    switch (id) {
        case 0x2:
            this->chunk_0x2(pol);
            break;
        case 0x3:
            this->chunk_0x3(pol);
            break;
        case 0x10:
            this->chunk_0x10(pol);
            break;
        case 0x11:
            this->chunk_0x11(pol);
            break;
        case 0x15:
            this->chunk_0x15(pol);
            break;
        case 0x16:
            this->chunk_0x16(pol);
            break;
        default:
            // Unimplemented window
            ImGui::Text("Unimplemented chunk type");
            return;
    }
}

polaris::chunk::chunk(u32 init_id) {
    id = init_id;

    switch (id) {
        case 0x3:
            window_0x3 = {};
            break;
        case 0x10:
            window_0x10 = {};
            break;
        case 0x15:
            window_0x15 = {};
            break;
        case 0x16:
            window_0x16 = {};
            break;
        default:
            return;
    }
}

polaris::polaris() noexcept {
    // TODO: Add an option to commit on reserve in bobtail
    // TODO: Look into MEM_RESET to reduce impact on page file?

    // "Expand" our reservation from 0 bytes to... not 0.
    this->expand_reservation(reserve_size);
}

void polaris::expand_reservation(s64 new_size) noexcept {
    if (new_size < reserve_size) {
        LOG_MSG(error, "No reason to shrink reservation from 0x%X -> 0x%X, ignoring!\n", reserve_size, new_size);
        return;
    }

    u8* new_buf = (u8*)vmem_reserve(new_size);
    if (new_buf == nullptr) {
        return;
    }

    // Commit the entire region. This ensures it's all accessible, but doesn't
    // comsume any physical memory until accessed. (May still create page file
    // entries)
    vmem_commit(new_buf, new_size);

    // Free old buffer and update our state
    if (alr_data != nullptr) {
        vmem_free(alr_data, reserve_size);
    }
    alr_data = new_buf;
    reserve_size = new_size;
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

bool polaris::save_alr(const char* path) const noexcept {
    FILE* out = fopen(path, "wb");
    if (out == nullptr) {
        return false;
    }

    bool result = true;
    if (fwrite(alr_data, alr_size, 1, out) != 1) {
        // Incomplete write
        result = false;
    }
    fclose(out);

    return result;
}

bool polaris::do_gui(GLFWwindow* window) noexcept {
    this->handle_input_suppression();

    ImGui::Begin("ALR Select");
    if (ImGui::Button("Load ALR")) {
        ImGuiFileDialog::Instance()->OpenDialog("chooseALR", "Choose ALR File", ".alr", {});
    }

    if (ImGui::Button("Save ALR")) {
        ImGuiFileDialog::Instance()->OpenDialog("chooseALRSave", "Choose ALR File", ".alr", {});
    }

    // Display file dialog if appropriate
    if (ImGuiFileDialog::Instance()->Display("chooseALR")) {
        // If user cancels, we can't load anything
        if (ImGuiFileDialog::Instance()->IsOk()) {
            const std::string path_str = ImGuiFileDialog::Instance()->GetFilePathName();
            const char* path = path_str.c_str();
            const s64 size = file_size(path);
            if (file_exists(path) || size > 8) {
                // Expand reservation if needed
                if (size > reserve_size) {
                    // If our reservation needs resizing, we're dealing with a
                    // truly massive file. Just add its size to the old size,
                    // more space can never hurt.
                    this->expand_reservation(reserve_size + size);
                }

                // Load the file into the buffer.
                file_load_existing(path, alr_data, size);
                alr_size = size;
                chunks = shatter_alr(alr_data, alr_size);
            }
        }

        // Close the dialog
        ImGuiFileDialog::Instance()->Close();
    }

    // Display file dialog if appropriate
    if (ImGuiFileDialog::Instance()->Display("chooseALRSave")) {
        // If user cancels, we can't load anything
        if (ImGuiFileDialog::Instance()->IsOk()) {
            const std::string path = ImGuiFileDialog::Instance()->GetFilePathName();

            this->save_alr(path.c_str());
        }

        // Close the dialog
        ImGuiFileDialog::Instance()->Close();
    }

    if (ImGui::BeginListBox(" ", ImVec2(0, -FLT_MIN))) {
        for (size_t n = 0; n < chunks.size(); n++) {
            polaris::chunk& chunk = chunks.at(n);
            char buf[128] = {0};
            snprintf(buf, sizeof(buf), "0x%02X chunk @ 0x%02lX [%d bytes] ##%lu", chunk.id, chunk.offset, chunk.size, n);

            if (ImGui::Selectable(buf, chunk.active)) {
                // Add chunk to the list
                chunk.active = !chunk.active;
            }
        }
        ImGui::EndListBox();
    }

    ImGui::End();

    // Draw window for all chunks being displayed right now
    for (u32 i = 0; i < chunks.size(); i++ ) {
        polaris::chunk& chunk = chunks.at(i);
        if (!chunk.active) {
            continue;
        }

        // Each window needs a unique ID, but "##x" isn't shown
        const char* known_name = "";
        switch (chunk.id) {
        case 0x2:
            known_name = "[Index Buffer]";
            break;
        case 0x3:
            known_name = "[Transform Matrix]";
            break;
        case 0x10:
            known_name = "[Texture Atlas]";
            break;
        case 0x11:
            known_name = "[Header]";
            break;
        }
        char buf[0x30] = {0};
        snprintf(buf, sizeof(buf), "0x%X %s Chunk @ 0x%lX ##%u", chunk.id, known_name, chunk.offset, i);

        if (ImGui::Begin(buf, &chunk.active)) {
            chunk.draw(this);
        }

        ImGui::End();
    }


    ImGui::ShowDemoWindow();

    // It's the end of the frame for us, save the current input
    prev_input = input;
    return true;
}

std::vector<polaris::chunk> polaris::shatter_alr(const u8* buf, s64 size) noexcept {
    // Technically we cast away const here, but we don't write any data so it's
    // fine.
    vfile vf = vfile_open((void*)buf, size);
    std::vector<polaris::chunk> out;

    // Loop until we exhaust the buffer or exit early
    u32 prev_id = -1;
    while (!vfile_eof(vf)) {
        // Read chunk data. We have to copy it over 1 field at a time because we
        // don't actually want/need any more of the chunk data.
        const u32 offset = vf.pos; // It's important to save offset before reading
        const u32 id = VFILE_READ(u32, &vf);
        polaris::chunk chunk(id);
        chunk.size = VFILE_READ(s32, &vf);
        chunk.offset = offset;

        if (chunk.id == 0 && prev_id == 0) {
            // There's never multiple consecutive chunks with ID 0. This means
            // we've hit an empty area, probably the end of the chunk data.
            break;
        }

        if (chunk.id == 0x11) {
            // This chunk has the resource buffer offset, save it for later
            // Our layout structure includes the id & size, so we have to seek
            // back for that...
            vf.pos -= sizeof(chunk_generic);
            chunk_layout layout = VFILE_READ(chunk_layout, &vf);
            resbuf_offset = layout.texbuf_offset;

            // Reset the position to what it used to be
            vf.pos -= sizeof(layout);
        }

        // Advance to the next chunk & add to output
        vf.pos = chunk.offset + chunk.size;
        out.push_back(chunk);
        prev_id = chunk.id; // Save current ID
    }

    return out;
}