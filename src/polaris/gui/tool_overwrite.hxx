#pragma once
#include <common/int.h>

struct overwrite_tool {
    bool enabled = false; // Whether the window is showing

    // Shared state
    u32 source_offset = 0;
    u32 target_offset = 0;

    u32 operation_count = 1;
    u32 target_step = 0;

    // memcpy() state
    u32 copy_size = 0;
    u32 source_step = 0;

    // memset() state
    u32 set_size = 0;
    u8 value = 0;

    void do_gui(void* data) noexcept;
    void advanced_submenu(bool showSourceStep) noexcept;
};
