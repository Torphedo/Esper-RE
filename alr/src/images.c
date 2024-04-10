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

typedef enum {
    DDSCAPS2_CUBEMAP           = (1 << 9),
    DDSCAPS2_CUBEMAP_POSITIVEX = (1 << 10),
    DDSCAPS2_CUBEMAP_NEGATIVEX = (1 << 11),
    DDSCAPS2_CUBEMAP_POSITIVEY = (1 << 12),
    DDSCAPS2_CUBEMAP_NEGATIVEY = (1 << 13),
    DDSCAPS2_CUBEMAP_POSITIVEZ = (1 << 14),
    DDSCAPS2_CUBEMAP_NEGATIVEZ = (1 << 15),

    // Sorry this is super long, just all of these flags combined.
    DDS_CUBEMAP_ALL_FACES = DDSCAPS2_CUBEMAP | DDSCAPS2_CUBEMAP_POSITIVEX | DDSCAPS2_CUBEMAP_NEGATIVEX | DDSCAPS2_CUBEMAP_POSITIVEY | DDSCAPS2_CUBEMAP_NEGATIVEY | DDSCAPS2_CUBEMAP_POSITIVEZ | DDSCAPS2_CUBEMAP_NEGATIVEZ
}dds_caps2_flags;

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

typedef struct {
    u32 dxgi_format;
    u32 resource_dimension; // Uses dds_resource_dimension enum
    u32 misc_flags; // Set to 4 (FLAG_2D_TEXTURECUBE) to indicate it's a cubemap.
    u32 array_size; // # of textures, or # of cubemaps (6 textures each)
    u32 misc_flags2; // New alpha settings
}dx10_extended_format;

// HEAVILY abbreviated list from MSDN:
// https://learn.microsoft.com/en-us/windows/win32/api/dxgiformat/ne-dxgiformat-dxgi_format
typedef enum {
    DXGI_FORMAT_BC1_UNORM_SRGB = 71,
    DXGI_FORMAT_BC3_UNORM_SRGB = 78
}dxgi_formats;

typedef enum {
    DIMENSION_1D = 2,
    DIMENSION_2D = 3,
    DIMENSION_3D = 4
}dds_resource_dimension;

typedef enum {
    FLAG_2D_TEXTURECUBE = 4
}misc_flags_1;

// We can't just check equality with "DDS" because it's 'DDS ' with no null
// terminator.
typedef enum {
    DDS_BEGIN = 0x20534444 // 'DDS ' (little endian)
}dds_const;

typedef enum {
    DDS_DXT1 = 0x31545844, // 'DXT1' (little endian)
    DDS_DXT3 = 0x33545844, // 'DXT3' (little endian)
    DDS_DXT5 = 0x35545844, // 'DXT5' (little endian)
    DDS_DX10 = 0x30315844, // 'DX10' (little endian)
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
    u32 block_res = 16; // 16 pixels per block
    // TODO: Divide by pixels per byte instead, so we don't need to use floats.
    float bytes_per_pixel = (float)block_size/ (float)block_res;
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
        },
        .caps = DDSCAPS_TEXTURE
    };

    if (texture.mipmap_count > 1) {
        header.flags |= DDSD_MIPMAPCOUNT;
        header.caps  |= DDSCAPS_MIPMAP | DDSCAPS_COMPLEX;
    }

    if (!texture.compressed) {
        header.pixel_format.bits_per_pixel = texture.bits_per_pixel;
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

        case 16:
            // We assume this is a 16-bit single channel texture
            // (e.g. for a specular map)
            header.pixel_format.flags = DDPF_LUMINANCE;
            header.pixel_format.red_bitmask = 0x0000FFFF;
            break;
        // RGBA8
        case 32:
            header.pixel_format.flags = DDPF_ALPHAPIXELS | DDPF_RGB;
            header.pixel_format.alpha_bitmask = 0xFF000000;
            header.pixel_format.red_bitmask   = 0x00FF0000;
            header.pixel_format.green_bitmask = 0x0000FF00;
            header.pixel_format.blue_bitmask  = 0x000000FF;
            break;
        default:
            LOG_MSG(warning, "No case for %d bpp for texture %s\n", texture.bits_per_pixel, texture.filename);
            LOG_MSG(info, "(proceeding anyway)\n");
            break;
    }

    if (texture.cubemap) {
        header.pixel_format.format_char_code = DDS_DX10;
        header.pixel_format.flags |= DDPF_FOURCC;
        header.caps2 = DDS_CUBEMAP_ALL_FACES;
    }


    float bytes_per_pixel = texture.bits_per_pixel / 8.0f;
    u32 texture_size = header.height * header.width * bytes_per_pixel;
 
    // Apply size override from caller if applicable.
    if (texture.size_override > 0) {
        texture_size = texture.size_override;
    }

    FILE* tex_out = fopen(texture.filename, "wb");
    if (tex_out == NULL) {
        LOG_MSG(error, "Unable to open %s for writing\n", texture.filename);
        return;
    }
    fwrite(&header, sizeof(dds_header), 1, tex_out);

    // Write special DX10 header for cubemaps if needed.
    if (texture.cubemap) {
        dx10_extended_format dx10_header = {
            .resource_dimension = DIMENSION_2D,
            .misc_flags = FLAG_2D_TEXTURECUBE,
            .array_size = 1,
            .misc_flags2 = 0
        };

        // Write appropriate texture format
        if (texture.bits_per_pixel == 4) {
            dx10_header.dxgi_format = DXGI_FORMAT_BC1_UNORM_SRGB;
        }
        else {
            dx10_header.dxgi_format = DXGI_FORMAT_BC3_UNORM_SRGB;
        }
        fwrite(&dx10_header, sizeof(dx10_header), 1, tex_out);
    }

    // TODO: Don't write padding data for texture arrays in cubemaps. The
    // padding makes cubemaps after the first be offset by a few blocks,
    // causing corruption which gets worse each time. Texture arrays are
    // aligned to 0x100.
    fwrite(texture.image_data, texture_size, 1, tex_out);
    fclose(tex_out);
}

u64 pixel_count_max_mips(u32 width, u32 height) {
    u64 count = width * height;

    while (width % 2 == 0) {
        width /= 2;
        height /= 2;
        count += width * height;
    }
    return count;
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

