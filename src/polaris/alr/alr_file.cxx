#include "alr_file.hxx"
#include <common/file.h>
#include <common/vmem.h>
#include <formats/alr_animations.h>
#include "alr_dump.hxx"

namespace alr {

bool file::load(const char* path) noexcept {
    if (!file_exists(path)) {
        LOG_MSG(error, "I couldn't find an ALR file named \"%s\".\n", path);
        return false;
    }

    const s64 size = file_size(path);
    if (size < 8) {
        // Smallest possible ALR chunk is 8 bytes
        LOG_MSG(error, "\"%s\" is only %d bytes, but an ALR must be at least 8 bytes.\n", path, size);
        return false;
    }

    // Expand reservation if needed
    if (size > reserve_size) {
        // If our reservation needs resizing, we're dealing with a
        // truly massive file. Just add its size to the old size,
        // more space can never hurt.
        this->expand_reservation(reserve_size + size);
    }

    // Load the file into the buffer.
    if (!file_load_existing(path, data, size)) {
        // Some loading failure, an error message should've been printed
        return false;
    }
    alr_size = size;
    chunks = shatter_alr(data, alr_size);

    loaded = true;
    return true;
}

bool file::save(const char* path) const noexcept {
    FILE* out = fopen(path, "wb");
    if (out == nullptr) {
        return false;
    }

    bool result = true;
    if (fwrite(data, alr_size, 1, out) != 1) {
        // Incomplete write
        result = false;
    }
    fclose(out);

    return result;
}

    std::vector<file::chunk> file::shatter_alr(const u8* buf, s64 size) noexcept {
        // Technically we cast away const here, but we don't write any data so it's
        // fine.
        vfile vf = vfile_open((void*)buf, size);
        std::vector<file::chunk> out;

        // Loop until we exhaust the buffer or exit early
        u32 prev_id = -1;
        while (!vfile_eof(vf)) {
            // Read chunk data. We have to copy it over 1 field at a time because we
            // don't actually want/need any more of the chunk data.
            const uintptr_t offset = vf.pos; // It's important to save offset before reading
            const u32 id = VFILE_READ(u32, &vf);
            const s32 chunk_size = VFILE_READ(s32, &vf);
            file::chunk chunk(id, chunk_size, offset);

            if (chunk.id == 0 && prev_id == 0) {
                // There's never multiple consecutive chunks with ID 0. This means
                // we've hit an empty area, probably the end of the chunk data.
                break;
            }

            if (chunk.id == 0x11) {
                // This chunk has the resource buffer offset, save it for later
                // Our layout structure includes the id & size, so we have to seek
                // back for that...
                vf.pos -= sizeof(chunk_generic);
                chunk_layout layout = VFILE_READ(chunk_layout, &vf);
                resbuf_offset = layout.texbuf_offset;

                // Reset the position to what it used to be
                vf.pos -= sizeof(layout);
            }

            // Advance to the next chunk & add to output
            vf.pos = chunk.offset + chunk.size;
            out.push_back(chunk);
            prev_id = chunk.id; // Save current ID
        }

        return out;
    }

    file::chunk file::first_chunk_by_id(u32 id) const noexcept {
        return first_chunk_in_range(id, 0, alr_size);
    }

    file::chunk file::prev_chunk_by_id(u32 id, u32 high, u32 low) const noexcept {
        assert(low < high && "Low bound must be < high bound!");
        for (s64 i = chunks.size() - 1; i > 0; i--) {
            const chunk& c = chunks[i];
            if (high < c.offset) {
                continue; // Skip to starting offset
            }
            if (c.offset < low) {
                break; // We passed the low bound
            }
            if (c.id == id) {
                return c; // Found it!
            }
        }

        return chunk(0, 0, 0); // Nothin...
    }

