#pragma once
#include <vector>
#include <optional>
#include <imgui.h>
#include <imgui_hex_editor.h>

extern "C" {
    #include <common/int.h>
    #include <common/image.h>
    #include <common/gl/input.h>
    #include <formats/alr.h>
    #include "viewer/render_image.h"
}

// State for 0x3 (transform matrix) window
struct window_state_0x3 {
    s32 selected_mat = 0;
    bool mat_slider = false;
};

// State for 0x10 chunk window
struct window_state_0x10 {
    u32 selected_atlas = 0;
    u32 selected_atlas_texture = 0;
    texture tex = {};
    gl_obj gl_tex_id = 0;

    // User can choose to render the texture at its real size, or scaled up
    // Render settings for entire atlas
    bool use_actual_size_atlas = false;
    float scale_atlas = 0;

    // Render settings for texture inside atlas
    bool use_actual_size = false;
    float scale = 0;
};

// State for 0x15 texture window
struct window_state_0x15 {
    u32 selected_texture = 0;
    texture tex = {};
    gl_obj gl_tex_id = 0;

    // User can choose to render the texture at its real size, or scaled up
    bool use_actual_size = false;
    float scale = 0;
};

// State for 0x16 chunk window
struct window_state_0x16 {
    u32 selected_vertex_buf = 0;
    MemoryEditor hex_vertbuf;
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

        void import_dds_0x15(polaris* pol, const char* path, u32 num_entries, resource_entry* entries);
        void chunk_0x15(polaris *pol);

        void dump_idx_buf(polaris *pol, FILE* out, std::optional<resource_entry_0x16> vert_entry = std::optional<resource_entry_0x16>()) const;
        void chunk_0x16(polaris *pol);
    };

    // State for the overall editor
    // Currently loaded ALR & metadata for all its chunks
    u8 *alr_data = nullptr;
    s64 alr_size = 0;
    ptrdiff_t resbuf_offset = 0;

    // If we guess the texture format wrong, we might accidentally read beyond
    // the filesize. Because a mistake will inevitably happen, we reserve a
    // large chunk of address space to avoid crashes in this case.
    s64 reserve_size = 1024 * 1024 * 32;
    std::vector<chunk> chunks;
    // Input state from the previous frame
    input_internal prev_input = {};
    // The texture currently being rendered in the background (buffpeep integration)
    img_state img_ctx = {0};

    polaris() noexcept;
    void handle_input_suppression() noexcept;

    /// @brief "Shatter" an ALR into all its chunks
    ///
    /// This also modifies the resource buffer offset.
    /// @param buf The ALR data
    /// @param size The size of the ALR buffer
    /// @return List of chunks
    std::vector<chunk> shatter_alr(const u8* buf, s64 size) noexcept;

    bool save_alr(const char *path) const noexcept;

    bool do_gui(GLFWwindow *window) noexcept;

    void expand_reservation(s64 new_size) noexcept;
};