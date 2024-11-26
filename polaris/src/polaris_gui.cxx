#include <cstdlib>
#include <string>
#include <algorithm>

#include <imgui.h>
#include <ImGuiFileDialog.h>
#include "polaris_gui.hxx"

extern "C" {
    #include <GLFW/glfw3.h>
    #include <common/image.h>
    #include <common/file.h>
    #include <common/vfile.h>
    #include <common/logging.h>
    #include <common/gl/input.h>
    #include <formats/alr.h>
    #include <formats/pd_common.h>
    #include "viewer/render_image.h"
    #include "viewer/viewer.h"
    #include "viewer/camera.h"
}

polaris::polaris() {
    // We basically always want to preview as a float for matrices
    matrixHex.OptShowDataPreview = true;
    matrixHex.PreviewDataType = ImGuiDataType_Float;

    vert_bufHex.OptShowDataPreview = true;
    vert_bufHex.OptShowAscii = false;
    vert_bufHex.PreviewDataType = ImGuiDataType_U32;
}

polaris::~polaris() {
    free(alr_data);
    // TODO: The image may be in the middle of another buffer, so it doesn't
    // free the buffer on destruction. We need some other pointer or pool for
    // texture buffers.
    image_destroy(img_ctx);
}

void polaris::handle_input_suppression() {
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

bool polaris::save_alr(const char* path) {
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

bool polaris::do_gui(GLFWwindow* window) {
    this->handle_input_suppression();

    ImGui::Begin("ALR Select");
    if (ImGui::Button("Load ALR")) {
        ImGuiFileDialog::Instance()->OpenDialog("chooseALR", "Choose ALR File", ".alr", {});
    }

    // Display file dialog if appropriate
    if (ImGuiFileDialog::Instance()->Display("chooseALR")) {
        // If user cancels, we can't load anything
        if (ImGuiFileDialog::Instance()->IsOk()) {
            const std::string path = ImGuiFileDialog::Instance()->GetFilePathName();

            // Load the texture
            free(alr_data);
            alr_data = file_load(path.c_str());
            if (alr_data != nullptr) {
                alr_size = file_size(path.c_str());
                chunks = shatter_alr(alr_data, alr_size);
            }
        }

        // Close the dialog
        ImGuiFileDialog::Instance()->Close();
    }

    ImGui::SameLine();
    if (ImGui::Button("Save ALR")) {
        ImGuiFileDialog::Instance()->OpenDialog("chooseALRSave", "Choose ALR File", ".alr", {});
    }

    // Display file dialog if appropriate
    if (ImGuiFileDialog::Instance()->Display("chooseALRSave")) {
        // If user cancels, we can't load anything
        if (ImGuiFileDialog::Instance()->IsOk()) {
            const std::string path = ImGuiFileDialog::Instance()->GetFilePathName();

        }

        // Close the dialog
        ImGuiFileDialog::Instance()->Close();
    }

    if (ImGui::BeginListBox(" ", ImVec2(0, -FLT_MIN))) {
        for (size_t n = 0; n < chunks.size(); n++) {
            chunk_desc chunk = chunks.at(n);
            char buf[128];
            sprintf(buf, "0x%02X chunk @ 0x%02lX [%d bytes] ##%lu", chunk.id, chunk.offset, chunk.size, n);

            // Vectors don't have a "contains" method, we have to use std::find
            const bool is_selected = std::find(selected_chunks.begin(), selected_chunks.end(), n) != selected_chunks.end();
            if (ImGui::Selectable(buf, is_selected) && !is_selected) {
                // Add chunk index to the list
                selected_chunks.push_back(n);
            }
        }
        ImGui::EndListBox();
    }

    ImGui::End();

    // Draw window for all chunks being displayed right now
    for (u32 i = 0; i < selected_chunks.size(); i++ ) {
        size_t idx = selected_chunks.at(i);
        if (idx >= chunks.size()) {
            continue;
        }
        const chunk_desc chunk = chunks.at(idx);

        // Each window needs a unique ID, but "##x" isn't shown
        char buf[0x20] = {0};
        sprintf(buf, "Chunk View [0x%X]##%lu", chunk.id, idx);

        bool keep_showing = true;
        if (ImGui::Begin(buf, &keep_showing)) {
            this->do_chunk_menu(chunk);
        }

        if (!keep_showing) {
            // Vectors have strange methods for dealing with indices, sorry.
            selected_chunks.erase(selected_chunks.begin() + i);
        }
        ImGui::End();
    }

    ImGui::Begin("Texture Select");
    if (ImGui::Button("Open File Dialog")) {
        ImGuiFileDialog::Instance()->OpenDialog("chooseTex", "Choose Texture File", ".dds,.bin", {});
    }

    // Display file dialog if appropriate
    if (ImGuiFileDialog::Instance()->Display("chooseTex")) {
        // If user cancels, we can't load anything
        if (ImGuiFileDialog::Instance()->IsOk()) {
            const std::string path = ImGuiFileDialog::Instance()->GetFilePathName();

            // Load the texture
            const s64 size = 32 * 1024 * 1024;
            u8* buf = (u8*)calloc(1, size);
            memset(buf, 0xCC, size);
            if (buf != nullptr && file_exists(path.c_str())) {
                // The buffer pointer we just allocated is copied into the context
                const texture tex = image_buf_load(path.c_str(), buf, size);
                img_ctx = image_init(tex, true);
            }
        }
    
        // Close the dialog
        ImGuiFileDialog::Instance()->Close();
    }

    ImGui::End();

    float ratio = (float)img_ctx.img.width / (float)img_ctx.img.height;
    if (img_ctx.img.width == 0 || img_ctx.img.height == 0) {
        ratio = 1.0f; 
    }
    camera_update(nullptr, ratio);

    if (img_ctx.img.data != nullptr) {
        image_render(&img_ctx, window);

        // Manages active texture's format, dimensions, etc.
        viewer_update(&img_ctx.img, img_ctx.gl_img);
    }

    ImGui::ShowDemoWindow();

    // It's the end of the frame for us, save the current input
    prev_input = input;
    return true;
}

std::vector<chunk_desc> polaris::shatter_alr(const u8* buf, s64 size) {
    // Technically we cast away const here, but we don't write any data so it's
    // fine.
    vfile vf = vfile_open((void*)buf, size);
    std::vector<chunk_desc> out;

    // Loop until we exhaust the buffer or exit early
    u32 prev_id = -1;
    while (!vfile_eof(vf)) {
        // Read chunk data. We have to copy it over 1 field at a time because we
        // don't actually want/need any more of the chunk data.
        chunk_desc chunk = {.offset = vf.pos};
        chunk.id = VFILE_READ(u32, &vf);
        chunk.size = VFILE_READ(s32, &vf);

        if (chunk.id == 0 && prev_id == 0) {
            // There's never multiple consecutive chunks with ID 0. This means
            // we've hit an empty area, probably the end of the chunk data.
            break;
        }

        // Advance to the next chunk & add to output
        vf.pos = chunk.offset + chunk.size;
        out.push_back(chunk);
        prev_id = chunk.id; // Save current ID
    }

    return out;
}

void polaris::do_chunk_menu(chunk_desc chunk) {
    if (alr_data == nullptr || alr_size == 0) {
        // There's no data to work on, we can't display any useful data.
        return;
    }

    switch (chunk.id) {
        case 0x2:
            this->chunk_0x2(chunk);
            break;
        case 0x3:
            this->chunk_0x3(chunk);
            break;
        case 0x10:
            this->chunk_0x10(chunk);
            break;
        case 0x11:
            this->chunk_0x11(chunk);
            break;
        case 0x15:
            this->chunk_0x15(chunk);
            break;
        case 0x16:
            this->chunk_0x16(chunk);
            break;
        default:
            // Unimplemented window
            ImGui::Text("Unimplemented chunk type");
            return;
    }
}

void polaris::chunk_0x2(chunk_desc chunk) {
    char dialog_key[0x20] = {0};
    snprintf(dialog_key, sizeof(dialog_key), "chooseOBJ_idx##%lu", chunk.offset);
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

            vfile vf = vfile_open(alr_data + chunk.offset, chunk.size);

            // Skip over chunk header
            vfile_seek(&vf, sizeof(chunk_generic));
            vfile_seek(&vf, sizeof(idx_buf_header));

            const u32 num_tris = (vf.size - vf.pos) / (3 * sizeof(u16));
            for (u32 i = 0; i < num_tris; i++) {
                u16 idx1 = VFILE_READ(u16, &vf);
                u16 idx2 = VFILE_READ(u16, &vf);
                u16 idx3 = VFILE_READ(u16, &vf);

                // OBJ indices start at 1 :(
                idx1++;
                idx2++;
                idx3++;

                fprintf(out, "f %d %d %d\n", idx1, idx2, idx3);
            }

            fclose(out);
        }

        // Close the dialog
        ImGuiFileDialog::Instance()->Close();
    }
}
void polaris::chunk_0x3(chunk_desc chunk) {
    if (chunk.id != 0x3) {
        return;
    }

    vfile vf = vfile_open(alr_data + chunk.offset, chunk.size);
    vfile_seek(&vf, sizeof(chunk_generic)); // Skip ID & size

    const u32 num_matrices = (chunk.size - sizeof(chunk_generic) - sizeof(chunk_transform)) / sizeof(mat4);
    const u16 num_non_identity = VFILE_READ(u16, &vf);
    const u16 unk = VFILE_READ(u16, &vf);
    const u32 pad = VFILE_READ(u32, &vf);
    auto *matrices = (mat4 *) vfile_cur(vf);

    const u32 min = 0;
    const u32 max = MAX(num_matrices - 1, 0);
    ImGui::Checkbox("Use slider", &mat_slider);
    if (mat_slider) {
        ImGui::SliderScalar("Selected Matrix", ImGuiDataType_S32, &selected_mat, &min, &max);
    } else {
        ImGui::InputScalar("Selected Matrix", ImGuiDataType_S32, &selected_mat);
    }
    // Don't allow out of bounds index
    selected_mat = CLAMP(min, selected_mat, max);

    ImGui::Text("%d matrices [%d identity]", num_matrices, num_matrices - num_non_identity);

    if (ImGui::BeginTabBar("Matrix Editing")) {
        if (ImGui::BeginTabItem("Raw editor")) {

            // Matrix inputs
            ImGui::PushItemWidth(200.0f); // Make inputs narrower
            ImGui::Text("Matrix Editor");
            for (u32 j = 0; j < 4; j++) {
                for (u32 k = 0; k < 4; k++) {
                    char buf[0x20] = {0};
                    sprintf(buf, "##%d%d%d", j, k);
                    ImGui::InputFloat(buf, &matrices[selected_mat][j][k]);
                    ImGui::SameLine();
                }
                ImGui::Text(" "); // Cause a new line
            }
            ImGui::PopItemWidth();
            ImGui::EndTabItem();
        }

        if (ImGui::BeginTabItem("Hex Editing")) {
            // Show hex editor
            matrixHex.DrawContents(&matrices[selected_mat], sizeof(matrices[selected_mat]));

            ImGui::EndTabItem();
        }

        ImGui::EndTabBar();
    }
}

