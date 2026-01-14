#pragma once
#include "util/fileclass.hxx"

#include <formats/questdata.h>

/// ImGui tool menu for audio files
struct quest_tool : fileclass {
    bool enabled = false; // Whether the window is showing
    u32 selected_quest = 0;

    void do_gui() noexcept;
};
