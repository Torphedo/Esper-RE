#include <glad/glad.h>

#include <alr/alr_file.hxx>
#include <alr/alr_dump.hxx>
#include "alr_assets.hxx"


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

void update_gl_tex(texture img, gl_obj texture_id) {
    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D, texture_id);

    // Wrapping & filtering settings
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_BORDER);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_BORDER);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_MAX_ANISOTROPY_EXT, 16.0f);

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

void texture_manager::invalidate_all() noexcept {
    destroy();
}

gl_obj texture_manager::get(alr::file& alr, u32 idx) noexcept {
    if (gl_tex_map.contains(idx)) {
        return gl_tex_map.at(idx);
    }

    if (texheader_offset == 0) {
        // What do you want from me?
        return 0;
    }

    vfile vf = vfile_open(alr.data, alr.alr_size);
    vf.pos = texheader_offset;
    const auto texheader = VFILE_READ(texture_header, &vf);
    if (idx > texheader.num_entries) {
        LOG_MSG(error, "Requested texture index %d is out of bounds (max = %d)\n", idx, texheader.num_entries);
        return 0;
    }
    const auto* entries = (texture_entry*)vfile_cur(vf);

    // Convert to our custom texture struct, then upload to OpenGL
    texture tex = alr::convert_tex(alr.resource_buffer(), entries[idx]);

    gl_obj gl_tex_id = 0;
    glGenTextures(1, &gl_tex_id);
    update_gl_tex(tex, gl_tex_id);

    gl_tex_map[idx] = gl_tex_id;

    return gl_tex_id;
}

bool texture_manager::get_material(alr::file& alr, u32 material_header_offset, u32 idx, material_entry** entry_out) const noexcept {
    vfile vf = vfile_open(alr.data, alr.alr_size);
    vf.pos = material_header_offset;
    const auto header = VFILE_READ_PTR(material_header, &vf);

    if (idx < header->num_entries) {
        *entry_out = &header->entries[idx];
        return true;
    }

    return false;
}

bool texture_manager::get_material(alr::file& alr, u32 material_header_offset, u32 idx, material_entry* entry_out) const noexcept {
    material_entry* entryptr = nullptr;
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