void polaris::chunk_0x10(chunk_desc chunk) {
    if (chunk.id != 0x10) {
        // Exit if we were called by mistake
        return;
    }

    // We use the vfile API to handle the chunk data
    vfile vf = vfile_open(alr_data + chunk.offset, chunk.size);
    // Skip over the ID and size fields we already have (both 32-bit)
    vfile_seek(&vf, sizeof(chunk.id) + sizeof(chunk.size));

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

    ImGui::Text("%d Atlases for %s:", header->atlas_count, header->alr_name);
    if (ImGui::BeginListBox("Texture Atlases")) {
        for (u32 i = 0; i < header->atlas_count; i++) {
            char buf[sizeof(atlas_names[i].name) + 0x20] = {0};
            atlas_info surface = atlases[i];
            snprintf(buf, sizeof(buf) - 1, "%s [%dx%d]", atlas_names[i].name, surface.width, surface.height);

            if (ImGui::Selectable(buf, selected_atlas == i)) {
                selected_atlas = i;
            }
        }
        ImGui::EndListBox();
    }

    // Display textures in the selected atlases
    if (ImGui::BeginListBox("Atlas Contents")) {
        for (u32 i = 0; i < header->texture_count; i++) {
            const tex_info tex = textures[i];
            // Only list textures belonging to the selected atlases
            if (tex.index != selected_atlas) {
                continue;
            }

            char buf[sizeof(textures[i].filename) + 0x20] = {0};
            snprintf(buf, sizeof(buf) - 1, "%s [%dx%d]", tex.filename, tex.width, tex.height);

            if (ImGui::Selectable(buf, selected_atlas_texture == i)) {
                selected_atlas_texture = i;
                // Once we have a mechanism to find the atlases' position in the
                // texture buffer, clicking on a texture should set it as the
                // active texture and display it.
            }
        }

        ImGui::EndListBox();
    }
}