    file::chunk file::first_chunk_in_range(u32 id, u32 low, u32 high) const noexcept {
        // TODO: Add an overload to find a chunk within an offset range. Since the list is sorted we can do a sort of binary search by starting @ the middle
        assert(low < high && "Low bound must be < high bound!");

        for (const auto & c : chunks) {
            if (high < c.offset) {
                break;
            }
            if (c.offset < low) {
                continue;
            }
            if (c.id == id) {
                return c; // Found it!
            }
        }

        return chunk(0, 0, 0); // Nothin...
    }

    u32 file::direct_insert_chunk(u32 offset, u32 size) noexcept {
        chunk empty(0, 0, 0);
        std::optional<chunk> overlap;
        std::optional<chunk> next = empty;

        for (const auto & c : chunks) {
            const u32 end_offset = c.offset + c.size;
            if (end_offset < offset) {
                continue; // The end of this chunk is still before our target
            } else if (c.offset < offset) {
                // This chunk starts before our target and ends after it
                overlap = c;
            } else {
                // The end and the start are after the target
                next = c;
                break;
            }
        }

        if (overlap.has_value()) {
            // Start the new chunk after the end of the conflicting one.
            offset = overlap->offset + overlap->size;
        }

        // If we're not at the end of the file now, the target offset should be
        // the start of a new chunk
        if (next.has_value()) {
            // Make room for the new chunk
            assert(offset == next->offset);
            shift_chunks(offset, size);
        }

        alr_size += size;
        return offset;
    }

    bool file::resize_chunk(u32 offset, s32 size_diff) noexcept {
        vfile vf = vfile_open(data + offset, sizeof(chunk_generic));
        chunk_generic* gen = VFILE_READ_PTR(chunk_generic, &vf);
        vf.size = gen->size;

        // Move the next chunk forward to make room
        const u32 next_offset = offset + gen->size;
        // This also fixes the size of this chunk
        const bool res = shift_chunks(next_offset, size_diff);
        if (!res) {
            return false;
        }

        // Update size & reparse the ALR
        chunks = shatter_alr(data, alr_size);
        return true;
    }

    bool file::set_chunk_size(u32 offset, s32 size) noexcept {
        assert(size >= 0 && "Size must be positive!");
        vfile vf = vfile_open(data + offset, sizeof(chunk_generic));
        chunk_generic* gen = VFILE_READ_PTR(chunk_generic, &vf);
        const s32 diff = (s32)size - gen->size;
        return resize_chunk(offset, diff);
    }

    bool file::shift_chunks(u32 begin_offset, s32 shift_amount) noexcept {
        const chunk last_chunk = chunks.back();
        const u32 end_of_chunks = last_chunk.offset + last_chunk.size;
        if (end_of_chunks + shift_amount >= resbuf_offset) {
            if (!shift_resource(0, shift_amount)) {
                return false;
            }
        }

        if (last_chunk.offset < begin_offset) {
            // This should be fine, there's just nothing that needs shifting
            LOG_MSG(warning, "Your starting offset %u is past the last chunk (offset %u)\n", begin_offset, last_chunk.offset);
            return true;
        }
        const u32 region_size = end_of_chunks - begin_offset;

        // Round beginning offset up to the next chunk in case it's off
        chunk last_before_shift = chunks[0];
        for (const auto& c : chunks) {
            if (c.offset < begin_offset) {
                last_before_shift = c;
                continue;
            }
            begin_offset = c.offset;
            break;
        }

        {
            // Adjust the previous chunk to avoid creating an invalid chunk
            vfile temp = vf_from_chunk(last_before_shift);
            auto* header = (chunk_generic*)vfile_cur(temp);
            if (shift_amount > 0) {
                header->size += shift_amount;
            }
        }

        void* source = data + begin_offset;
        void* target = (void*)(s64(source) + shift_amount);
        memmove(target, source, region_size);
        // Wipe the now unused space
        memset(source, 0, shift_amount);

        chunk header = chunks[0];
        assert(header.id == 0x11);
        vfile vf = vf_from_chunk(header);
        auto* layout = (chunk_layout*)vfile_cur(vf);
        for (u32 i = 0; i < layout->offset_array_size; i++) {
            if (layout->offsets[i] > begin_offset) {
                layout->offsets[i] += shift_amount;
            }
        }

        return true;
    }

