#pragma once
#include "polaris.hxx"

/// @brief Underlying GUI loop for the program
///
/// This function handles the main loop and setup/teardown of ImGui & GLFW,
/// but the main program UI & logic is in polaris::do_gui().
/// @return Returns false when unable to create the UI
bool gui_main(polaris& pol);