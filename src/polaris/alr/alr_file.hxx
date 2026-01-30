#pragma once
#include <vector>
#include <formats/alr.h>
#include <gui/alr_assets.hxx>

typedef struct {
    chunk_0x1_header* mat_chunk; // Materials
    chunk_armature*  skel_chunk;
    idxbuf_header*   idx_chunk;
}alr_model_desc;

namespace alr {
    class file {
    public:
        /// State for each ALR chunk
        struct chunk {
            chunk(u32 id, s32 size, uintptr_t offset) noexcept
            : id(id), size(size), offset(offset) {

            }

            /// The ID determines what data the chunk should contain
            u32 id = 0;

            s32 size = 0; // Sizes are sometimes negative, not sure why.

            /// The location of this chunk in the ALR file
            uintptr_t offset = 0;
        };

        // Currently loaded ALR & metadata for all its chunks
        u8* data = nullptr;
        s64 alr_size = 0;
        ptrdiff_t resbuf_offset = 0;
        bool loaded = false;

        // If we guess the texture format wrong, we might accidentally read beyond
        // the filesize. Because a mistake will inevitably happen, we reserve a
        // large chunk of address space to avoid crashes in this case.
        s64 reserve_size = 1024 * 1024 * 32;
        std::vector<chunk> chunks;
        texture_manager tex_manager;

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

        /// @brief Shift all chunks at/after the starting offset forward.
        ///
        /// This also fixes some offsets in the header to account for the change.
        /// This function will fail if the resource buffer gets in the way.
        bool shift_chunks(u32 begin_offset, s32 shift_amount) noexcept;

        /// @brief Shift a vertex buffer forwards by some amount
        ///
        /// @param data_offset Offset of the vertex buffer within the larger resource buffer
        /// @param shift_amount The amount to shift forward by
        bool shift_vertbuf(u32 data_offset, s32 shift_amount) noexcept;

        /// Find the index of the first offset in the header that points to a model
        /// @return Index, or -1 if none are found
        s32 first_model_idx() const noexcept;

        u8* resource_buffer() const noexcept {
            return this->data + this->resbuf_offset;
        }

        vfile vf_from_chunk(chunk c) noexcept {
            return vfile_open(this->data + c.offset, c.size);
        }

        void expand_reservation(s64 new_size) noexcept;
        file() noexcept;
        ~file() noexcept;
    };

} // namespace al