    bool file::expand_resbuf(s32 amount) {
        alr_size += amount;
        if (alr_size > reserve_size) {
            LOG_MSG(error, "Unimplemented case: not enough reserved space to expand resource buffer.\n");
            return false;
        }

        vfile vf = vf_from_chunk(first_chunk_by_id(ALR_ID_HEADER));
        auto* header = (chunk_layout*) vfile_cur(vf);
        header->texbuf_size += amount;
        return true;
    }

    bool file::shift_resource(u32 data_offset, s32 shift_amount) noexcept {
        s64 remaining_size = alr_size - (resbuf_offset + data_offset);
        alr_size += shift_amount;
        if (alr_size > reserve_size) {
            LOG_MSG(error, "Unimplemented case: not enough reserved space to expand resource buffer.\n");
            return false;
        }

        u8* source = resource_buffer() + data_offset;
        u8* target = source + shift_amount;

        // Shift forward and wipe unused space
        memmove(target, source, remaining_size);
        memset(source, 0, shift_amount);

        // Adjust resource buffer offsets
        for (chunk c : chunks) {
            vfile vf = vf_from_chunk(c);
            if (c.id == ALR_ID_HEADER) {
                auto* header = (chunk_layout*) vfile_cur(vf);
                if (data_offset == 0) {
                    header->texbuf_offset += shift_amount;
                    resbuf_offset += shift_amount;
                    // Since the start has moved, the other offsets don't need to move.
                    break;
                } else {
                    header->texbuf_size += shift_amount;
                }
            }
            else if (c.id == ALR_ID_TEXTURE) {
                vfile_seek(&vf, sizeof(chunk_generic));
                const u32 num_entries = VFILE_READ(u32, &vf);
                auto* entries = (texture_entry*)vfile_cur(vf);
                for (u32 i = 0; i < num_entries; i++) {
                    if (entries[i].data_ptr >= data_offset) {
                        entries[i].data_ptr += shift_amount;
                    }
                }
            }
            else if (c.id == ALR_ID_MODEL) {
                vfile_seek(&vf, sizeof(chunk_generic));
                const u32 num_entries = VFILE_READ(u32, &vf);
                auto* entries = (vertbuf_entry*)vfile_cur(vf);
                for (u32 i = 0; i < num_entries; i++) {
                    if (entries[i].data_ptr >= data_offset) {
                        entries[i].data_ptr += shift_amount;
                    }
                }
            }
        }

        return true;
    }

    s32 file::first_model_idx(u32* num_models_out) const noexcept {
        vfile vf = vfile_open(data, alr_size);
        const auto* header = (chunk_layout*)vfile_cur(vf);

        s32 result = -1;
        for (s32 i = 0; i < header->offset_array_size; i++) {
            vf.pos = header->offsets[i];
            const auto* chunk = VFILE_READ_PTR(chunk_generic, &vf);
            // All models start with an 0x1 chunk
            if (chunk->id == 0x1) {
                result = i;
                break;
            }
        }

        if (num_models_out) {
            if (result >= 0) {
                *num_models_out = header->offset_array_size - result;
            } else {
                *num_models_out = 0;
            }
        }

        return result;
    }

