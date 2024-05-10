#include <stdlib.h>
#include <stdio.h>
#include <string.h>

#include "logging.h"

#include "int_shorthands.h"
#include "filesystem.h"
#include "alr.h"
#include "images.h"

bool found_texture_meta = false;
texture_info texture_meta[128] = {0};
u32 texture_meta_count = 0;

resource_entry* entries = NULL;
u32 res_entry_count = 0;

// Sanity checks for 0xD chunks that have been observed to always be empty
void chunk_0xD(void* ctx, chunk_generic chunk, u8* chunk_buf, u32 idx) {
    // This isn't really a *problem*, but the warning status helps it stand out
    // (also acts as a nice sanity check)
    if (chunk.size != 0xC) {
        LOG_MSG(warning, "Non-empty chunk! Size is %d bytes rather than the usual 12 bytes\n", chunk.size);
    }
}

// Reads 0x10 chunks and uses their metadata to write texture data to disk
void chunk_texture(void* ctx, chunk_generic header, u8* chunk_buf, u32 idx) {
    texture_metadata_header* tex_header = (texture_metadata_header*)chunk_buf;
    found_texture_meta = true;
    texture_meta_count = tex_header->surface_count;

    LOG_MSG(info, "Surface count: %d Image count: %d\n\n", tex_header->surface_count, tex_header->texture_count);

    // &tex_header[1] = the address after the header.
    tex_name* names = (tex_name*)&tex_header[1];

    surface_info* surfaces = (surface_info*)&names[tex_header->surface_count];
    tex_info* textures = (tex_info*)&surfaces[tex_header->surface_count];

    for (u32 i = 0; i < tex_header->surface_count; i++) {
        u32 pixel_count = surfaces[i].width * surfaces[i].height;

        texture_meta[i].width = surfaces[i].width;
        texture_meta[i].height = surfaces[i].height;

        // Mip count here includes base texture.
        texture_meta[i].mipmap_count = surfaces[i].mipmap_count - 1;

        // TODO: Make this also go into the textures folder. This would require
        // another allocation.
        texture_meta[i].filename = (char*)&names[i].name;

        u32 total_pixel_count = full_pixel_count(surfaces[i].width, surfaces[i].height, surfaces[i].mipmap_count);

        LOG_MSG(info, "Surface %2d %-16s: %2d mip(s),", i, &names[i].name, surfaces[i].mipmap_count - 1);
        printf(" 0x%05X pixels, %4dx%-4d (", total_pixel_count, surfaces[i].width, surfaces[i].height);

        printf("@ buf + 0x%X)\n", entries[i].data_ptr);

        if (entries[i].pad != 0) {
            LOG_MSG(warning, "Resource meta (0x15) anomaly, apparent padding at sub-chunk %d was 0x%X\n", i, entries[i].pad);
        }
        if (entries[i].flags != 0x00040001) {
            LOG_MSG(warning, "Resource meta (0x15) anomaly, ID at sub-chunk %d was 0x%X\n", i, entries[i].flags);
        }
    }
    printf("\n");

    u32 total_image_pixels = 0;
    for (u32 i = 0; i < tex_header->texture_count; i++) {
        LOG_MSG(info, "%-32s: Width %4hi, Height %4hi (Surface %2d)\n", textures[i].filename, textures[i].width, textures[i].height, textures[i].index);
        total_image_pixels += textures[i].width * textures[i].height;
    }
    LOG_MSG(debug, "Total pixel count for all textures: 0x%08X\n", total_image_pixels);
}

void res_layout(void* ctx, chunk_generic chunk, u8* chunk_buf, u32 idx) {
    entries = (resource_entry*)(chunk_buf + sizeof(u32));
    res_entry_count = *(u32*)chunk_buf;
}

void stream_dump(chunk_generic chunk, u8* chunk_buf) {
    // Don't bother trying if there's nothing to write
    s64 buf_size = chunk.size - sizeof(chunk);
    if (buf_size <= 0) {
        LOG_MSG(warning, "Chunk size <= 0, skipping\n");
        return;
    }

    // Generate filename
    char name[1024] = {0};
    sprintf(name, "streams/0x%02X.bin", chunk.id);

    // Make the directory if it doesn't exist.
    if (!dir_exists("streams")) {
        system("mkdir streams");
    }

    // Open file in append mode
    FILE* stream = fopen(name, "ab");

    // Write & exit
    fwrite(chunk_buf, buf_size, 1, stream);
    fclose(stream);
}

