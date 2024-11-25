#pragma once
#include <GLFW/glfw3.h>

extern "C" {
    #include "common/gl/input.h"
    #include "viewer/render_image.h"
}

struct polaris {
public:
    input_internal prev_input;
    img_state img_ctx;

    /// @brief Effectively the "real" entry point for Polaris, driving the UI
    ///
    /// @param window The window, needed for aspect ratio and input and such
    /// @return Returns false when the user wants to exit (if ever)
    bool do_gui(GLFWwindow* window);
};
