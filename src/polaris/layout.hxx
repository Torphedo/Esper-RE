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
    layout_t(const char* filepath);
    layout_t() = default;

    layout_t& operator=(layout_t&& other);
    ~layout_t();

    /// @brief Render and update all the UI
    void do_gui() noexcept;
};
