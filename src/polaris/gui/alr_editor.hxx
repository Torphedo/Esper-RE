#pragma once
#include <imgui.h>
#include <imgui_hex_editor.h>

#include <common/image.h>
#include <common/int.h>
#include <common/vfile.h>

#include <formats/alr.h>

#include <alr/alr_file.hxx>
#include <gui/alr_assets.hxx>

// State for material (ID 0x1) window
struct window_state_material {
    u32 selected_entry = 0;
};

// State for 0x3 (armature) window
struct window_state_skeleton {
    s32 selected_joint = 0;
    bool slider = false;
};

// State for 0x10 chunk window
struct window_state_atlas {
    u32 selected_atlas = 0;
    u32 selected_atlas_texture = 0;
    gl_obj gl_tex_id = 0;
    texture tex;

    // User can choose to render the texture at its real size, or scaled up
    // Render settings for entire atlas
    bool use_actual_size_atlas = false;
    float scale_atlas = 1.0f;

    // Render settings for texture inside atlas
    bool use_actual_size = false;
    float scale = 1.0f;
};

// State for 0x15 texture window
struct window_state_texture {
    u32 selected_texture = 0;
    gl_obj gl_tex_id = 0;

    // User can choose to render the texture at its real size, or scaled up
    bool use_actual_size = false;
    float scale = 1.0f;
};

// State for 0x16 chunk window
struct window_state_vertbuf {
    u32 selected_vertex_buf = 0;
    MemoryEditor hex_vertbuf;
    s32 shift_amount = 0;
};

struct viewport_t;

namespace alr {

class editor {
public:

    struct window_state {
        // Try to only store primitive data here that can be trivially
        // zero-initialized. Otherwise it's kind of a pain.
        // TODO: See if we can use std::variant or inheritance to make it harder to call functions on the wrong chunk type
        union {
            window_state_material win_material;
            window_state_skeleton win_skel;
            window_state_atlas win_atlas;
            window_state_texture win_texture;
            window_state_vertbuf window_vertbuf;
        };
        // Hex editor used by specialized editors of each chunk
        MemoryEditor hex_edit;

        // Generic hex editor used in every chunk's draw function
        MemoryEditor hex_chunk;

        // Index of the chunk in the file / ALR chunk vector
        u32 chunk_idx;

        /// Whether to show this chunk's editing window
        bool active = true;

        void draw(editor& ed, viewport_t& viewport) noexcept;
        void draw_chunk_material(const file& alr, file::chunk& chunk) noexcept;
        void draw_chunk_idxbuf(file& alr, file::chunk& chunk) noexcept;
        void draw_chunk_skeleton(const file& alr, file::chunk& chunk) noexcept;
        void draw_chunk_animation(const file& alr, file::chunk& chunk) noexcept;
        void draw_chunk_cam_anim(const file& alr, file::chunk& chunk) noexcept;
        void draw_chunk_atlas(file& alr, file::chunk& chunk) noexcept;
        void draw_chunk_header(const file& alr, file::chunk& chunk) const noexcept;
        void draw_chunk_texture(editor& ed, file::chunk& chunk) noexcept;
        void draw_chunk_vertbuf(file& alr, file::chunk& chunk, viewport_t& viewport) noexcept;

        void import_dds_0x15(file& alr, const char* path, u32 num_entries, texture_entry* entries) noexcept;
        void send_vertbuf_to_viewport(file& alr, viewport_t& viewport) noexcept;

        window_state();
        window_state(u32 chunk_idx, u32 chunk_id);
    };

    alr::file alr;

    // Per-chunk UI state
    std::vector<window_state> states;

    // State for texture editor, which pulls information from 0x15 and 0x16 chunks
    struct tex_edit_state_t {
        // Status of texture export popup
        bool tex_export_active = false;
        texture export_cfg = {};
        u32 offset_texture = 0;
        u32 offset_atlas = 0;
        u32 export_tex_idx = 0; // Index of texture entry we're exporting
        bool guess_atlas = true;
        bool override_buf = false;

        void draw(file& alr) noexcept;
    };

    tex_edit_state_t tex_edit;

    // If present, only display chunks with this ID
    std::optional<u32> chunk_filter;
    bool graphics_initialized = false;

    void draw(viewport_t& viewport) noexcept;
    bool load(const char* path, viewport_t& viewport) noexcept;
    void send_all_to_viewport(viewport_t& viewport) const noexcept;
};

} // namespace al
