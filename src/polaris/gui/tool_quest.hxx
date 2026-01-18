#pragma once
#include <imgui.h>
#include <imgui_hex_editor.h>

#include <formats/questdata.h>
#include "util/fileclass.hxx"

/// ImGui tool menu for audio files
struct quest_tool : fileclass {
    bool enabled = false; // Whether the window is showing
    u32 selected_quest = 0;
    MemoryEditor hex_edit;

    void do_gui() noexcept;
};
