#include <glad/glad.h>
#include "alr_assets.hxx"

#include <formats/alr_animations.h>
#include <alr/alr_file.hxx>

const char* texformat_str(alr_pixel_format format) {
    const char* out = "[UNKNOWN]";
    switch (format) {
        case FORMAT_A8:
            out = "1-channel 8-bit raw [red]";
            break;
        case FORMAT_R8:
        case FORMAT_R8_2:
            out = "1-channel 8-bit raw [alpha]";
            break;
        case FORMAT_BGRA_5551:
            out = "4-channel 5/5/5/1-bit raw";
            break;
        case FORMAT_BGR_565:
            out = "3-channel 5/6/5-bit raw";
            break;
        case FORMAT_BGRA_4444:
            out = "4-channel 4-bit raw";
            break;
        case FORMAT_RGBA8:
        case FORMAT_RGBA8_2:
        // case FORMAT_RGBA8_3:
            out = "4-channel 8-bit raw [RGBA8]";
            break;
        case FORMAT_DXT1:
            out = "Compressed DXT1/BC1";
            break;
        case FORMAT_DXT3:
            out = "Compressed DXT3/BC2";
            break;
        case FORMAT_DXT5:
            out = "Compressed DXT5/BC3";
            break;
    }

    return out;
}

texture convert_tex(u8* resbuf, texture_entry entry) {
    // We default to uncompressed RGBA8 here
    texture out = {
        .data = resbuf + entry.data_ptr,
        .height = out.width = 1 << entry.resolution_pwr,
        .compressed = false,
        .unit_size = 1,
        .channels = 4,
    };

    if (entry.unknown == TEXTURE_CUBEMAP) {
        out.cubemap = true;
        out.cubemap_alignment = 0x100;
        out.use_mipmaps = true;
    }

    switch (entry.pixel_format) {
        case FORMAT_A8:
        case FORMAT_R8:
        case FORMAT_R8_2:
            out.channels = 1;
            break;
        case FORMAT_RGBA8:
        case FORMAT_RGBA8_2:
        // case FORMAT_RGBA8_3:
            out.channels = 4;
            break;
        case FORMAT_BGR_565:
            out.compressed = true;
            out.fmt = DDS_FORMAT_BGR_565;
            break;
        case FORMAT_BGRA_5551:
            out.compressed = true;
            out.fmt = DDS_FORMAT_BGRA_5551;
            break;
        case FORMAT_BGRA_4444:
            out.compressed = true;
            out.fmt = DDS_FORMAT_BGRA_4444;
            break;
        case FORMAT_DXT1:
            out.compressed = true;
            out.fmt = DXT1;
            break;
        case FORMAT_DXT3:
            out.compressed = true;
            out.fmt = DXT3;
            break;
        case FORMAT_DXT5:
            out.compressed = true;
            out.fmt = DXT5;
            break;
    }

    return out;
}

