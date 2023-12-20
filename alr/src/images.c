#include <stdio.h>

#include "int_shorthands.h"
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
    u32 size; // Must be 32 (0x20)
    u32 flags;
    u32 format_char_code; // See dwFourCC here: https://learn.microsoft.com/en-us/windows/win32/direct3ddds/dds-pixelformat
    u32 bits_per_pixel;
    u32 red_bitmask;
    u32 green_bitmask;
    u32 blue_bitmask;
    u32 alpha_bitmask;
}dds_pixel_format;

// We can't just check equality with "DDS" because it's 'DDS ' with no null
// terminator.
typedef enum {
    DDS_BEGIN = 0x20534444 // 'DDS ' (little endian)
}dds_const;

typedef enum {
    DDS_DXT1 = 0x31545844, // 'DXT1' (little endian)
    DDS_DXT3 = 0x33545844, // 'DXT3' (little endian)
    DDS_DXT5 = 0x35545844, // 'DXT5' (little endian)
    DXT1_BLOCK_SIZE = 0x8,
    DXT3_BLOCK_SIZE = 0x10,
    DXT5_BLOCK_SIZE = 0x10
}dds_bc_format;

typedef struct dds_header {
    u32 identifier;   // DDS_BEGIN as defined above. aka "file magic" / "magic number".
    u32 size;         // Must be 124 (0x7C)
    u32 flags;
    u32 height;
    u32 width;
    u32 pitch_or_linear_size;
    u32 depth;
    u32 mipmap_count;
    u32 reserved[11]; // Unused
    dds_pixel_format pixel_format;
    u32 caps;         // Flags for complexity of the surface
    u32 caps2;        // Always 0 because we don't use cubemaps or volumes
    u32 caps3;        // Unused
    u32 caps4;        // Unused
    u32 reserved2;    // Unused
}dds_header;

u32 dxt_pitch(u32 height, u32 width, u32 block_size) {
    u32 block_res = 0x10;
    // TODO: Divide by pixels per byte instead, so we don't need to use floats.
    float bytes_per_pixel = (float)block_res / (float)block_size;
    u32 pitch = (width * height) * bytes_per_pixel;
    return pitch;
}

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
        // .mipmap_count = 0,
        .pixel_format = {
                .size = sizeof(dds_pixel_format),
                .bits_per_pixel = texture.bits_per_pixel
        },
        .caps = DDSCAPS_TEXTURE
    };

    if (texture.mipmap_count > 1) {
        header.flags |= DDSD_MIPMAPCOUNT;
        header.caps  |= DDSCAPS_MIPMAP | DDSCAPS_COMPLEX;
    }
    switch (texture.bits_per_pixel) {
        // DXT1
        case 4:
            header.pixel_format.flags = DDPF_FOURCC;
            header.pixel_format.format_char_code = DDS_DXT1;
            header.flags |= DDSD_LINEARSIZE;
            header.flags ^= DDSD_PITCH;

            u32 pitch = dxt_pitch(texture.height, texture.width, DXT1_BLOCK_SIZE);
            header.pitch_or_linear_size = pitch;
            break;
        // A8 / DXT3 / DXT5
        case 8:
            if (texture.compressed) {
                header.pixel_format.flags = DDPF_FOURCC;
                header.pixel_format.format_char_code = DDS_DXT5;
                header.flags |= DDSD_LINEARSIZE;
                header.flags ^= DDSD_PITCH;

                u32 pitch = dxt_pitch(texture.height, texture.width, DXT5_BLOCK_SIZE);
                header.pitch_or_linear_size = pitch;
            }
            else {
                header.pixel_format.flags = DDPF_ALPHA;
                header.pixel_format.alpha_bitmask = 0x000000FF;
            }
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
    u32 texture_size = header.height * header.width * bytes_per_pixel;
 
    // Apply size override from caller if applicable.
    if (texture.size_override > 0) {
        texture_size = texture.size_override;
    }

    FILE* tex_out = fopen(texture.filename, "wb");
    fwrite(&header, sizeof(dds_header), 1, tex_out);
    fwrite(texture.image_data, texture_size, 1, tex_out);
    fclose(tex_out);
}

u64 full_pixel_count(u32 width, u32 height, u32 mipmap_count) {
    u64 pixel_count = width * height;
    for (u32 i = 0; i < mipmap_count - 1; i++) {
        width /= 2;
        height /= 2;
        pixel_count += (width * height);
    }
    return pixel_count;
}

