#pragma once
#include <vector>

#include <cglm/struct.h>
#include <common/vfile.h>

#include <formats/alr.h>

typedef struct {
    material_header* mat_chunk; // Materials
    chunk_armature*  skel_chunk;
    vertbuf_header*  vert_chunk;
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

        /// Add a chunk in the middle of the ALR.
        ///
        /// This function doesn't do any initialization or validation, it only
        /// makes room for the new chunk. The ALR size is adjusted, but the
        /// chunk list is not regenerated (you must do this yourself later)
        /// @param offset The approximate offset of the new chunk. If it points
        /// to the middle of another chunk, the new chunk will be placed at the
        /// end of it instead.
        /// @param size The size of the new chunk
        /// @return The offset the chunk was actually placed at
        u32 direct_insert_chunk(u32 offset, u32 size) noexcept;

        /// @brief Adjust the size of a chunk by some amount
        ///
        /// This function reparses the ALR, so you should avoid calling it many
        /// times in a row if you can avoid it.
        /// @param offset The offset of the chunk to be resized
        /// @param size_diff The amount to change the size by
        bool resize_chunk(u32 offset, s32 size_diff) noexcept;

        /// @brief Set the size of a chunk
        ///
        /// This function calculates the difference in size and then uses
        /// @ref resize_chunk().
        /// That function reparses the ALR, so you should avoid calling this
        /// many times in a row if you can avoid it.
        /// @param offset The offset of the chunk to be resized
        /// @param size The new size
        bool set_chunk_size(u32 offset, s32 size) noexcept;

        /// @brief Shift all chunks at/after the starting offset forward.
        ///
        /// This also fixes some offsets in the header to account for the change.
        /// This function will fail if the resource buffer gets in the way.
        bool shift_chunks(u32 begin_offset, s32 shift_amount) noexcept;

        /// @brief Shift a vertex buffer or texture forwards by some amount
        ///
        /// @param data_offset Offset of the resource within the larger resource buffer
        /// @param shift_amount The amount to shift forward by
        bool shift_resource(u32 data_offset, s32 shift_amount) noexcept;

        /// @brief Expand the resource buffer by some amount
        ///
        /// If the value is negative, the resource buffer shrinks.
        bool expand_resbuf(s32 amount);

        /// @brief Add a new texture to the file
        ///
        /// This function expands the resource buffer and initializes the new
        /// texture to a small all-black image.
        /// @param texChunkOffset The offset of the texture (0x15) chunk.
        /// It will be found automatically if you give a negative offset.
        bool add_new_texture(s32 texChunkOffset = -1) noexcept;

        /// Find the index of the first offset in the header that points to a model
        /// @param Optional output argument to receive the number of models present
        /// @return Index, or -1 if none are found
        s32 first_model_idx(u32* num_models_out = nullptr) const noexcept;

        alr_model_desc model_at_idx(u32 idx) const noexcept;

        /// Find the animation chunk for a specific joint in a specific animation
        /// @param alr ALR to parse
        /// @param idx Internal animation ID
        /// @param joint_idx Index of the joint to find the animation for
        /// @return Offset to the animation chunk, or -1 on failure
        s32 animation_by_idx(u32 idx, u32 joint_idx) const noexcept;

        /// Calculate a joint's animation transform at a specific frame of an animation
        /// @param anim_id Internal animation ID (within the ALR)
        /// @param joint_idx Index of joint being animated
        /// @param cur_frame Current animation frame
        /// @return Transform to right-multiply with joint transform
        mat4s anim_xform_for_joint(u32 anim_id, s32 joint_idx, float cur_frame) const noexcept;

        /// @brief Calculate the final animated transform of a joint
        ///
        /// This function applies all parent transforms with their corresponding
        /// animation transforms.
        /// @param joint_header Skeleton data
        /// @param anim_id Animation to play
        /// @param joint_idx The joint whose transform will be calculated
        /// @param cur_frame The current animation frame
        /// @return Final local space transform of the animated joint
        mat4s joint_final_xform(const chunk_armature* joint_header, u32 anim_id, s32 joint_idx, float cur_frame) const noexcept;


        u8* resource_buffer() const noexcept {
            return this->data + this->resbuf_offset;
        }

        u32 resource_buffer_size() const noexcept {
            return alr_size - resbuf_offset;
        }

        vfile vf_from_chunk(chunk c) noexcept {
            return vfile_open(this->data + c.offset, c.size);
        }

        void expand_reservation(s64 new_size) noexcept;
        file() noexcept;
        ~file() noexcept;
    };

} // namespace al
