#pragma once
#include <layer.hxx>

#include "mapdata_editor.hxx"
#include "alr_editor.hxx"
#include "render_context.hxx"
#include "tool_audio.hxx"
#include "tool_quest.hxx"

// State for the overall editor
struct polaris : gui_layer {
    // State for a loaded ALR file
    alr::editor editor;

    // State for accompanying .dat file for a stage ALR.
    mapdata_editor mapEdit = mapdata_editor(*this);

    audio_tool audioTool;
    quest_tool questTool;
    render_context viewport; // 3D viewport

    bool headless = true; // Whether we're running without graphics.
    bool show_demo = false; // ImGui Demo Window toggle
    bool show_about = false; // About menu toggle

    // Whether we show a window with all the debug performance timers.
    bool show_timers = false;

    void do_menu_bar() noexcept;

    void init(GLFWwindow* window) noexcept override;
    
    /// @brief Render and update all the UI
    void update(GLFWwindow *window) noexcept override;
    void render(GLFWwindow *window) noexcept override;
    void destroy() noexcept override;
};
