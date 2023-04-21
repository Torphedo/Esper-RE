#include <stdio.h>

#include <logging.h>
#include "images.h"

// TGA structures & naming

typedef enum tga_pixel_formats {
    NONE = 0x0,
    UNCOMPRESSED_COLOR_MAP = 0x1,
    UNCOMPRESSED_TRUE_COLOR = 0x2,
    UNCOMPRESSED_GREYSCALE = 0x3,
    RUN_LENGTH_COLOR_MAP = 0x9,
    RUN_LENGTH_TRUE_COLOR = 0xA,
    RUN_LENGTH_GREYSCALE = 0xB
}tga_pixel_formats;

// TGA header
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
        uint8_t image_settings; // This setting makes pixels order from top to bottom when set to 0b00100000
}tga_header;

// DDS structures

typedef enum dds_flags {
    DDSD_CAPS        = 0x00000001,
    DDSD_HEIGHT      = 0x00000002,
    DDSD_WIDTH       = 0x00000004,
    DDSD_PITCH       = 0x00000008,
    DDSD_PIXELFORMAT = 0x00001000,
    DDSD_MIPMAPCOUNT = 0x00020000,
    DDSD_LINEARSIZE  = 0x00080000,
    DDSD_DEPTH       = 0x00800000,

    REQUIRED_BASE_FLAGS = DDSD_CAPS | DDSD_HEIGHT | DDSD_WIDTH | DDSD_PIXELFORMAT
}dds_flags;

typedef enum dds_format_flags {
    DDPF_ALPHAPIXELS = 0x00000001,
    DDPF_ALPHA       = 0x00000002,
    DDPF_FOURCC      = 0x00000004,
    DDPF_RGB         = 0x00000040,
    DDPF_YUV         = 0x00000200,
    DDPF_LUMINANCE   = 0x00020000
}dds_format_flags;

typedef enum dds_caps_flags {
    DDSCAPS_COMPLEX = 0x00000008,
    DDSCAPS_MIPMAP  = 0x00400000,
    DDSCAPS_TEXTURE = 0x00001000
}dds_caps_flags;

typedef struct dds_pixel_format {
    uint32_t size; // Must be 32 (0x20)
    uint32_t flags;
    uint32_t format_char_code; // See dwFourCC here: https://learn.microsoft.com/en-us/windows/win32/direct3ddds/dds-pixelformat
    uint32_t bits_per_pixel;
    uint32_t red_bitmask;
    uint32_t green_bitmask;
    uint32_t blue_bitmask;
    uint32_t alpha_bitmask;
}dds_pixel_format;

// Because they used "DDS " instead of "DDS" with a null terminator, we can't just check equality
// with the string literal "DDS". Thanks, Microsoft.
typedef enum {
    DDS_BEGIN = 0x20534444
}dds_const;

typedef struct dds_header {
    uint32_t identifier;   // "DDS ", or DDS_BEGIN defined above. Also known as the "file magic" or "magic number".
    uint32_t size;         // Must be 124 (0x7C)
    uint32_t flags;
    uint32_t height;
    uint32_t width;
    uint32_t pitch_or_linear_size;
    uint32_t depth;
    uint32_t mipmap_count;
    uint32_t reserved[11]; // Unused
    dds_pixel_format pixel_format;
    uint32_t caps;         // Flags for complexity of the surface
    uint32_t caps2;        // Will always be 0 because we're not dealing with cubemaps or volumes
    uint32_t caps3;        // Unused
    uint32_t caps4;        // Unused
    uint32_t reserved2;    // Unused
}dds_header;

void write_tga(texture_info texture) {
    if (texture.bits_per_pixel == 0 || texture.bits_per_pixel % 8 != 0) {
        log_error(CRITICAL, "write_tga(): Bits per pixel for the file %s had an invalid value of %d\n", texture.filename, texture.bits_per_pixel);
        return;
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
    else if (texture.bits_per_pixel > 32) {
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
}

void write_dds(texture_info texture) {
    dds_header header = {
            .identifier = DDS_BEGIN,
            .size = 0x7C,
            .flags = REQUIRED_BASE_FLAGS | DDSD_PITCH,
            .height = texture.height,
            .width = texture.width,
            .pitch_or_linear_size = ( texture.width * texture.bits_per_pixel + 7 ) / 8,
            .depth = 0,
            .mipmap_count = texture.mipmap_count,
            .pixel_format = {
                    .size = sizeof(dds_pixel_format),
                    .bits_per_pixel = texture.bits_per_pixel
            },
            .caps = DDSCAPS_TEXTURE
    };

    // Disable mipmaps until we can figure out where they're actually stored
    // texture.mipmap_count = 1;

    if (texture.mipmap_count > 1) {
        header.flags |= DDSD_MIPMAPCOUNT;
        header.caps  |= DDSCAPS_MIPMAP | DDSCAPS_COMPLEX;
    }
    switch (texture.bits_per_pixel) {
        case 8:
            header.pixel_format.flags = DDPF_ALPHA;
            header.pixel_format.alpha_bitmask = 0x000000FF;
            break;
        case 32:
            header.pixel_format.flags = DDPF_ALPHAPIXELS | DDPF_RGB;
            header.pixel_format.alpha_bitmask = 0xFF000000;
            header.pixel_format.red_bitmask   = 0x00FF0000;
            header.pixel_format.green_bitmask = 0x0000FF00;
            header.pixel_format.blue_bitmask  = 0x000000FF;
            break;
    }

    uint8_t bytes_per_pixel = texture.bits_per_pixel / 8;

    FILE* tex_out = fopen(texture.filename, "wb");
    fwrite(&header, sizeof(dds_header), 1, tex_out);
    fwrite(texture.image_data, bytes_per_pixel, header.width * header.height, tex_out);
    fclose(tex_out);
}

void write_texture(texture_info texture) {
    switch (texture.format) {
        case TGA:
            write_tga(texture);
            break;
        case DDS:
            write_dds(texture);
            break;
    }
}

uint64_t full_pixel_count(uint32_t width, uint32_t height, uint32_t mipmap_count) {
    uint64_t pixel_count = width * height;
    for (uint32_t i = 0; i < mipmap_count - 1; i++) {
        width /= 2;
        height /= 2;
        pixel_count += (width * height);
    }
    return pixel_count;
}

