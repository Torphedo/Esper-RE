#include <cstdlib>
#include <string>

#include <imgui.h>
#include <ImGuiFileDialog.h>

extern "C" {
    #include <GLFW/glfw3.h>
    #include <common/image.h>
    #include <common/file.h>
    #include <common/logging.h>
    #include "viewer/render_image.h"
    #include "viewer/viewer.h"
    #include "viewer/camera.h"
}

bool polaris_gui(GLFWwindow* window) {
    static img_state img_ctx = {0};

    ImGui::Begin("Temp Window");

    if (ImGui::Button("Open File Dialog")) {
        ImGuiFileDialog::Instance()->OpenDialog("chooseTex", "Choose File", ".dds,.bin", {});
    }

    // Display file dialog if appropriate
    if (ImGuiFileDialog::Instance()->Display("chooseTex")) {
        if (ImGuiFileDialog::Instance()->IsOk()) { // action if OK
            const std::string path = ImGuiFileDialog::Instance()->GetFilePathName();

            // Load the texture
            const s64 size = 32 * 1024 * 1024;
            u8* buf = (u8*)calloc(1, size);
            memset(buf, 0xCC, size);
            if (buf != NULL && file_exists(path.c_str())) {
                // The buffer pointer we just allocated is copied into the context
                const texture tex = image_buf_load(path.c_str(), buf, size);
                img_ctx = image_init(tex);
            }
        }
    
        // close
        ImGuiFileDialog::Instance()->Close();
    }

    ImGui::End();

    float ratio = (float)img_ctx.img.width / (float)img_ctx.img.height;
    if (img_ctx.img.width == 0 || img_ctx.img.height == 0) {
        ratio = 1.0f; 
    }
    camera_update(NULL, ratio);

    if (img_ctx.img.data != NULL) {
        image_render(&img_ctx, window);

        // Manages active texture's format, dimensions, etc.
        viewer_update(&img_ctx.img, img_ctx.gl_img);
    }

    ImGui::ShowDemoWindow();

    return true;
}
