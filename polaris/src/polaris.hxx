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

// State for 0x2 (index buffer) window
struct window_state_0x2 {
    bool trust_alr_tri_count = false;
};

// State for 0x3 (armature) window
struct window_state_0x3 {
    s32 selected_joint = 0;
    bool slider = false;
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
    float scale_atlas = 1.0f;

    // Render settings for texture inside atlas
    bool use_actual_size = false;
    float scale = 1.0f;
};

// State for 0x15 texture window
struct window_state_0x15 {
    u32 selected_texture = 0;
    texture tex = {};
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

struct polaris {
    /// State for each ALR chunk
    struct chunk {
        chunk(u32 id, s32 size, uintptr_t offset) noexcept;

        // Try to only store primitive data here that can be trivially
        // zero-initialized. Otherwise it's kind of a pain.
        // TODO: See if we can use std::variant or inheritance to make it harder to call functions on the wrong chunk type
        union {
            window_state_0x2 window_0x2;
            window_state_0x3 window_0x3;
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
        void draw(polaris *pol) noexcept;

        // Dedicated editing windows for each chunk type
        void chunk_0x2(const polaris *pol) noexcept;
        void chunk_0x3(const polaris *pol) noexcept;
        void chunk_0x10(const polaris *pol) noexcept;
        void chunk_0x11(const polaris *pol) const noexcept;

        /// Replace the selected texture with a DDS file from disk, updating the
        /// metadata in the 0x15 chunk. Does nothing if not called on an 0x15 chunk.
        /// @param pol The rest of the program's state
        /// @param path The filepath of the DDS to load
        /// @param num_entries The number of texture entries in the 0x15 chunk
        /// @param entries Texture entries to be modified
        void import_dds_0x15(const polaris* pol, const char* path, u32 num_entries, texture_entry* entries);
        void chunk_0x15(polaris *pol) noexcept;

        /// @brief Save index buffer data from an 0x2 chunk into an OBJ file.
        ///
        /// @param pol The rest of the program's state
        /// @param out The output file to write to
        /// @param vert_entry Optional metadata about the vertex format. If
        /// present, extra checks occur to avoid saving invalid indices, and
        /// indices are formatted to use UVs if present. Otherwise, the indices
        /// are saved as-is.
        void dump_idx_buf(const polaris *pol, FILE* out, std::optional<vertbuf_entry> vert_entry = std::optional<vertbuf_entry>()) const noexcept;

        void dump_vertex_buf(const polaris *pol, const char* path, vertbuf_entry entry) const noexcept;
        void chunk_0x16(polaris *pol) noexcept;
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

    // If present, only display chunks with this ID
    std::optional<u32> chunk_filter;

    // Whether to show the ImGui Demo Window
    bool show_demo = false;

    // Input state from the previous frame
    input_internal prev_input = {};
    // The texture currently being rendered in the background (buffpeep integration)
    img_state img_ctx = {0};

    /// @brief Hide input from the rest of the program when ImGui is using it.
    void handle_input_suppression() noexcept;

    /// @brief "Shatter" an ALR into all its chunks
    ///
    /// This also modifies the resource buffer offset.
    /// @param buf The ALR data
    /// @param size The size of the ALR buffer
    /// @return List of chunks
    std::vector<chunk> shatter_alr(const u8* buf, s64 size) noexcept;

    /// @brief Overwrite the loaded ALR with a new one
    bool load_alr(const char* path) noexcept;

    /// @brief Save the ALR data in-memory to the specified path.
    bool save_alr(const char *path) const noexcept;

    void do_menu_bar() noexcept;

    /// @brief Render and update all the UI
    bool do_gui(GLFWwindow *window) noexcept;

    /// @brief Increase the amount of address space reserved for the ALR data
    void expand_reservation(s64 new_size) noexcept;

    polaris() noexcept;
};
