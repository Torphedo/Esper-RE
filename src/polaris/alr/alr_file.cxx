#include "alr_file.hxx"
#include <string.h>
#include <common/file.h>
#include <common/vmem.h>

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

    tex_manager.destroy(); // Clear texture cache
    tex_manager.atlasheader_offset = first_chunk_by_id(0x10).offset;
    tex_manager.texheader_offset = first_chunk_by_id(0x15).offset;
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

    bool file::shift_chunks(u32 begin_offset, s32 shift_amount) noexcept {
        const chunk last_chunk = chunks.back();
        const u32 end_of_chunks = last_chunk.offset + last_chunk.size;
        if (end_of_chunks + shift_amount >= resbuf_offset) {
            if (!shift_vertbuf(0, shift_amount)) {
                return false;
            }
        }

        if (last_chunk.offset < begin_offset) {
            LOG_MSG(error, "Your starting offset %u is past the last chunk (offset %u)\n", begin_offset, last_chunk.offset);
            return false;
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

    bool file::shift_vertbuf(u32 data_offset, s32 shift_amount) noexcept {
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
            if (c.id == 0x11) {
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
            else if (c.id == 0x15) {
                vfile_seek(&vf, sizeof(chunk_generic));
                const u32 num_entries = VFILE_READ(u32, &vf);
                auto* entries = (texture_entry*)vfile_cur(vf);
                for (u32 i = 0; i < num_entries; i++) {
                    if (entries[i].data_ptr >= data_offset) {
                        entries[i].data_ptr += shift_amount;
                    }
                }
            }
            else if (c.id == 0x16) {
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

    s32 file::first_model_idx() const noexcept {
        vfile vf = vfile_open(data, alr_size);

        const auto* header = (chunk_layout*)vfile_cur(vf);
        for (u32 i = 0; i < header->offset_array_size; i++) {
            vf.pos = header->offsets[i];
            const auto* chunk = VFILE_READ_PTR(chunk_generic, &vf);
            // All models start with an 0x1 chunk
            if (chunk->id == 0x1) {
                return i;
            }
        }

        return -1;
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
