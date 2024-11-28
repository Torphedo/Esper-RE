#pragma once
#include <vector>
#include <imgui.h>
#include <imgui_hex_editor.h>

extern "C" {
    #include <common/int.h>
    #include <common/gl/input.h>
    #include "viewer/render_image.h"
}

typedef struct {
    u32 id;
    s32 size; // Sizes can actually be signed, not sure why.
    uintptr_t offset;

    // A chunk is "orphaned" if it can't be found using the offset table
    bool orphan;
}chunk_desc;

// State for 0x3 (transform matrix) window
struct window_state_0x3 {
    s32 selected_mat = 0;
    bool mat_slider = false;
};

// State for 0x10 chunk window
struct window_state_0x10 {
    u32 selected_atlas = 0;
    u32 selected_atlas_texture = 0;
};

// State for 0x15 texture window
struct window_state_0x15 {
    u32 selected_texture = 0;
};

// State for 0x16 chunk window
struct window_state_0x16 {
    u32 selected_vertex_buf = 0;
};

struct polaris {
    struct chunk {
        chunk(u32 init_id);

        // Try to only store POD data here that can be trivially
        // zero-initialized. Otherwise it's kind of a pain.
        union {
            window_state_0x3 window_0x3;
            window_state_0x10 window_0x10;
            window_state_0x15 window_0x15;
            window_state_0x16 window_0x16;
        };
        MemoryEditor hex_edit;

        u32 id = 0;
        s32 size = 0; // Sizes can actually be signed, not sure why.
        uintptr_t offset = 0;

        // A chunk is "orphaned" if it can't be found using the offset table
        bool orphan = false;
        bool active = false;

        void draw(polaris *pol);

        void chunk_0x2(polaris *pol);
        void chunk_0x3(polaris *pol);
        void chunk_0x10(polaris *pol);
        void chunk_0x11(polaris *pol);
        void chunk_0x15(polaris *pol);

        void dump_idx_buf(polaris *pol, FILE* out) const;
        void chunk_0x16(polaris *pol);
    };

    // State for the overall editor
    // Currently loaded ALR & metadata for all its chunks
    u8 *alr_data = nullptr;
    s64 alr_size = 0;
    std::vector<chunk> chunks;
    // Input state from the previous frame
    input_internal prev_input = {};
    // The texture currently being rendered in the background (buffpeep integration)
    img_state img_ctx = {0};

    void handle_input_suppression();
    static std::vector<chunk> shatter_alr(const u8* buf, s64 size) ;

    bool save_alr(const char *path);

    bool do_gui(GLFWwindow *window);
};