#include "validation.hxx"

#include <common/vfile.h>

#include "scope_timer.hxx"
#include "imgui_utils.hxx"

bool alr_validate(std::string& msg, const polaris& pol) noexcept {
    const scope_timer draw_timer(pol.timer_map, "polaris_validate");

    bool result = true;
    std::optional<al::resource::chunk> header_chunk;
    for (const al::resource::chunk& chunk : pol.alr.chunks) {
        result &= alr_chunk_validate(pol.alr, chunk, msg, pol.headless);
        if (chunk.id == 0x11) {
            header_chunk = chunk;
        }
    }

    // We expect that the header chunk is always present
    if (!header_chunk.has_value()) {
        str_format_append(msg, "ALR header is missing!\n");
        return false;
    }

    if (header_chunk->offset != 0) {
        str_format_append(msg, "Expected header @ offset 0 [found @ 0x%x]!\n", header_chunk->offset);
        result = false;
    }

    // Get vfile for header offsets
    vfile alr_vf = vfile_open(pol.alr.data, pol.alr.alr_size);
    vfile header_vf = alr_vf;
    const auto header = VFILE_READ(chunk_layout, &header_vf);
    const s32* offsets = (s32*)vfile_cur(header_vf);

    const s64 size_mismatch = pol.alr.alr_size - (header.texbuf_offset + header.texbuf_size);
    if (size_mismatch < 0) {
        str_format_append(msg,
                          "Header claims resbuf is 0x%X bytes @ 0x%X, but ALR is only 0x%X bytes (off by 0x%X)\n",
                          header.texbuf_size, header.texbuf_offset, pol.alr.alr_size, abs(size_mismatch));
        result = false;
    } else if (size_mismatch > 0) {
        str_format_append(msg,
                          "Header claims resbuf is 0x%X bytes @ 0x%X, leaving 0x%X bytes extra\n",
                          header.texbuf_size, header.texbuf_offset, size_mismatch);
        result = false;
    }

    s32 prev_offset = offsets[0];
    u32 chunk_idx = 0;

    for (u32 i = 1; i < header.offset_array_size; i++) {
        const s32 cur_offset = offsets[i];
        if (cur_offset < 0) {
            str_format_append(msg, "Header offset #%d is negative [%d]!\n", i, cur_offset);
            continue;
        }

        // The first 0x00 chunk we find after the previous offset.
        std::optional<al::resource::chunk> terminator;
        // The last chunk before we hit the current offset
        al::resource::chunk last(0xFF, 0, 0);

        // Skip up to the last chunk before the current offset
        while (chunk_idx < pol.alr.chunks.size()) {
            const al::resource::chunk chunk = pol.alr.chunks.at(chunk_idx);

            // Save the first 0x00 chunk we find
            if (chunk.id == 0x00 && !terminator.has_value()) {
                terminator = chunk;
            }

            // We've hit the current offset
            if (chunk.offset >= cur_offset) {
                break;
            }

            // If we got here, this chunk is still before the current offset.
            last = chunk;
            chunk_idx++;
        }

        // Unless our assumptions break or the file is wrong, the terminator
        // should always be the last chunk.
        if (!terminator.has_value()) {
            str_format_append(msg, "Chunk series @ offset 0x%x missing a null terminator!\n", cur_offset);
            result = false;
        }
        else if (last.offset != terminator->offset) {
            str_format_append(msg, "Chunk series @ offset 0x%x has terminator @ 0x%x, but last chunk @ 0x%x!\n", cur_offset, terminator->offset, last.offset);
            result = false;
        }

        // Update previous offset
        prev_offset = cur_offset;
    }

    // Verify that the offset table is in order (aside from negative entries)
    s32 temp = -1;
    for (s32 i = 0; i < header.offset_array_size; i++) {
        const s32 offset = offsets[i];
        if (offset <= temp) {
            str_format_append(msg, "Offset %d [0x%x] <= offset %d [0x%x]\n", i, offset, i - 1, temp);
            result = false;
        }
        temp = offset;
    }

    return result;
}

