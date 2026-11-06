#pragma once
#include <imgui.h>
#include <imgui_hex_editor.h>

#include <util/fileclass.hxx>

/// ImGui tool menu for stageset & charaset files
struct xset_tool : fileclass {
    bool enabled = false; // Whether the window is showing
    bool is_charaset = false;
    u32 selected_entry = 0;
    MemoryEditor hex_edit;

    void do_gui() noexcept;
    char* get_textbuf() noexcept;
    const char* name_for_entry(u32 idx) noexcept;
};