    alr_model_desc file::model_at_idx(u32 idx) const noexcept {
        alr_model_desc out = {};

        const chunk header_chunk = chunks[0];
        if (header_chunk.id != 0x11) {
            LOG_MSG(error, "Header chunk ID != 0x11, something is seriously wrong!\n");
            return out;
        }

        // We cast away const because we won't be editing the ALR data at all.
        vfile vf = vfile_open(data, alr_size);
        vf.pos = header_chunk.offset;

        const auto* header = (chunk_layout*)vfile_cur(vf);
        u32 num_models = 0;
        const s32 first_model_idx = this->first_model_idx(&num_models);
        if (first_model_idx < 0 || num_models < 1) {
            LOG_MSG(error, "No models found!\n");
            return out;
        }

        if (idx >= num_models) {
            LOG_MSG(error, "Mesh index %d is out of bounds (max = %d)\n", idx, num_models);
            return out;
        }

        const s32 offset = header->offsets[first_model_idx + idx];
        if (offset < 0) {
            LOG_MSG(error, "Mesh index %d doesn't exist (negative offset %d)\n", idx, offset);
            return out;
        }
        vf.pos = offset;

        out.mat_chunk = (material_header*)vfile_cur(vf);
        vfile_seek(&vf, out.mat_chunk->size);

        out.skel_chunk = (chunk_armature*)vfile_cur(vf);
        vfile_seek(&vf, out.skel_chunk->size);

        out.vert_chunk = (vertbuf_header*)vfile_cur(vf);
        vfile_seek(&vf, out.vert_chunk->size);

        out.idx_chunk = (idxbuf_header*)vfile_cur(vf);
        vfile_seek(&vf, out.idx_chunk->size);
        if (out.idx_chunk->id == 0x13) {
            // Sometimes this chunk appears before the index buffers, not sure
            // what it does. Skip it.
            out.idx_chunk = (idxbuf_header*)vfile_cur(vf);
            vfile_seek(&vf, out.idx_chunk->size);
        }

        bool bad_id = (out.idx_chunk->id != 0x2 || out.vert_chunk->id != 0x16 ||
                      out.skel_chunk->id != 0x3 || out.mat_chunk->id != 0x1);
        if (bad_id) {
            // This happens with some empty models. Return all NULL.
            out = {};
        }

        return out;
    }

    s32 file::animation_by_idx(u32 idx, u32 joint_idx) const noexcept {
        vfile vf = vfile_open(data, alr_size);

        const auto* layout = VFILE_READ_PTR(chunk_layout, &vf);

        const bool invalid_anim_id = (idx >= ALR_NUM_PLAYER_ANIMATIONS);
        const bool no_anims = (layout->offset_array_size < ALR_NUM_PLAYER_ANIMATIONS);
        const bool out_of_bounds = (layout->offset_array_size <= idx);
        if (invalid_anim_id || no_anims || out_of_bounds) {
            return -1;
        }

        s32 offset = layout->offsets[idx];
        if (offset < 0) {
            // Animation doesn't exist
            return -1;
        }

        vf.pos = offset;
        while (true) {
            s32 cur_offset = vf.pos;
            auto* animation_header = VFILE_READ_PTR(anim_header, &vf);
            if (animation_header->id != 0x5) {
                break; // We hit the end of the animation
            }

            if (animation_header->joint_idx == joint_idx) {
                return cur_offset;
            }
            vf.pos = cur_offset + animation_header->size;
        }

        return -1;
    }

