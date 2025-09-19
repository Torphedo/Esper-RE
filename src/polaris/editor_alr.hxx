#pragma once
#include <vector>
#include <optional>

#include <imgui.h>
#include <imgui_hex_editor.h>

#include <common/image.h>
#include <common/int.h>

#include <formats/alr.h>

#include "alr_texture.hxx"

// State for 0x1 (index buffer) window
struct window_state_0x1 {
    u32 selected_entry = 0;
};

// State for 0x2 (index buffer) window
struct window_state_0x2 {
};

// State for 0x3 (armature) window
struct window_state_0x3 {
    s32 selected_joint = 0;
    bool slider = false;
};

// State for 0x5 (animation) window
struct window_state_0x5 {
};

// State for 0x10 chunk window
struct window_state_0x10 {
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
struct window_state_0x15 {
    u32 selected_texture = 0;
    texture tex;
    gl_obj gl_tex_id = 0;

    // User can choose to render the texture at its real size, or scaled up
    bool use_actual_size = false;
    float scale = 1.0f;
};

// State for 0x16 chunk window
struct window_state_0x16 {
    u32 selected_vertex_buf = 0;
    MemoryEditor hex_vertbuf;
};

struct viewport_t;

namespace al {

class resource {
public:
    /// State for each ALR chunk
    struct chunk {
        chunk(u32 id, s32 size, uintptr_t offset) noexcept;

        // Try to only store primitive data here that can be trivially
        // zero-initialized. Otherwise it's kind of a pain.
        // TODO: See if we can use std::variant or inheritance to make it harder to call functions on the wrong chunk type
        union {
            window_state_0x1 window_0x1;
            window_state_0x2 window_0x2;
            window_state_0x3 window_0x3;
            window_state_0x5 window_0x5;
            window_state_0x10 window_0x10;
            window_state_0x15 window_0x15;
            window_state_0x16 window_0x16;
        };
        // Hex editor used by specialized editors of each chunk
        MemoryEditor hex_edit;

        // Generic hex editor used in every chunk's draw function
        MemoryEditor hex_chunk;

        /// The ID determines what data the chunk should contain
        u32 id = 0;

        s32 size = 0; // Sizes are sometimes negative, not sure why.

        /// The location of this chunk in the ALR file
        uintptr_t offset = 0;

        /// A chunk is "orphaned" if it can't be found using the offset table
        bool orphan = false;

        /// Whether to show this chunk's editing window
        bool active = false;

        /// Render and update the chunk's editing window.
        /// This always draws, and doesn't check the @ref active flag
        void draw(al::resource& alr, viewport_t& viewport) noexcept;

        // Dedicated editing windows for each chunk type
        void chunk_0x1(const al::resource& alr, viewport_t& viewport) noexcept;
        void chunk_0x2(const al::resource& alr, viewport_t& viewport) noexcept;
        void chunk_0x3(const al::resource& alr, viewport_t& viewport) noexcept;
        void chunk_0x5(const al::resource& alr, viewport_t& viewport) noexcept;
        void chunk_0x7(const al::resource& alr, viewport_t& viewport) noexcept;
        void chunk_0x10(al::resource& alr, viewport_t& viewport) noexcept;
        void chunk_0x11(const al::resource& alr, viewport_t& viewport) const noexcept;

        /// Replace the selected texture with a DDS file from disk, updating the
        /// metadata in the 0x15 chunk. Does nothing if not called on an 0x15 chunk.
        /// @param alr The rest of the program's state
        /// @param path The filepath of the DDS to load
        /// @param num_entries The number of texture entries in the 0x15 chunk
        /// @param entries Texture entries to be modified
        void import_dds_0x15(const al::resource& alr, const char* path, u32 num_entries, texture_entry* entries) noexcept;
        void chunk_0x15(al::resource& alr, viewport_t& viewport) noexcept;

        void send_vertbuf_to_viewport(al::resource& alr, viewport_t& viewport) noexcept;
        void chunk_0x16(al::resource& alr, viewport_t& viewport) noexcept;
    };

    // State for texture editor, which pulls information from 0x15 and 0x16 chunks
    struct tex_edit_state_t {
        // Status of texture export popup
        bool tex_export_active = false;
        texture export_cfg = {};
        u32 offset_0x15 = 0;
        u32 offset_0x10 = 0;
        u32 export_tex_idx = 0; // Index of texture entry we're exporting
        bool override_buf = 0;

        void draw(resource& alr) noexcept;
    };

    tex_edit_state_t tex_edit;

    // Currently loaded ALR & metadata for all its chunks
    u8* data = nullptr;
    s64 alr_size = 0;
    ptrdiff_t resbuf_offset = 0;

    // If we guess the texture format wrong, we might accidentally read beyond
    // the filesize. Because a mistake will inevitably happen, we reserve a
    // large chunk of address space to avoid crashes in this case.
    s64 reserve_size = 1024 * 1024 * 32;
    std::vector<chunk> chunks;

    texture_manager tex_manager;

    // If present, only display chunks with this ID
    std::optional<u32> chunk_filter;

    /// @brief Overwrite the loaded ALR with a new one
    bool load(const char* path) noexcept;

    /// @brief Save the ALR data in-memory to the specified path.
    bool save(const char* path) const noexcept;

    /// @brief "Shatter" an ALR into all its chunks
    ///
    /// This also modifies the resource buffer offset.
    /// @param buf The ALR data
    /// @param size The size of the ALR buffer
    /// @return List of chunks
    std::vector<chunk> shatter_alr(const u8* buf, s64 size) noexcept;

    [[nodiscard]] chunk first_chunk_by_id(u32 id) const noexcept;
    // Search backwards from an offset to find a chunk
    [[nodiscard]] chunk prev_chunk_by_id(u32 id, u32 high, u32 low = 0) const noexcept;
    [[nodiscard]] chunk first_chunk_in_range(u32 id, u32 low, u32 high) const noexcept;

    void draw(viewport_t& viewport) noexcept;

    u8* resource_buffer() const noexcept {
        return this->data + this->resbuf_offset;
    }

    void expand_reservation(s64 new_size) noexcept;
    resource() noexcept;
    ~resource() noexcept;
};

} // namespace al
