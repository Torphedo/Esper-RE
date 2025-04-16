#include <glad/glad.h>

#include "alr_texture.hxx"

const char* texformat_str(alr_pixel_format format) {
    const char* out = "[UNKNOWN]";
    switch (format) {
        case FORMAT_MONO_16_2:
        case FORMAT_MONO_16:
            out = "1-channel 16-bit raw";
            break;
        case FORMAT_A8:
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
    texture out = {};
    out.data = resbuf + entry.data_ptr;
    out.channels = 4;
    out.height = out.width = 1 << entry.resolution_pwr;
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

        glTexImage2D(GL_TEXTURE_2D, 0, format, img.width, img.height, 0, format, gl_size, img.data);
    }

    // Reset state
    glBindTexture(GL_TEXTURE_2D, 0);
}

