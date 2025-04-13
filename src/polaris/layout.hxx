#pragma once
#include <common/int.h>

#include <imgui.h>
#include <imgui_hex_editor.h>

class layout_t {
public:
    u8* data = nullptr;
    size_t size = 0;

    MemoryEditor hex_edit;
    u32 selected_chunk = 0;

    bool initialized = false;

    // Explicit constructor
    static layout_t setup(const char* filepath);
    layout_t() = default;
    void destroy();

    /// @brief Render and update all the UI
    void do_gui() noexcept;
};