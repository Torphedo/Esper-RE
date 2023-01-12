#pragma once
#include <stdint.h>

typedef struct texture_info
{
    char* filename;
    char* image_data;
    uint8_t bits_per_pixel;
    uint16_t width;
    uint16_t height;
    uint16_t mipmap_count;
}texture_info;

bool write_tga(texture_info texture);
