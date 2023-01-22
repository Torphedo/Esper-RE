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