void update_gl_tex(texture img, gl_obj texture_id) {
    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D, texture_id);

    // Wrapping & filtering settings
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_BORDER);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_BORDER);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST_MIPMAP_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

    GLint res = (img.height * img.width);
    if (img.compressed) {
        GLenum format = 0;
        GLint size = res;
        bool block_compressed = true;
        switch (img.fmt) {
        case DXT3:
            format = GL_COMPRESSED_RGBA_S3TC_DXT3_EXT;
            break;
        case DXT5:
            format = GL_COMPRESSED_RGBA_S3TC_DXT5_EXT;
            break;
        case DXT1:
            format = GL_COMPRESSED_RGBA_S3TC_DXT1_EXT;
            size /= 2;
            break;
        default:
            block_compressed = false;
            break;
        };

        if (block_compressed) {
            // Re-upload the texture
            glCompressedTexImage2D(GL_TEXTURE_2D, 0, format, img.width, img.height, 0, size, img.data);
        } else {
            GLenum gl_size = 0;
            GLenum internalFormat = 0;
            GLenum glFormat = GL_RGBA;
            // These aren't considered compressed by OpenGL
            switch (img.fmt) {
            case DDS_FORMAT_BGRA_5551:
                gl_size = GL_UNSIGNED_SHORT_5_5_5_1;
                internalFormat = GL_RGB5_A1;
                break;
            case DDS_FORMAT_BGR_565:
                gl_size = GL_UNSIGNED_SHORT_5_6_5;
                internalFormat = GL_RGB5;
                glFormat = GL_RGB;
                break;
            case DDS_FORMAT_BGRA_4444:
                gl_size = GL_UNSIGNED_SHORT_4_4_4_4;
                internalFormat = GL_RGBA4;
                break;
            default:
                LOG_MSG(error, "Unknown bobtail compressed format code: %d\n", img.fmt);
                // Exit! Otherwise the internal format will be 0 and crash.
                return;
            }

            glTexImage2D(GL_TEXTURE_2D, 0, internalFormat, img.width, img.height, 0, glFormat, gl_size, img.data);
        }
    } else {
        // "Raw" uncompressed image
        GLenum gl_size = GL_UNSIGNED_BYTE;
        GLint gl_format;

        switch (img.unit_size) {
        case 1:
            gl_size = GL_UNSIGNED_BYTE;
            break;
        case 2:
            gl_size = GL_UNSIGNED_SHORT;
            break;
        case 4:
            gl_size = GL_UNSIGNED_INT;
            break;
        default:
            // LOG_MSG(warning, "Unknown unit size %d, assuming 8-bit.\n", img.unit_size);
            break;
        }

        switch (img.channels) {
        case 1:
            gl_format = GL_RED;
            break;
        case 2:
            gl_format = GL_RG;
            break;
        case 3:
            gl_format = GL_BGR;
            break;
        default:
            gl_format = GL_BGRA;
            break;
        }

        // Internal format aren't supposed to be a BGR format. Some drivers will
        // let this slide, others will work but give error messages.
        GLint internalFormat = gl_format;
        switch (gl_format) {
        case GL_BGR:
            internalFormat = GL_RGB;
            break;
        case GL_BGRA:
            internalFormat = GL_RGBA;
            break;
        default:
            break;
        }

        glTexImage2D(GL_TEXTURE_2D, 0, internalFormat, img.width, img.height, 0, gl_format, gl_size, img.data);
    }
    glGenerateMipmap(GL_TEXTURE_2D);

    // Reset state
    glBindTexture(GL_TEXTURE_2D, 0);
}


void texture_manager::invalidate(u32 idx) noexcept {
    glDeleteTextures(1, &gl_tex_map[idx]);
    gl_tex_map.erase(idx);
}

gl_obj texture_manager::get(alr::file& alr, u32 idx) noexcept {
    if (gl_tex_map.contains(idx)) {
        return gl_tex_map[idx];
    }

    if (texheader_offset == 0) {
        // What do you want from me?
        return 0;
    }

    vfile vf = vfile_open(alr.data, alr.alr_size);
    vf.pos = texheader_offset;
    const auto texheader = VFILE_READ(texture_header, &vf);
    if (idx > texheader.array_size) {
        LOG_MSG(error, "Requested texture index %d is out of bounds (max = %d)\n", idx, texheader.array_size);
        return 0;
    }
    const auto* entries = (texture_entry*)vfile_cur(vf);

    const atlas_entry* atlases = nullptr;
    if (atlasheader_offset > 0) {
        vf.pos = atlasheader_offset;
        vfile_seek(&vf, sizeof(chunk_generic)); // Skip id/size
        const auto atlasheader = VFILE_READ(atlas_header, &vf);
        if (idx > atlasheader.atlas_count) {
            LOG_MSG(warning, "Requested texture atlas index %d is out of bounds (max = %d)\n", idx, atlasheader.atlas_count);
        } else {
            // Skip over names
            vfile_seek(&vf, atlasheader.atlas_count * sizeof(atlas_name));
            atlases = (atlas_entry*)vfile_cur(vf);
        }
    }

    texture tex = convert_tex(alr.resource_buffer(), entries[idx]);

    // Use atlas metadata if reasonable
    if (atlases) {
        const u32 too_small = 0;
        const u32 too_big = 2048;
        const u32 height = atlases[idx].height;
        const u32 width = atlases[idx].width;
        if (too_small < height && height < too_big) {
            tex.height = height;
        }

        if (too_small < width && width < too_big) {
            tex.width = width;
        }

    }

    gl_obj gl_tex_id = 0;
    glGenTextures(1, &gl_tex_id);
    update_gl_tex(tex, gl_tex_id);

    gl_tex_map[idx] = gl_tex_id;

    return gl_tex_id;
}

bool texture_manager::get_material(alr::file& alr, u32 material_header_offset, u32 idx, chunk_0x1_entry** entry_out) const noexcept {
    vfile vf = vfile_open(alr.data, alr.alr_size);
    vf.pos = material_header_offset;
    const auto header = VFILE_READ(chunk_0x1_header, &vf);
    auto* entries = (chunk_0x1_entry*)vfile_cur(vf);

    if (idx < header.num_entries) {
        *entry_out = &entries[idx];
        return true;
    }

    return false;
}

