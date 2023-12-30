#pragma once
#include <stdint.h>
#include <stdbool.h>

#include "int_shorthands.h"

typedef struct {
    char* filename;
    char* image_data;
    u8 bits_per_pixel;
    bool compressed;
    u16 width;
    u16 height;
    u16 mipmap_count;
    bool cubemap;
    // When we know the size but our resolution or mip count might be wrong,
    // this prevents EOF errors in other software.
    u32 size_override;
}texture_info;

void write_texture(texture_info texture);

// Calculates the total number of pixels required to store an image, assuming
// the highest possible mipmap count.
u64 pixel_count_max_mips(u32 width, u32 height);

// Calculates the total number of pixels required to store an image with the
// specified mipmap count. Assumes that a mipmap count of 1 means 1 full
// resolution image with no mipmaps.
u64 full_pixel_count(u32 width, u32 height, u32 mipmap_count);

