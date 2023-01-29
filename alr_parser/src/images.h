#pragma once
#include <stdint.h>

typedef enum {
    TGA,
    DDS
}image_type;

typedef struct {
    char* filename;
    char* image_data;
    uint8_t bits_per_pixel;
    uint16_t width;
    uint16_t height;
    uint16_t mipmap_count;
    image_type format;
}texture_info;

void write_texture(texture_info texture);

// Calculates the total number of pixels required to store an image with the specified mipmap
// count. Assumes that a mipmap count of 1 means 1 full-resolution image with no mipmaps.
uint64_t full_pixel_count(uint32_t width, uint32_t height, uint32_t mipmap_count);
