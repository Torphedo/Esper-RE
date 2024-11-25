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
    #include "viewer/render_image.h"
    #include "viewer/viewer.h"
    #include "viewer/camera.h"
}

bool polaris::do_gui(GLFWwindow* window) {
    if (ImGui::GetIO().WantCaptureMouse) {
        // ImGui wants control of the mouse (it's probably over a window),
        // so we'll suppress the real cursor state this frame.
        input.cursor = this->prev_input.cursor;
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
            this->alr_data = file_load(path.c_str());
            if (this->alr_data != nullptr) {
                this->alr_size = file_size(path.c_str());
                this->chunks = shatter_alr(this->alr_data, this->alr_size);
            }
        }

        // Close the dialog
        ImGuiFileDialog::Instance()->Close();
    }

    static s32 selected = 0;
    if (ImGui::BeginListBox(" ", ImVec2(0, -FLT_MIN))) {
        for (size_t n = 0; n < this->chunks.size(); n++) {
            chunk_desc chunk = this->chunks.at(n);
            char buf[128];
            sprintf(buf, "0x%x chunk @ 0x%lx [%d bytes] ##%d", chunk.id, chunk.offset, chunk.size, n);
            const bool is_selected = (selected == n);
            if (ImGui::Selectable(buf, is_selected)) {
                selected = n;
            }
            if (is_selected) {
                ImGui::SetItemDefaultFocus();
            }
        }
        ImGui::EndListBox();
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