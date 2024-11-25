#include <stdbool.h>
#include <stdio.h>

#include <glad/glad.h>
#include <cglm/struct.h>

#include <common/gl/input.h>
#include <common/int.h>
#include <common/image.h>
#include <common/logging.h>

input_internal input_prev = {
    .space = true, // Update texture state on startup
};

void viewer_update(texture* img) {
    bool up = (input.k && !input_prev.k) || (input.up && !input_prev.up);
    bool down = (input.j && !input_prev.j) || (input.down && input_prev.down);
    bool left = (input.h && !input_prev.h) || (input.left && !input_prev.left);
    bool right = (input.l && !input_prev.l) || (input.right && !input_prev.right);
    bool space = (input.space * !input_prev.space);
    img->compressed ^= input.c && !input_prev.c; // Toggle if pressed

    // Increments of 1, or by 4 if compressed (compressed resolution must be a multiple of 4)
    s32 delta_h = right - left;
    s32 delta_v = up - down;
    // Compressed images have to increase in increments of their (square) block
    // width, but uncompressed ones can increase by 1 pixel at a time.
    u32 multiplier = img->compressed ? COMPRESSED_BLK_DIM : 1;
    multiplier *= (input.alt ? 16 : 1); // Adjust 16x faster when alt is held

    // Adjust image dimensions
    img->height += delta_v * multiplier;
    img->width  += delta_h * multiplier;

    printf("\033[1F\033[2K"); // Go up a line & clear

    GLint res = (img->height * img->width);
    if (img->compressed) {
        img_snap(img, 4); // Keep image size at multiple of 4
        img->fmt = (img->fmt + space) % DXT_ENUM_MAX; // Cycle through formats

        GLenum format = 0;
        GLint size = res;
        switch (img->fmt) {
            case DXT3:
                LOG_MSG(info, "DXT3");
                format = GL_COMPRESSED_RGBA_S3TC_DXT3_EXT;
                break;
            case DXT5:
                LOG_MSG(info, "DXT5");
                format = GL_COMPRESSED_RGBA_S3TC_DXT5_EXT;
                break;
            default:
                LOG_MSG(info, "DXT1");
                format = GL_COMPRESSED_RGBA_S3TC_DXT1_EXT;
                size /= 2;
                break;
        };
        printf(" %dx%d\n", img->width, img->height);
        glCompressedTexImage2D(GL_TEXTURE_2D, 0, format, img->width, img->height, 0, size, img->data);
    }
    else {
        if (input.shift) {
            img->unit_size += space;
            img->unit_size %= 2;
        }
        else {
            img->channels--;
            img->channels += space;
            img->channels = (img->channels % 4) + 1;
        }

        LOG_MSG(info, "%d-bit, %d channels", (1 << img->unit_size) * 8, img->channels);
        printf(" %dx%d\n", img->width, img->height);
        GLenum gl_size = GL_UNSIGNED_BYTE + (img->unit_size * 2);
        GLint format;
        switch (img->channels) {
            case 1:
                format = GL_RED;
                break;
            case 2:
                format = GL_RG;
                break;
            case 3:
                format = GL_RGB;
                break;
            default:
                format = GL_RGBA;
                break;
        }

        glTexImage2D(GL_TEXTURE_2D, 0, format, img->width, img->height, 0, format, gl_size, img->data);
    }

    // Wrapping & filtering settings
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_BORDER);	
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_BORDER);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST_MIPMAP_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
    glGenerateMipmap(GL_TEXTURE_2D);

    if (input.w && !input_prev.w) {
        img_write(*img, "img.dds");
    }

    input_prev = input;
}

