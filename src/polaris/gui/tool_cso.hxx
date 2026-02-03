#pragma once
#include <imgui.h>
#include <imgui_hex_editor.h>

#include <util/fileclass.hxx>

/// ImGui tool menu for CSO shaders
struct cso_tool : fileclass {
    bool enabled = false; // Whether the window is showing
    MemoryEditor hex_edit;

    void do_gui() noexcept;
};
