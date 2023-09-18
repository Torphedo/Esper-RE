#include <bits/stdint-uintn.h>
#include <stdio.h>

#include "logging.h"
#include "images.h"

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

// We can't just check equality with "DDS" because it's 'DDS ' with no null
// terminator.
typedef enum {
    DDS_BEGIN = 0x20534444 // 'DDS ' (little endian)
}dds_const;

typedef enum {
    DDS_DXT1 = 0x31545844 // 'DXT1' (little endian)
}dds_bc_format;

typedef struct dds_header {
    uint32_t identifier;   // DDS_BEGIN as defined above. aka "file magic" / "magic number".
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
    uint32_t caps2;        // Always 0 because we don't use cubemaps or volumes
    uint32_t caps3;        // Unused
    uint32_t caps4;        // Unused
    uint32_t reserved2;    // Unused
}dds_header;

void write_texture(texture_info texture) {
    dds_header header = {
        .identifier = DDS_BEGIN,
        .size = 0x7C,
        .flags = REQUIRED_BASE_FLAGS | DDSD_PITCH,
        .height = texture.height,
        .width = texture.width,
        .pitch_or_linear_size = (texture.width * texture.bits_per_pixel + 7) / 8,
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
        // BC1 / DXT1
        case 4:
            header.pixel_format.flags = DDPF_FOURCC;
            header.pixel_format.format_char_code = DDS_DXT1;
            header.flags |= DDSD_LINEARSIZE;
            header.flags ^= DDSD_PITCH;
            break;
        // A8
        case 8:
            header.pixel_format.flags = DDPF_ALPHA;
            header.pixel_format.alpha_bitmask = 0x000000FF;
            break;
        // RGBA8
        case 32:
            header.pixel_format.flags = DDPF_ALPHAPIXELS | DDPF_RGB;
            header.pixel_format.alpha_bitmask = 0xFF000000;
            header.pixel_format.red_bitmask   = 0x00FF0000;
            header.pixel_format.green_bitmask = 0x0000FF00;
            header.pixel_format.blue_bitmask  = 0x000000FF;
            break;
    }

    float bytes_per_pixel = texture.bits_per_pixel / 8.0f;
    uint32_t texture_size = header.height * header.width * bytes_per_pixel;

    FILE* tex_out = fopen(texture.filename, "wb");
    fwrite(&header, sizeof(dds_header), 1, tex_out);
    fwrite(texture.image_data, texture_size, 1, tex_out);
    fclose(tex_out);
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

