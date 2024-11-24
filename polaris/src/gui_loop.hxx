#pragma once
extern "C" {
    #include <GLFW/glfw3.h>
}

/// @brief Main GUI loop for the program
///
/// @param gui_callback A callback function where you can render your actual
/// Dear ImGui UI. The callback is called every frame, and the loop exits if
/// it returns false.
bool gui_main(bool (*gui_callback)(GLFWwindow* window));
