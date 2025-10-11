#pragma once
#include <vector>
#include <optional>

#include <imgui.h>
#include <imgui_hex_editor.h>

#include <common/image.h>
#include <common/int.h>

#include <formats/alr.h>

#include "alr_texture.hxx"
#include "common/vfile.h"

namespace al {

class resource {
public:
    /// State for each ALR chunk
    struct chunk {
        chunk(u32 id, s32 size, uintptr_t offset) noexcept;

        /// The ID determines what data the chunk should contain
        u32 id = 0;

        s32 size = 0; // Sizes are sometimes negative, not sure why.

        /// The location of this chunk in the ALR file
        uintptr_t offset = 0;
    };

    enum alr_data_type {
        TYPE_TEXTURE,
        TYPE_MESH,
        TYPE_ANIMATION,
    };

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
    u32 selected_tex = 0;
    u32 selected_mesh = 0;
    alr_data_type active_type = TYPE_TEXTURE;

    bool tex_auto_scale = false;
    float tex_manual_scale = 1.0f;

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

    // Draw a view of the file's contents in a tree-like structure
    void draw_file() noexcept;

    // Draw the contents of the currently selected data within the file
    void draw() noexcept;

    u8* resource_buffer() const noexcept {
        return this->data + this->resbuf_offset;
    }

    vfile vf_from_chunk(chunk c, bool skip_generic = false) {
        vfile vf = vfile_open(data + c.offset, MIN(c.size, alr_size - c.offset));
        if (skip_generic) {
            vfile_seek(&vf, sizeof(chunk_generic));
        }

        return vf;
    }

    void expand_reservation(s64 new_size) noexcept;
    resource() noexcept;
    ~resource() noexcept;
};

} // namespace al
