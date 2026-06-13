#include "alr.h"
#include <math.h>

/// @brief Find the total number of pixels in a texture with full mipmaps
/// @param width The texture width
/// @param height The texture height
/// @param compressed If the texture is block-compressed
/// @return The number of pixels
u64 calc_pixel_count(u32 width, u32 height, bool compressed) {
    u64 count = width * height;

    while (width > 0) {
        width /= 2;
        height /= 2;
        u32 res = width * height;
        if (compressed) {
            // Don't include 0 as a mipmap
            if (res < 1) {
                break;
            }
            // Compressed textures can only go as low as a 4x4 mipmap
            res = MAX(res, 16);
        }
        count += res;
    }
    return count;
}

bool is_power_of_two(u16 val) {
    // For example:
    // 2^7     = 0x80 = 0b100000000
    // 2^7 - 1 = 0x7F = 0b001111111
    // 0x80 & 0x7F == 0 (false).

    // See https://stackoverflow.com/a/108340
    return !(val & (val - 1));
}

void alr_texture_get_dimensions(texture_entry entry, u16* height_out, u16* width_out) {
    // It would make more sense to check if *either* is 0, but the game only
    // checks that they're *both* 0.
    if (entry.width_direct == 0 && entry.width_direct == 0) {
        *width_out = 1 << entry.width_pwr;
        *height_out = 1 << entry.height_pwr;
    } else {
        *width_out = entry.width_direct + 1;
        *height_out = entry.height_direct + 1;
    }
}

void alr_texture_set_dimensions(texture_entry* entry, u16 height, u16 width) {
    if (is_power_of_two(height) && is_power_of_two(width)) {
        entry->height_pwr = log2(height);
        entry->width_pwr = log2(width);

        // Wipe the other fields so the game doesn't try to use them
        entry->height_direct = 0;
        entry->width_direct = 0;
    } else {
        height = MAX(1, height); // Avoid underflow
        width = MAX(1, width);
        entry->height_direct = height - 1;
        entry->width_direct = width - 1;

        // Wipe the other fields so the game doesn't try to use them
        entry->height_pwr = 0;
        entry->width_pwr = 0;
    }
}

texture_entry alr_make_blank_texture(u16 height, u16 width, alr_pixel_format format) {
    texture_entry entry = {
        .flags = ALR_DEFAULT_TEXTURE_FLAGS,
        .dimensions = 2, // 2D texture
        .unused2 = 1,
        .unused3 = 1,
        .pixel_format = format,
        .mipmap_count = 1, // This means just the base texture
    };
    alr_texture_set_dimensions(&entry, height, width);
    entry.text1 = encode_single32("newtex");

    // TODO: Allow caller to set mip count or enable full mipmaps (we calculate it for them)
    return entry;
}

u32 alr_texture_calc_size(texture_entry entry) {
    u16 width = 0, height = 0;
    alr_texture_get_dimensions(entry, &height, &width);
    const alr_pixel_format f = entry.pixel_format;
    const bool compressed = (f == FORMAT_DXT1) || (f == FORMAT_DXT3) || (f == FORMAT_DXT5);

    u32 bitsPerPixel = 0;
    switch (f) {
        case FORMAT_DXT1: // 8-byte block per 16 pixels
            bitsPerPixel = 4;
            break;
        case FORMAT_DXT3: // 8-byte RGB block plus 4-bit raw alpha, which adds
                          // up to 64 bits per 16 pixels.
                          // Totals 16 bytes per 16 pixels.
        case FORMAT_DXT5: // Uses two 8-byte blocks per 16 pixels

        case FORMAT_A8:   // Raw 8-bit single channel formats
        case FORMAT_R8:
        case FORMAT_R8_2:
            bitsPerPixel = 8;
            break;
        case FORMAT_RGBA8:
        case FORMAT_RGBA8_2:
            bitsPerPixel = 32;
            break;
        case FORMAT_BGR_565:
        case FORMAT_BGRA_4444:
        case FORMAT_BGRA_5551:
            bitsPerPixel = 16;
            break;
    }

    u64 numPixels = width * height;
    if (entry.mipmap_count > 1) {
        numPixels = calc_pixel_count(width, height, compressed);
    }
    if (entry.is_cubemap) {
        numPixels = ALIGN_UP(numPixels, ALR_CUBEMAP_ALIGNMENT);
        numPixels *= 6; // Cubemap has 6 faces
    }

    const u64 totalBits = numPixels * bitsPerPixel;
    const u64 totalBytes = totalBits / 8;

    assert(totalBytes <= UINT32_MAX && "This function can't handle textures bigger than 4GiB!");
    return (u32)totalBytes;
}