bool validate_entry_sizes(std::string& msg, u32 total_size, u32 header_size, u32 num_entries, u32 entry_size) {
    const u32 estimated_num_entries = (total_size - header_size) / entry_size;
    if (estimated_num_entries != num_entries) {
        str_format_append(msg, "Entry count seems to be wrong!");

        // Some ALRs (like boss03b & boss01) replace the texture count field
        // with a size in bytes, fairly close to the chunk size. I'm not sure
        // why they do this, but it can be accounted for. - torph
        const s64 size_diff = (s64)num_entries - (s64)total_size;
        if (abs(size_diff) < 100) {
            str_format_append(msg, "What was supposed to be an entry count looks to be a size in bytes.");
        }
        return false;
    }

    return true;
}

// Assumes a variable std::string& msg exists, which the formatted message is
// added to if the condition fails.
#define AL_ASSERT(cond, ...) result = (cond) ? result : (str_format_append(msg, __VA_ARGS__), false)

bool alr_chunk_validate(const al::resource& alr, const al::resource::chunk& chunk, std::string& msg, bool headless) noexcept {
    if (alr.data == nullptr || alr.alr_size == 0) {
        return false; // Something is already wrong...
    }
    bool result = true;

    // We might want access to ALR and/or chunk data during validation
    vfile alr_file = vfile_open(alr.data, alr.alr_size);
    vfile chunkvf = vfile_open(alr.data + chunk.offset, chunk.size);
    chunkvf.pos += sizeof(chunk_generic);

    switch (chunk.id) {
        case 0x0:
            AL_ASSERT(chunk.size == sizeof(chunk_generic), "0x0 chunk had %d bytes of data (expected 8)!", chunk.size);
            break;
        case 0x1: {
            const u16 num_entries = VFILE_READ(u16, &chunkvf);
            result &= validate_entry_sizes(msg, chunk.size, sizeof(chunk_0x1_header), num_entries, sizeof(chunk_0x1_entry));
            break;
        }
        case 0x2: {
            const auto header = VFILE_READ(idxbuf_header, &chunkvf);
            const auto indices = (u16*)vfile_cur(chunkvf);
            if (header.num_indices > 0 && header.first_idx != indices[0]) {
                str_format_append(msg, "The listed first index (%d) didn't match the real first index (%d)!", chunk.offset, header.first_idx, indices[0]);
                result = false;
            }
            const idxbuf_header empty = {0};
            if (memcmp(header.pad, empty.pad, sizeof(header.pad)) != 0) {
                str_format_append(msg, "What I thought was padding had data!");
                result = false;
            }

            // Check that ALR reported array size matches the measured size
            const u32 estimated_num_entries = (chunk.size - sizeof(header)) / sizeof(*indices);
            result &= validate_entry_sizes(msg, chunk.size, sizeof(header), estimated_num_entries, sizeof(*indices));

            // TODO: Check that the vertex/texture entry indices are in bounds

            break;
        }
        case 0x3: {
            const u16 num_joints = VFILE_READ(u16, &chunkvf);
            // We don't validate the entry count since the value in the header
            // doesn't actually indicate entry count
            break;
        }
        case 0x5: {
            const anim_header header = VFILE_READ(anim_header, &chunkvf);
            const al::resource::chunk skel_chunk = alr.first_chunk_in_range(3, chunk.offset, alr.alr_size);
            if (skel_chunk.offset == 0) {
                str_format_append(msg, "Couldn't find matching skeleton chunk for animation @%x", chunk.offset);
            } else {
                vfile skel_vf = vfile_open(alr.data + skel_chunk.offset, skel_chunk.size);
                vfile_seek(&skel_vf, sizeof(chunk_generic));

                // Skeleton header == "skull"
                const chunk_armature skull = VFILE_READ(chunk_armature, &skel_vf);
                const u32 joint_slots = (skel_vf.size - skel_vf.pos) / sizeof(joint_t);

                if (header.unknown_settings1 > skull.joint_count) {
                    str_format_append(msg, "Joint index (0x%x) < joint count (0x%x)", header.unknown_settings1, skull.joint_count);
                    if (header.unknown_settings1 < joint_slots) {
                        str_format_append(msg, "\t(but still under the actual array size (0x%x)", joint_slots);
                    } else {
                        result = false;
                    }
                }
            }
            break;
        }
        case 0x7:
            break;
        case 0xD:
            AL_ASSERT(chunk.size == 12, "0xD chunk had %d bytes of data (expected 12)!", chunk.size);
            break;
        case 0x10: {
            const atlas_header header = VFILE_READ(atlas_header, &chunkvf);
            vfile_seek(&chunkvf, sizeof(atlas_name) * header.atlas_count);

            const auto* atlas_entries = (atlas_entry*)vfile_cur(chunkvf);
            vfile_seek(&chunkvf, sizeof(atlas_entry) * header.atlas_count);
            const auto* tex_entries = (atlas_tex_entry*)vfile_cur(chunkvf);

            for (u32 i = 0; i < header.atlas_count; i++) {
                u32 num_matched = 0;
                for (u32 j = 0; j < header.texture_count; j++) {
                    if (tex_entries[j].index == i) {
                        num_matched++;
                    }
                }

                const u32 expected = atlas_entries[i].tex_count;
                if (num_matched != expected) {
                    str_format_append(msg, "Atlas %d says it has %d children, but there's only %d\n", expected, num_matched);
                    result = false;
                }
            }

            break;
        }
        case 0x11: {
            chunkvf.pos -= sizeof(chunk_generic);
            const auto header = VFILE_READ(chunk_layout, &chunkvf);
            if (header.texbuf_offset + header.texbuf_size > alr.alr_size) {
                str_format_append(msg, "0x%x-byte resource buffer @ 0x%x can't fit in this 0x%x-byte ALR!", header.texbuf_size, header.texbuf_offset, alr.alr_size);
                result = false;
            }
            if (header.pad != 0) {
                str_format_append(msg, "What I thought was padding had data!");
                result = false;
            }
            const u32 guessed_offset_count = (header.chunk_size - sizeof(header)) / sizeof(u32);
            if (guessed_offset_count != header.offset_array_size) {
                str_format_append(msg, "Offset array size seems wrong (found %d, should be %d)!", header.offset_array_size, guessed_offset_count);
                result = false;
            }
            break;
        }
        case 0x15: {
            const u32 num_entries = VFILE_READ(u32, &chunkvf);
            const auto entries = (texture_entry*)vfile_cur(chunkvf);
            result &= validate_entry_sizes(msg, chunk.size, sizeof(chunk_generic), num_entries, sizeof(texture_entry));

            for (u32 i = 0; i < num_entries; i++) {
                const s64 resbuf_size = alr.alr_size - alr.resbuf_offset;
                AL_ASSERT(entries[i].pad == 0, "What I thought was padding in entry %d had data!", i);
                AL_ASSERT(entries[i].data_ptr < resbuf_size, "Entry %d is well outside the resource buffer!", i);
            }
            break;
        }
        case 0x16: {
            const u32 num_entries = VFILE_READ(u32, &chunkvf);
            const auto entries = (texture_entry*)vfile_cur(chunkvf);
            result &= validate_entry_sizes(msg, chunk.size, sizeof(chunk_generic) + sizeof(u32), num_entries, sizeof(vertbuf_entry));
            break;
        }
        default:
            // Unimplemented chunk, skip
            str_format_append(msg, "Unknown chunk type 0x%X @ offset 0x%lx\n", chunk.id, chunk.offset);
            break;
    }

    if (!result) {
        // In CLI mode, we don't have the context that's on-screen in the GUI.
        if (headless) {
            str_format_append(msg, "\t[0x%X chunk @ 0x%x]", chunk.id, chunk.offset);
        }
        msg.append("\n");
    }
    return result;
}