    mat4s file::anim_xform_for_joint(u32 anim_id, s32 joint_idx, float cur_frame) const noexcept {
        s32 anim_offset = animation_by_idx(anim_id, joint_idx);
        if (anim_offset <= 0) {
            return GLMS_MAT4_IDENTITY_INIT;
        }

        vfile afile = vfile_open(data, alr_size);
        vfile_seek(&afile, anim_offset);
        const auto* aheader = VFILE_READ_PTR(anim_header, &afile);
        cur_frame = fmodf(cur_frame, aheader->length);

        vec3s position = {};
        vec3s rotation = {};
        const u32 transkey_offset = afile.pos;
        for (u32 i = 0; i < aheader->translation_key_count; i++) {
            float frame = 0.0f;
            const u8* keydata = (const u8*)vfile_cur(afile);
            vec3s key = alr::anim_read_key(keydata, aheader->translation_key_size, frame);
            if (frame > cur_frame) {
                break;
            }
            vfile_seek(&afile, aheader->translation_key_size);

            position = key;
        }
        // Skip all translation keys
        afile.pos = transkey_offset + (aheader->translation_key_count * aheader->translation_key_size);

        const u32 rotkey_offset = afile.pos;
        for (u32 i = 0; i < aheader->rotation_key_count; i++) {
            float frame = 0.0f;
            const u8* keydata = (const u8*)vfile_cur(afile);
            vec3s key = alr::anim_read_key(keydata, aheader->rotation_key_size, frame);
            if (frame > cur_frame) {
                break;
            }
            vfile_seek(&afile, aheader->rotation_key_size);

            rotation = key;
        }
        // Skip all rotation keys
        afile.pos = rotkey_offset + (aheader->rotation_key_count * aheader->rotation_key_size);

        mat4s rot_xform = glms_euler_zyx(rotation);
        mat4s pos_xform = glms_translate_make(position);
        mat4s anim_xform = glms_mat4_mul(pos_xform, rot_xform);
        return rot_xform;
    }

    mat4s file::joint_final_xform(const chunk_armature* joint_header, u32 anim_id, s32 joint_idx, float cur_frame) const noexcept {
        // We're going to use J1->J2 to mean "J2 is J1's parent".
        // If we have 3 joints J0->J1->J2, with matching animation transforms
        // A0 / A1 / A2, then the final transform for J0 is:
        //     (J2 * A2) * (J1 * A1) * (J0 * A0)
        const joint_t* joint = &joint_header->joints[joint_idx];
        mat4s obj_transform = GLMS_MAT4_IDENTITY_INIT;
        do {
            mat4s joint_xform = alr::transform_from_joint(*joint);
            mat4s anim_xform = anim_xform_for_joint(anim_id, joint_idx, cur_frame);

            // HACK: If there's an animation for this joint, discard joint rotation to fix broken limbs.
            mat4s identity = GLMS_MAT4_IDENTITY_INIT;
            if (memcmp(identity.raw, anim_xform.raw, sizeof(identity)) != 0) {
                vec4s translation = {};
                mat4s rot_xform = {};
                vec3s scale = {};
                glms_decompose(joint_xform, &translation, &rot_xform, &scale);
                joint_xform = glms_translate_make(glms_vec3(translation));
            }

            joint_xform = glms_mat4_mul(joint_xform, anim_xform);
            obj_transform = glms_mat4_mul(joint_xform, obj_transform);
            if (joint->parent_idx < 0 || joint->parent_idx >= joint_header->joint_count) {
                break;
            }
            joint_idx = joint->parent_idx;
            joint = &joint_header->joints[joint_idx];
        } while (true);

        return obj_transform;
    }

    void file::expand_reservation(s64 new_size) noexcept {
        if (new_size < reserve_size) {
            LOG_MSG(error, "No reason to shrink reservation from 0x%X -> 0x%X, ignoring!\n", reserve_size, new_size);
            return;
        }

        u8* new_buf = (u8*)vmem_reserve(new_size);
        if (new_buf == nullptr) {
            return;
        }

        // Commit the entire region. This ensures it's all accessible, but doesn't
        // comsume any physical memory until accessed. (May still create page file
        // entries)
        vmem_commit(new_buf, new_size);

        // Free old buffer and update our state
        if (data != nullptr) {
            vmem_free(data, reserve_size);
        }
        data = new_buf;
        reserve_size = new_size;
    }

    file::file() noexcept {
        // "Expand" our reservation from 0 bytes to... not 0.
        this->expand_reservation(reserve_size);
    }

    file::~file() noexcept {
        vmem_free(data, reserve_size);
    }
} // namespace al
