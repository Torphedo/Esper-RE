#include <glad/glad.h>

#include <common/vfile.h>
#include "alr_texture.hxx"
#include "editor_alr.hxx"

const char* texformat_str(alr_pixel_format format) {
    const char* out = "[UNKNOWN]";
    switch (format) {
        case FORMAT_MONO_16_2:
        case FORMAT_MONO_16:
            out = "1-channel 16-bit raw";
            break;
        case FORMAT_A8:
        case FORMAT_A8_2:
            out = "1-channel 8-bit raw";
            break;
        case FORMAT_RG8:
            out = "2-channel 8-bit raw";
            break;
        case FORMAT_RGBA8:
        case FORMAT_RGBA8_2:
        case FORMAT_RGBA8_3:
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
        .channels = 4,
    };

    if (entry.unknown == TEXTURE_CUBEMAP) {
        // TODO: Add cubemap support in our standard texture struct
    }

    switch (entry.pixel_format) {
        case FORMAT_MONO_16_2:
        case FORMAT_MONO_16:
            out.unit_size = 1; // See documentation, this means 16-bit channels
            out.channels = 1;
            break;
        case FORMAT_A8:
        case FORMAT_A8_2:
            out.channels = 1;
            break;
        case FORMAT_RG8:
            out.channels = 2;
            break;
        case FORMAT_RGBA8:
        case FORMAT_RGBA8_2:
        case FORMAT_RGBA8_3:
            out.channels = 4;
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
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);

    GLint res = (img.height * img.width);
    if (img.compressed) {
        GLenum format = 0;
        GLint size = res;
        switch (img.fmt) {
            case DXT3:
                format = GL_COMPRESSED_RGBA_S3TC_DXT3_EXT;
                break;
            case DXT5:
                format = GL_COMPRESSED_RGBA_S3TC_DXT5_EXT;
                break;
            default:
            case DXT1:
                format = GL_COMPRESSED_RGBA_S3TC_DXT1_EXT;
                size /= 2;
                break;
        };

        // Re-upload the texture
        glCompressedTexImage2D(GL_TEXTURE_2D, 0, format, img.width, img.height, 0, size, img.data);

    } else {
        // "Raw" uncompressed image
        const GLenum gl_size = GL_UNSIGNED_BYTE + (img.unit_size * 2);
        GLint format;
        switch (img.channels) {
            case 1:
                format = GL_RED;
                break;
            case 2:
                format = GL_RG;
                break;
            case 3:
                format = GL_BGR;
                break;
            default:
                format = GL_BGRA;
                break;
        }

        // Internal format aren't supposed to be a BGR format. Some drivers will
        // let this slide, others will work but give error messages.
        GLint internalFormat = format;
        switch (format) {
            case GL_BGR:
                internalFormat = GL_RGB;
                break;
            case GL_BGRA:
                internalFormat = GL_RGBA;
                break;
            default:
                break;
        }

        glTexImage2D(GL_TEXTURE_2D, 0, internalFormat, img.width, img.height, 0, format, gl_size, img.data);
    }

    // Reset state
    glBindTexture(GL_TEXTURE_2D, 0);
}


gl_obj texture_manager::get(al::resource& alr, u32 idx) noexcept {
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
            LOG_MSG(error, "Requested texture atlas index %d is out of bounds (max = %d)\n", idx, atlasheader.atlas_count);
            return 0;
        }

        // Skip over names
        vfile_seek(&vf, atlasheader.atlas_count * sizeof(atlas_name));
        atlases = (atlas_entry*)vfile_cur(vf);
    }

    texture tex = convert_tex(alr.resource_buffer(), entries[idx]);

    // Use atlas metadata if reasonable
    if (atlases) {
        const u32 too_small = 0;
        const u32 too_big = 8192;
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

bool texture_manager::get_material(al::resource& alr, u32 idx, chunk_0x1_entry** entry_out) const noexcept {
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

bool texture_manager::get_material(al::resource& alr, u32 idx, chunk_0x1_entry* entry_out) const noexcept {
    chunk_0x1_entry* entryptr = nullptr;
    bool result = get_material(alr, idx, &entryptr);
    if (!result || !entryptr) {
        return false;
    }

    *entry_out = *entryptr;
    return true;
}

texture_manager::~texture_manager() noexcept {
    for (const auto& pair : gl_tex_map) {
        gl_obj tex = pair.second;
        glDeleteTextures(1, &tex);
    }

    gl_tex_map.clear();
}