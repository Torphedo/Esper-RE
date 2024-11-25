#include <cstdlib>
#include <string>

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
    this->matrixHex.OptShowDataPreview = true;
    this->matrixHex.PreviewDataType = ImGuiDataType_Float;
}

polaris::~polaris() {
    free(this->alr_data);
}

bool polaris::do_gui(GLFWwindow* window) {
    if (ImGui::GetIO().WantCaptureMouse) {
        // ImGui wants control of the mouse (it's probably over a window),
        // so we'll suppress the real mouse state this frame.
        input.cursor = this->prev_input.cursor;
        input.scroll = this->prev_input.scroll;
        input.click_left = this->prev_input.click_left;
        input.click_right = this->prev_input.click_right;
        input.click_middle = this->prev_input.click_middle;
        input.mouse_button_4 = this->prev_input.mouse_button_4;
        input.mouse_button_5 = this->prev_input.mouse_button_5;
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
        input = this->prev_input;

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
            free(this->alr_data);
            this->alr_data = file_load(path.c_str());
            if (this->alr_data != nullptr) {
                this->alr_size = file_size(path.c_str());
                this->chunks = shatter_alr(this->alr_data, this->alr_size);
            }
        }

        // Close the dialog
        ImGuiFileDialog::Instance()->Close();
    }

    if (ImGui::BeginListBox(" ", ImVec2(0, -FLT_MIN))) {
        for (size_t n = 0; n < this->chunks.size(); n++) {
            chunk_desc chunk = this->chunks.at(n);
            char buf[128];
            sprintf(buf, "0x%02X chunk @ 0x%02lX [%d bytes] ##%lu", chunk.id, chunk.offset, chunk.size, n);
            const bool is_selected = (this->selected_chunk == n);
            if (ImGui::Selectable(buf, is_selected)) {
                this->selected_chunk = n;
            }
        }
        ImGui::EndListBox();
    }

    ImGui::End();

    // Window for the currently selected chunk
    ImGui::Begin("ALR Editor");
    if (this->selected_chunk < this->chunks.size()) {
        this->do_chunk_menu(this->chunks.at(this->selected_chunk));
    }
    ImGui::End();

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
                this->img_ctx = image_init(tex);
            }
        }
    
        // Close the dialog
        ImGuiFileDialog::Instance()->Close();
    }

    ImGui::End();

    float ratio = (float)this->img_ctx.img.width / (float)img_ctx.img.height;
    if (this->img_ctx.img.width == 0 || this->img_ctx.img.height == 0) {
        ratio = 1.0f; 
    }
    camera_update(nullptr, ratio);

    if (this->img_ctx.img.data != nullptr) {
        image_render(&this->img_ctx, window);

        // Manages active texture's format, dimensions, etc.
        viewer_update(&this->img_ctx.img, this->img_ctx.gl_img);
    }

    ImGui::ShowDemoWindow();

    // It's the end of the frame for us, save the current input
    this->prev_input = input;
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
    if (this->alr_data == nullptr || this->alr_size == 0) {
        // There's no data to work on, we can't display any useful data.
        return;
    }
    switch (chunk.id) {
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
        default:
            return;
    }
}

void polaris::chunk_0x3(chunk_desc chunk) {
    if (chunk.id != 0x3) {
        return;
    }

    vfile vf = vfile_open(this->alr_data + chunk.offset, chunk.size);
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
        ImGui::SliderScalar("Selected Matrix", ImGuiDataType_S32, &this->selected_mat, &min, &max);
    } else {
        ImGui::InputInt("Selected Matrix", (int*)&this->selected_mat);
    }
    // Don't allow out of bounds index
    this->selected_mat = CLAMP(min, this->selected_mat, max);

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
                    ImGui::InputFloat(buf, &matrices[this->selected_mat][j][k]);
                    ImGui::SameLine();
                }
                ImGui::Text(" "); // Cause a new line
            }
            ImGui::PopItemWidth();
            ImGui::EndTabItem();
        }

        if (ImGui::BeginTabItem("Hex Editing")) {
            // Show hex editor
            matrixHex.DrawContents(&matrices[this->selected_mat], sizeof(matrices[this->selected_mat]));

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
    vfile vf = vfile_open(this->alr_data + chunk.offset, chunk.size);
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

            if (ImGui::Selectable(buf, this->selected_atlas == i)) {
                this->selected_atlas = i;
            }
        }
        ImGui::EndListBox();
    }

    // Display textures in the selected atlases
    if (ImGui::BeginListBox("Atlas Contents")) {
        for (u32 i = 0; i < header->texture_count; i++) {
            const tex_info tex = textures[i];
            // Only list textures belonging to the selected atlases
            if (tex.index != this->selected_atlas) {
                continue;
            }

            char buf[sizeof(textures[i].filename) + 0x20] = {0};
            snprintf(buf, sizeof(buf) - 1, "%s [%dx%d]", tex.filename, tex.width, tex.height);

            if (ImGui::Selectable(buf, this->selected_atlas_texture == i)) {
                this->selected_atlas_texture = i;
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

    vfile vf = vfile_open(this->alr_data + chunk.offset, chunk.size);
    auto* layout = (chunk_layout*)vfile_cur(vf);
    vfile_seek(&vf, sizeof(*layout));
    auto* offsets = (u32*)vfile_cur(vf);

    ImGui::Text("Texture buffer @ 0x%X [%d bytes]", layout->texbuf_offset, layout->texbuf_size);
    ImGui::Text("%d offsets in array:\n", layout->offset_array_size);

    ImGui::BeginListBox("Offsets");
    for (u32 i = 0; i < layout->offset_array_size; i++) {
        ImGui::Text("0x%X", offsets[i]);
    }
    ImGui::EndListBox();
}

void polaris::chunk_0x15(chunk_desc chunk) {
    if (chunk.id != 0x15) {
        // Exit if we were called by mistake
        return;
    }

    // We use the vfile API to handle the chunk data
    vfile vf = vfile_open(this->alr_data + chunk.offset, chunk.size);
    // Skip over the ID and size fields we already have (both 32-bit)
    vfile_seek(&vf, sizeof(chunk.id) + sizeof(chunk.size));

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
