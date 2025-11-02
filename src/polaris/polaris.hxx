#pragma once
#include <glad/glad.h>
#include <vector>
#include <string>
#include <unordered_map>

#include <formats/alr.h>
#include <layer.hxx>

#include "alr/editor_alr.hxx"
#include "mapdata.hxx"
#include "viewport.hxx"

extern "C" {
    #include <common/gl/input.h>
}

// State for the overall editor
struct polaris : gui_layer {
    // State for a loaded ALR file
    al::resource alr = al::resource(*this);

    // State for accompanying .dat file for a stage ALR.
    mapdata map = mapdata(*this);

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

    void do_menu_bar() noexcept;

    void init(GLFWwindow* window) noexcept override;
    
    /// @brief Render and update all the UI
    void update(GLFWwindow *window) noexcept override;
    void render(GLFWwindow *window) noexcept override;
    void destroy() noexcept override;
};
