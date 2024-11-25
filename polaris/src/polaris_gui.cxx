#include <cstdlib>

#include <imgui.h>
#include <GLFW/glfw3.h>

extern "C" {
    #include <common/image.h>
    #include <common/file.h>
    #include "viewer/render_image.h"
}

bool polaris_gui(GLFWwindow* window) {
    static img_state img_ctx = {0};
    if (img_ctx.img.data == NULL) {
        const char* path = "player_alb.bin";
        const s64 size = 32 * 1024 * 1024;
        u8* buf = (u8*)calloc(1, size);
        memset(buf, 0xCC, size);
        if (buf != NULL) {
            // The buffer pointer we just allocated is copied into the context
            const texture tex = image_buf_load(path, buf, size);
            img_ctx = image_init(tex);
        }
    }

    if (img_ctx.img.data != NULL) {
        image_render(&img_ctx, window);
    }

    ImGui::ShowDemoWindow();

    return true;
}
