#pragma once
#include <common/int.h>

#include <imgui.h>
#include <imgui_hex_editor.h>

// This class represents ".dat" map files in memory
class mapdata {
public:
    u8* data = nullptr;
    size_t size = 0;

    MemoryEditor hex_edit;
    u32 selected_chunk = 0;

    bool initialized = false;

    // Explicit constructor
    mapdata(const char* filepath);
    mapdata() = default;

    mapdata& operator=(mapdata&& other);
    ~mapdata();

    /// @brief Render and update all the UI
    void do_gui() noexcept;
};
