#include <stdbool.h>
#include <stdio.h>

#include "data_structures.h"
#include "images.h"
#include "lib/logging.h"

#define UNCOMPRESSED_TRUE_COLOR (0x2)
#define UNCOMPRESSED_GREYSCALE  (0x3)

// TGA header with all relevant settings until the shorts for width & height.
typedef struct tga_header {
        uint8_t id_size; // 0
        uint8_t color_map; // 0
        uint8_t format;
        uint8_t color_map_spec[5]; // Unused, describes color map if present
        uint16_t x_origin; // 0
        uint16_t y_origin; // 0
        uint16_t width;
        uint16_t height;
        uint8_t bits_per_pixel;
        uint8_t image_settings; // This setting makes pixels order from top to bottom when 0b00100000
}tga_header;

bool write_tga(texture_info texture)
{
    if (texture.bits_per_pixel == 0 || texture.bits_per_pixel % 8 != 0) {
        log_error(CRITICAL, "write_tga(): Bits per pixel for the file %s had an invalid value of %d\n", texture.filename, texture.bits_per_pixel);
        return false;
    }

    tga_header header = {
            .width = texture.width,
            .height = texture.height,
            .bits_per_pixel = texture.bits_per_pixel,
            .image_settings = 0b00100000
    };

    if (texture.bits_per_pixel == 8) {
        header.format = UNCOMPRESSED_GREYSCALE;
    }
    else if (texture.bits_per_pixel > 32)
    {
        log_error(WARNING, "write_tga(): The file %s has %d bits per pixel, higher than the TGA limit of 32.\n", texture.filename, texture.bits_per_pixel);
    }
    else {
        header.format = UNCOMPRESSED_TRUE_COLOR;
    }

    uint8_t bytes_per_pixel = header.bits_per_pixel / 8;
    // Writing out an uncompressed TGA file
    FILE* tex_out = fopen(texture.filename, "wb");
    fwrite(&header, sizeof(tga_header), 1, tex_out);
    fwrite(texture.image_data, bytes_per_pixel, header.width * header.height, tex_out);
    fclose(tex_out);
    return true;
}
