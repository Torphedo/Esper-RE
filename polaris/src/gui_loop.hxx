#pragma once
extern "C" {
    #include <GLFW/glfw3.h>
}

/// @brief Underlying GUI loop for the program
///
/// This function handles the main loop and setup/teardown of ImGui & GLFW,
/// but doesn't actually contain the main program UI or logic.
/// @param gui_callback A callback function where you can render your actual
/// Dear ImGui UI. The callback is called every frame, and the loop exits if
/// it returns false.
/// @return Returns false when unable to create the UI
bool gui_main(bool (*gui_callback)(GLFWwindow* window));
