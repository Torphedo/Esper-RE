#pragma once
#include <vector>
#include <string>
#include <unordered_map>

#include <formats/alr.h>

#include "editor_alr.hxx"
#include "mapdata.hxx"
#include "editor_cad.hxx"
#include "viewport.hxx"

extern "C" {
    #include <common/gl/input.h>
}

// State for the overall editor
struct polaris {
    // State for a loaded ALR file
    al::resource alr;

    // State for accompanying .dat file for a stage ALR.
    mapdata map;

    // State for .cad AI pathfinding data
    editor_cad ai_cad;

    // TODO: Unload textures if they aren't being used in the viewport when loading another ALR, to save on memory.
    // Set of OpenGL textures used in the viewport
    std::vector<gl_obj> gl_textures;

    // 3D viewport
    viewport_t viewport;

    // Whether we're running without graphics.
    bool headless = true;

    // Whether to show the ImGui Demo Window
    bool show_demo = false;

    // Whether we show a window with all the debug performance timers.
    bool show_timers = false;

    // Set of named timers keyed by name.
    // "mutable" allows const methods to modify this
    mutable std::unordered_map<const char*, double> timer_map;

    // Input state from the previous frame
    input_internal prev_input = {};

    /// @brief Hide input from the rest of the program when ImGui is using it.
    void handle_input_suppression() noexcept;

    void unload_gl_textures() noexcept;

    void do_menu_bar() noexcept;

    /// @brief Render and update all the UI
    void do_gui(GLFWwindow *window) noexcept;

    /// @brief Increase the amount of address space reserved for the ALR data
    void expand_reservation(s64 new_size) noexcept;

    polaris() noexcept;
    ~polaris() noexcept {
        this->unload_gl_textures();
    }
};
