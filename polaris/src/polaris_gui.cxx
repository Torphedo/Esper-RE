#include <cstdlib>
#include <string>

#include <imgui.h>
#include <ImGuiFileDialog.h>
#include "polaris_gui.hxx"

extern "C" {
    #include <GLFW/glfw3.h>
    #include <common/image.h>
    #include <common/file.h>
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

    static s32 selected = 0;
    ImGui::BeginListBox(" ");
    for (int n = 0; n < 55; n++) {
        char buf[32];
        sprintf(buf, "Object %d", n);
        const bool is_selected = (selected == n);
        if (ImGui::Selectable(buf, is_selected)) {
            selected = n;
        }
        if (is_selected) {
            ImGui::SetItemDefaultFocus();
        }
    }
    ImGui::EndListBox();
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
            if (buf != NULL && file_exists(path.c_str())) {
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
    camera_update(NULL, ratio);

    if (this->img_ctx.img.data != NULL) {
        image_render(&this->img_ctx, window);

        // Manages active texture's format, dimensions, etc.
        viewer_update(&this->img_ctx.img, this->img_ctx.gl_img);
    }

    ImGui::ShowDemoWindow();

    // It's the end of the frame for us, save the current input
    this->prev_input = input;
    return true;
}
