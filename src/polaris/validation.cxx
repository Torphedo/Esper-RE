#include "validation.hxx"
#include <string.h>

#include <common/vfile.h>

#include "util/utils.hxx"

bool alr_validate(std::string& msg, const alr::file& alr, bool headless) noexcept {
    if (!alr.loaded) {
        return true; // Not a failure, just not loaded
    }

    bool result = true;
    std::optional<alr::file::chunk> header_chunk;
    for (const alr::file::chunk& chunk : alr.chunks) {
        result &= alr_chunk_validate(alr, chunk, msg, headless);
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
    vfile alr_vf = vfile_open(alr.data, alr.alr_size);
    vfile header_vf = alr_vf;
    const auto header = VFILE_READ(chunk_layout, &header_vf);
    const s32* offsets = (s32*)vfile_cur(header_vf);

    const s64 size_mismatch = alr.alr_size - (header.texbuf_offset + header.texbuf_size);
    if (size_mismatch < 0) {
        str_format_append(msg,
                          "Header claims resbuf is 0x%X bytes @ 0x%X, but ALR is only 0x%X bytes (off by 0x%X)\n",
                          header.texbuf_size, header.texbuf_offset, alr.alr_size, abs(size_mismatch));
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
        std::optional<alr::file::chunk> terminator;
        // The last chunk before we hit the current offset
        alr::file::chunk last(0xFF, 0, 0);

        // Skip up to the last chunk before the current offset
        while (chunk_idx < alr.chunks.size()) {
            const alr::file::chunk chunk = alr.chunks.at(chunk_idx);

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

bool alr_chunk_validate(const alr::file& alr, const alr::file::chunk& chunk, std::string& msg, bool headless) noexcept {
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
            const auto joint_header = VFILE_READ(chunk_armature, &chunkvf);
            const joint_t* joints = VFILE_READ_PTR(joint_t, &chunkvf);

            for (u32 i = 0; i < joint_header.joint_count; i++) {
                const joint_t& joint = joints[i];
                AL_ASSERT(joint.idx == i, "Joint %d had the wrong index (%d)!", i, joint.idx);
                AL_ASSERT(joint.pad1 == 0, "Joint %d pad1 was 0x%hX, not 0!", i, joint.pad1);
            }

            const u32 size_left = chunk.size - sizeof(chunk_generic) - sizeof(u16);
            const u16 num_entries = size_left / sizeof(joint_t);

            for (u32 i = joint_header.joint_count; i < num_entries; i++) {
                const joint_t& joint = joints[i];
                mat4s identity = GLMS_MAT4_IDENTITY_INIT;
                if (memcmp(&joint, identity.raw, sizeof(joint_t)) != 0) {
                    AL_ASSERT(false, "Joint %d had a non-identity matrix!", i);
                }
            }
            break;
        }
        case 0x5: {
            chunkvf.pos -= sizeof(chunk_generic);
            const anim_header header = VFILE_READ(anim_header, &chunkvf);
            AL_ASSERT(header.scale_key_count == 0, "Scale keys are used!");

            const alr::file::chunk skel_chunk = alr.first_chunk_in_range(3, chunk.offset, alr.alr_size);
            if (skel_chunk.offset == 0) {
                str_format_append(msg, "Couldn't find matching skeleton chunk for animation @ 0x%X", chunk.offset);
            } else {
                vfile skel_vf = vfile_open(alr.data + skel_chunk.offset, skel_chunk.size);
                vfile_seek(&skel_vf, sizeof(chunk_generic));

                // Skeleton header == "skull"
                const chunk_armature skull = VFILE_READ(chunk_armature, &skel_vf);
                const u32 joint_slots = (skel_vf.size - skel_vf.pos) / sizeof(joint_t);

                if (header.joint_idx > skull.joint_count) {
                    str_format_append(msg, "Joint index (0x%x) < joint count (0x%x)", header.joint_idx, skull.joint_count);
                    if (header.joint_idx < joint_slots) {
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
                    str_format_append(msg, "Atlas %d claims to have %d children, but actually has %d\n", i, expected, num_matched);
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
        case 0x12:
        case 0x13:
            // Don't know anything about this chunk type
            break;
        case 0x15: {
            const u32 num_entries = VFILE_READ(u32, &chunkvf);
            const auto entries = (texture_entry*)vfile_cur(chunkvf);
            result &= validate_entry_sizes(msg, chunk.size, sizeof(chunk_generic), num_entries, sizeof(texture_entry));

            for (u32 i = 0; i < num_entries; i++) {
                const s64 resbuf_size = alr.alr_size - alr.resbuf_offset;
                AL_ASSERT(entries[i].data_ptr < resbuf_size, "Entry %d is well outside the resource buffer!", i);
            }
            break;
        }
        case 0x16: {
            const u32 num_entries = VFILE_READ(u32, &chunkvf);
            const auto entries = (vertbuf_entry*)vfile_cur(chunkvf);
            result &= validate_entry_sizes(msg, chunk.size, sizeof(chunk_generic) + sizeof(u32), num_entries, sizeof(vertbuf_entry));

            for (u32 i = 0; i < num_entries; i++) {
                const vertbuf_entry entry = entries[i];
                AL_ASSERT(entry.vertex_size == entry.vertex_size2,
                        "Vertex sizes in entry %d don't match! (%d vs. %d)", i, entry.vertex_size, entry.vertex_size2);

                bool found_format = false;
                if (entry.format < ALR_MAX_FORMAT) {
                    u8 expected_size = 0;
                    for (vertex_format_t format_entry : alr_vert_formats) {
                        if (format_entry.id == entry.format) {
                            expected_size = format_entry.size;
                            break;
                        }
                    }
                    AL_ASSERT(entry.vertex_size == expected_size,
                              "Format ID 0x%x is 0x%x bytes (expected 0x%x)", entry.format, entry.vertex_size, expected_size);

                    // Format 0xD has a size of 0, but any other time that
                    // indicates that the format isn't in the array.
                    found_format = ((entry.format != 0x0D) && (expected_size != 0));
                }

                if (!found_format) {
                    LOG_MSG(warning, "Unknown vertex format 0x%02X with size 0x%02X\n", entry.format, entry.vertex_size);
                }
            }
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

bool ps01_validate(const void* data, u32 offset, u32 num_entries, u32 nm00_count, std::string& msg) noexcept {
    const ps01_entry* entries = (ps01_entry*)data;

    bool result = false;
    for (u32 i = 0; i < num_entries; i++) {
        const ps01_entry& entry = entries[i];
        const u32 cur_offset = offset + i * sizeof(*entries);

        if (entry.object_id >= nm00_count) {
            str_format_append(msg, "Object ID @ 0x%X %d is out of bounds (max %d)", cur_offset, entry.object_id, nm00_count);
            result = false;
        }
        AL_ASSERT(entry.pad == 0, "Apparent PS01 padding @ 0x%X != 0 (0x%X)", cur_offset, entry.pad);

        // Only values 1-4 have been observed
        if (entry.unk1 < 1 || entry.unk1 > 4) {
            str_format_append(msg, "PS01 unk1 @ 0x%X has unknown value 0x%X", cur_offset, entry.unk1);
            result = false;
        }
    }

    return result;
}

bool mapdata_validate(const mapdata& map, std::string& msg) noexcept {
    if (!map.data) {
        return true; // Not a failure, just not loaded
    }
    if (!map.load_verify()) {
        return false;
    }

    bool result = true;
    vfile vf = vfile_open(map.data, map.size);
    const st00_t& header = VFILE_READ(st00_t, &vf);
    // Area file magic has the form "AR0x" (e.g. "AR02", "AR05", etc.)
    const bool is_area = strncmp("AR0", (const char*)&header.magic, 3) == 0;
    if (header.magic != st00_magic && !is_area) {
        str_format_append(msg, "Bad stage magic 0x%X ('%04s')", header.magic, (const char*)&header.magic);
        result = false;
    }

    // Check for new values in areas where only a few values are known
    if (header.unk1 != 1 && header.unk1 != 2 && header.unk1 != 3 && header.unk1 != 9 && header.unk1 != -1) {
        str_format_append(msg, "Header unk1 has unknown value %d!", header.unk1);
    }
    if (header.unk2 != -1 && header.unk2 != 0x140) {
        str_format_append(msg, "Header unk2 has unknown value %d!", header.unk2);
    }

    if (header.chunk_size > 0) {
        const auto* ps00 = (ps01_entry*)(map.data + header.chunk_size);
        ps01_validate(ps00, header.chunk_size, header.ps00_count, header.nm00_count, msg);
    }
    if (header.ps01_offset > 0) {
        const auto* ps01 = (ps01_entry*)(map.data + header.ps01_offset);
        ps01_validate(ps01, header.chunk_size, header.ps01_count, header.nm00_count, msg);
    }

    // TODO: Verify that none of the regions overlap
    // TODO: Verify that area-only fields are unused in normal stages

    return true;
}