void texture_from_meta(u8* buf, u32 size, u32 idx) {
    texture_info* tex = &texture_meta[idx];
    tex->image_data = (char*)buf;
    tex->size_override = size;
    u32 total_pixels = full_pixel_count(tex->width, tex->height, tex->mipmap_count + 1);

    // These must be cast to floats to account for textures with < 8 bpp
    tex->bits_per_pixel = ((float)size / (float)total_pixels) * 8;

    write_texture(*tex);
}

// Try to deduce texture metadata by brute force using the limited data in the
// resource header (0x15 chunk)
void texture_brute(char* path, const u8* buf, u32 size, u32 idx) {
    // Find the position of the '.' in the filename.
    u32 dot_idx = 0;
    for (u32 i = strlen(path); i > 0; i--) {
        if (path[i] == '.') {
            dot_idx = i;
        }
    }
    // We temporarily replace the '.' with a null terminator so it's not in the
    // output filename.
    path[dot_idx] = 0x00;
    char filename[256] = { 0 };
    snprintf(filename, sizeof(filename), "textures/%s_%d.dds", path, idx);
    path[dot_idx] = '.';

    // Make the directory if it doesn't exist.
    if (!dir_exists("textures")) {
        system("mkdir textures");
    }

    u32 resolution = 1 << entries[idx].resolution_pwr;
    u8 format = entries[idx].pixel_format;
    texture_info tex = {
        .width = resolution,
        .height = resolution,
        .filename = filename,
        .image_data = (char*)buf,
        .size_override = size,
    };

    switch (format) {
    case FORMAT_MONO_16:
        tex.bits_per_pixel = 16;
        break;
    case FORMAT_RGBA8:
        tex.bits_per_pixel = 32;
        break;
    case FORMAT_DXT5:
        // Block-compressed dual-channel texture at 16 bytes per block
        // (1 byte per pixel)
        tex.bits_per_pixel = 8;
        tex.compressed = true;
        break;
    case FORMAT_A8:
        tex.bits_per_pixel = 8;
        tex.compressed = false;
        break;
    default:
        // BC1 texture.
        tex.bits_per_pixel = 4;
        tex.compressed = true;
        break;
    }
    LOG_MSG(info, "texture %02d is 0x%05X bytes, %3dX%-3d", idx, size, resolution, resolution);
    printf(", format 0b%08b (%d bpp)\n", format, tex.bits_per_pixel);

    if (entries[idx].unknown == TEXTURE_CUBEMAP) {
        tex.cubemap = true;
    }

    u32 pixel_count = pixel_count_max_mips(tex.width, tex.height);
    float bytes_per_pixel = (float)tex.bits_per_pixel / 8.0f;
    u32 apparent_size = pixel_count * bytes_per_pixel; // Size it ought to be, based on the info we have
    // Disable mipmaps and give a debug message when our size guessing is way
    // off. Reduces the chance of a DDS file that fails to load
    if (apparent_size > tex.size_override) {
        LOG_MSG(warning, "format or res might be wrong, mipmaps are disabled. pixel_count = 0x%X @ %1.1fBpp, but size = 0x%X\n", pixel_count, bytes_per_pixel, tex.size_override);
    }
    else {
        tex.mipmap_count = entries[idx].resolution_pwr + 1;
    }

    if (apparent_size * 2 < tex.size_override && !tex.cubemap) {
        // Buffer is more than double what should be needed...
        LOG_MSG(warning, "Way more space than needed, texture might be a cubemap.\n");
    }
    // LOG_MSG(debug, "unknown = 0x%hX, ", entries[idx].unknown);
    // printf("unknown2 = 0x%hX, ", entries[idx].unknown2);
    // printf("unknown3 = 0x%X\n", entries[idx].unknown3);

    write_texture(tex);
}

void process_texture(void* ctx, u8* buf, u32 size, u32 idx) {
    if (found_texture_meta && idx <= texture_meta_count) {
        // Try to pull data intelligently where possible
        texture_from_meta(buf, size, idx);
    }
    if (ctx != NULL) {
        texture_brute((char*)ctx, buf, size, idx);
    }
}