bool texture_manager::get_material(alr::file& alr, u32 material_header_offset, u32 idx, chunk_0x1_entry* entry_out) const noexcept {
    chunk_0x1_entry* entryptr = nullptr;
    bool result = get_material(alr, material_header_offset, idx, &entryptr);
    if (!result || !entryptr) {
        return false;
    }

    *entry_out = *entryptr;
    return true;
}

void texture_manager::destroy() noexcept {
    for (const auto& pair : gl_tex_map) {
        gl_obj tex = pair.second;
        glDeleteTextures(1, &tex);
    }

    gl_tex_map.clear();
}

texture_manager::~texture_manager() noexcept {
    destroy();
}

mesh_view mesh_at_idx(const alr::file& alr, u32 idx, u32 vertbuf_idx) {
    mesh_view out = {};

    const alr::file::chunk header_chunk = alr.chunks[0];
    if (header_chunk.id != 0x11) {
        LOG_MSG(error, "Header chunk ID != 0x11, something is seriously wrong!\n");
        return out;
    }

    // We cast away const because we won't be editing the ALR data at all.
    vfile vf = vfile_open(alr.data, alr.alr_size);
    vf.pos = header_chunk.offset;

    const auto* header = (chunk_layout*)vfile_cur(vf);
    if (header->offset_array_size > ALR_NUM_PLAYER_ANIMATIONS) {
        // This is a player file, so all the animation offsets come before the
        // model offsets, and we need to skip past them.
        idx += ALR_NUM_PLAYER_ANIMATIONS;
    }
    if (idx >= header->offset_array_size) {
        LOG_MSG(error, "Mesh index %d is out of bounds (max = %d)\n", idx, header->offset_array_size);
        return out;
    }

    const s32 offset = header->offsets[idx];
    if (offset < 0) {
        LOG_MSG(error, "Mesh index %d doesn't exist (negative offset %d)\n", idx, offset);
        return out;
    }
    vf.pos = offset;

    // Skip 0x1 (materials) chunk
    const auto* generic_0x1 = (chunk_generic*)vfile_cur(vf);
    vfile_seek(&vf, generic_0x1->size);

    // Get joint array from 0x3 chunk
    u32 next_chunk_off = vf.pos;
    const u32 armature_chunk_offset = vf.pos;
    const auto generic_0x3 = VFILE_READ(chunk_generic, &vf);
    next_chunk_off += generic_0x3.size;

    // Unused but we read them anyway
    const auto* joint_header = VFILE_READ_PTR(chunk_armature, &vf);
    const joint_t* joints = (joint_t*)vfile_cur(vf);

    // Skip to the 0x16 chunk
    vf.pos = next_chunk_off;
    const auto generic_0x16 = VFILE_READ(chunk_generic, &vf);
    next_chunk_off = next_chunk_off + generic_0x16.size;

    // Get vertex buffer entries
    const u32 num_vertbuf_entries = VFILE_READ(u32, &vf);
    const auto* vertbuf_entries = (vertbuf_entry*)vfile_cur(vf);

    // Get the actual vertex buffer
    const u8* resbuf = alr.resource_buffer();
    const vertbuf_entry& entry = vertbuf_entries[vertbuf_idx];
    const u8* vertices = resbuf + entry.data_ptr;

    // Upload vertex buffer with correct attributes
    out.setup();
    out.update_vertex_buf(vertices, entry.vertex_size * entry.vertex_count);
    get_vert_attribute(&out, entry);
    out.apply_attributes();

    // Skip past the rest of the 0x16 chunk, to the first 0x2 chunk
    vf.pos = next_chunk_off;

    // Parse all index buffers
    u32 cur_offset = vf.pos;
    chunk_generic cur_chunk = VFILE_READ(chunk_generic, &vf);
    while (cur_chunk.id == 0x2) {
        const idxbuf_header idx_header = VFILE_READ(idxbuf_header, &vf);
        if (idx_header.vertex_buf == vertbuf_idx) {
            // Setup & add index buffer
            const index_buffer idx_buf(cur_offset, armature_chunk_offset);
            out.add_index_buf(alr.data, alr.alr_size, idx_buf);
        }

        // Prepare to read next index buffer
        cur_offset += cur_chunk.size;
        vf.pos = cur_offset;
        cur_chunk = VFILE_READ(chunk_generic, &vf);
        assert(cur_chunk.id < 0x15);
    }

    return out;
}
