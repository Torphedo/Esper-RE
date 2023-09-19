#pragma once
#include <stdint.h>

#include "int_shorthands.h"

typedef struct {
    char* filename;
    char* image_data;
    u8 bits_per_pixel;
    u16 width;
    u16 height;
    u16 mipmap_count;
}texture_info;

void write_texture(texture_info texture);

// Calculates the total number of pixels required to store an image with the
// specified mipmap count. Assumes that a mipmap count of 1 means 1 full
// resolution image with no mipmaps.
u64 full_pixel_count(u32 width, u32 height, u32 mipmap_count);