void polaris::chunk_0x11(chunk_desc chunk) {
    if (chunk.id != 0x11) {
        return;
    }

    vfile vf = vfile_open(alr_data + chunk.offset, chunk.size);
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

void polaris::chunk_0x15(chunk_desc chunk) {
    if (chunk.id != 0x15) {
        // Exit if we were called by mistake
        return;
    }

    // We use the vfile API to handle the chunk data
    vfile vf = vfile_open(alr_data + chunk.offset, chunk.size);
    // Skip over the ID and size fields we already have (both 32-bit)
    vfile_seek(&vf, sizeof(chunk_generic));

    const u32 num_entries = VFILE_READ(u32, &vf);
    auto* entries = (resource_entry*)vfile_cur(vf);

    if (ImGui::BeginListBox("Textures")) {
        for (u32 i = 0; i < num_entries; i++) {
            char buf[0x30] = {0};
            char name[PD_ENCODED_CHAR_COUNT + 1] = {0};
            decode_single32(name, entries[i].text1);
            decode_single32(&name[6], entries[i].text2);

            const u32 size = 1 << entries[i].resolution_pwr;
            snprintf(buf, sizeof(buf) - 1, "%s [%dx%d] @ 0x%X", name, size, size, entries[i].data_ptr);

            ImGui::Selectable(buf, false);
        }
        ImGui::EndListBox();
    }
}

void polaris::chunk_0x16(chunk_desc chunk) {
    if (chunk.id != 0x16) {
        // Exit if we were called by mistake
        return;
    }

    // We use the vfile API to handle the chunk data
    vfile vf = vfile_open(alr_data + chunk.offset, chunk.size);
    // Skip over the ID and size fields we already have (both 32-bit)
    vfile_seek(&vf, sizeof(chunk_generic));

    const u32 num_entries = VFILE_READ(u32, &vf);
    auto* entries = (resource_entry_0x16*)vfile_cur(vf);

    ImGui::BeginChild("Vertex Buffers", ImVec2(300, 0));
    for (u32 i = 0; i < num_entries; i++) {
        char buf[0x30] = {0};
        snprintf(buf, sizeof(buf) - 1, "0x%X verts @ 0x%X", entries[i].vertex_count, entries[i].data_ptr);

        const bool is_selected = selected_vertex_buf == i;
        if (ImGui::Selectable(buf, is_selected)) {
            selected_vertex_buf = i;
        }
    }
    ImGui::EndChild();
    ImGui::SameLine();

    ImGui::BeginChild("Vertex Buffer Settings", ImVec2(600, 0));
    if (ImGui::Button("Dump to OBJ")) {
        // All we can do this frame is open the dialog
        ImGuiFileDialog::Instance()->OpenDialog("chooseOBJ", "Choose OBJ File", ".obj", {});
    }

    vert_bufHex.DrawContents(&entries[selected_vertex_buf], sizeof(*entries));

    // Display file dialog if appropriate
    if (ImGuiFileDialog::Instance()->Display("chooseOBJ")) {
        // If user cancels, we can't load anything
        if (ImGuiFileDialog::Instance()->IsOk()) {
            const std::string path = ImGuiFileDialog::Instance()->GetFilePathName();

            // Dump to OBJ
            resource_entry_0x16 entry = entries[selected_vertex_buf];
            FILE *out = fopen(path.c_str(), "wb");
            if (out != nullptr) {
                vf = vfile_open(alr_data, alr_size);

                // Jump to resource buffer
                chunk_layout layout = VFILE_READ(chunk_layout, &vf);
                vf.pos = layout.texbuf_offset + entry.data_ptr;

                for (u32 i = 0; i < entry.vertex_count; i++) {
                    // Read our vertex positions, which always come first
                    const vec3s vert = VFILE_READ(vec3s, &vf);
                    fprintf(out, "v %f %f %f\n", vert.x, vert.y, vert.z);

                    // There might be some data left over, for now we just skip
                    // over it.
                    const u32 skip = entry.vertex_size - (sizeof(vert));
                    vfile_seek(&vf, skip);
                }

                fclose(out);
            }
        }

        // Close the dialog
        ImGuiFileDialog::Instance()->Close();
    }

    ImGui::EndChild();
}
