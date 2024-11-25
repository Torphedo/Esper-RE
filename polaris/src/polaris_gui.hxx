#include <GLFW/glfw3.h>

/// @brief Effectively the "real" entry point for Polaris, driving the UI
///
/// @param window The window, needed for aspect ratio and input and such
/// @return Returns false when the user wants to exit (if ever)
bool polaris_gui(GLFWwindow* window);
