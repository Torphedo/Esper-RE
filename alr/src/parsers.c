#include <bits/stdint-uintn.h>
#include <stdlib.h>
#include <stdio.h>
#include <stdbool.h>
#include <sys/stat.h>
#include <threads.h>

#include "logging.h"

#include "parsers.h"
#include "alr.h"
#include "images.h"

// Sanity checks for 0xD chunks that have been observed to always be empty
static void chunk_0xD(chunk_generic chunk) {
    // This isn't really a *problem*, but the warning status helps it stand out
    if (chunk.size != 0xC) {
        LOG_MSG(warning, "Non-empty chunk! Size is %d bytes rather than the usual 12 bytes\n", chunk.size);
    }
}

// Reads 0x10 chunks and uses their metadata to write texture data to disk
static void chunk_texture(chunk_generic header, uint8_t* chunk_buf, uint8_t* tex_buf, uint32_t tex_buf_size, resource_entry* entries, flags options) {
    texture_metadata_header* tex_header = (texture_metadata_header*)chunk_buf;

    LOG_MSG(info, "Surface count: %d Image count: %d\n\n", tex_header->surface_count, tex_header->texture_count);

    // &tex_header[1] = the address after the header.
    tex_name* names = (tex_name*)&tex_header[1];

    surface_info* surfaces = (surface_info*)&names[tex_header->surface_count];
    tex_info* textures = (tex_info*)&surfaces[tex_header->surface_count];

    for (uint32_t i = 0; i < tex_header->surface_count; i++) {
        uint32_t pixel_count = surfaces[i].width * surfaces[i].height;
        uint32_t texture_id = textures[i].index;
        uint32_t res_size = 0;
        if (i == tex_header->surface_count - 1) {
            res_size = tex_buf_size - entries[i].data_ptr;
        }
        else {
            res_size = entries[i + 1].data_ptr - entries[i].data_ptr; // next offset - current offset
        }

        uint8_t bits_per_pixel = (res_size / pixel_count) * 8;

        uint32_t total_pixel_count = full_pixel_count(surfaces[i].width, surfaces[i].height, surfaces[i].mipmap_count);

        LOG_MSG(info, "Surface %2d %-32s: %2d mipmap(s), estimated %2d bpp, 0x%05x pixels, %4dx%-4d (", i, &names[i].name, surfaces[i].mipmap_count, bits_per_pixel, total_pixel_count, surfaces[i].width, surfaces[i].height, res_size);

        // Print out size, formatted in appropriate unit
        if (res_size < 0x400) {
            printf("%d bytes", res_size);
        }
        else if (res_size < 0x100000) {
            printf("%gKiB", (double) res_size / 0x400);
        }
        else {
            printf("%gMiB", (double) res_size / 0x100000);
        }
        printf(" buffer @ buf + 0x%x)\n", entries[i].data_ptr);

        if (entries[i].pad != 0) {
            LOG_MSG(debug, "Resource meta (0x15) anomaly, apparent padding at sub-chunk %d was 0x%x\n", i, entries[i].pad);
        }
        if (entries[i].flags != 0x00040001) {
            LOG_MSG(debug, "Resource meta (0x15) anomaly, ID at sub-chunk %d was 0x%x\n", i, entries[i].flags);
        }

        if (!options.info_mode) {
            texture_info info = {
                .filename = (char*)&names[i].name,
                .bits_per_pixel = bits_per_pixel,
                .mipmap_count = surfaces[i].mipmap_count,
                .image_data = (char *)&tex_buf[entries[i].data_ptr],
                .width = surfaces[i].width,
                .height = surfaces[i].height,
            };
            write_texture(info);
        }
    }
    printf("\n");

    uint32_t total_image_pixels = 0;
    for (unsigned int i = 0; i < tex_header->texture_count; i++) {
        LOG_MSG(info, "%-32s: Width %4hi, Height %4hi (Surface %2d)\n", textures[i].filename, textures[i].width, textures[i].height, textures[i].index);
        total_image_pixels += textures[i].width * textures[i].height;
    }
    LOG_MSG(debug, "Total pixel count for all textures: 0x%08x\n", total_image_pixels);
}

uint64_t filesize(const char* path) {
    struct stat st = {0};
    if (stat(path, &st) != 0) {
        LOG_MSG(error, "Failed to get file metadata for %s", path);
        return 0;
    }
    return st.st_size;
}

bool chunk_parse_all(char* alr_filename, flags options) {
    FILE* alr = fopen(alr_filename, "rb");
    if (alr == NULL) {
        LOG_MSG(error, "Couldn't open %s.\n", alr_filename);
        return false;
    }

    LOG_MSG(debug, "Loaded %s (%d bytes)\n", alr_filename, filesize(alr_filename));

    // Read the file header & offset array
    chunk_layout header = {0};
    fread(&header, sizeof(header), 1, alr);
    uint32_t* offset_array = calloc(header.offset_array_size, sizeof(*offset_array));
    if (offset_array == NULL) {
        fclose(alr);
        return false;
    }
    fread(offset_array, sizeof(*offset_array), header.offset_array_size, alr);

    // Get resource metadata. Basically same as the file header but for the big
    // buffer at the end of the file
    resource_layout_header res_header = {0};
    fread(&res_header, sizeof(res_header), 1, alr);
    resource_entry* entries = calloc(res_header.array_size, sizeof(*entries));
    if (entries == NULL) {
        free(offset_array);
        fclose(alr);
        return false;
    }
    fread(entries, sizeof(*entries), res_header.array_size, alr);

    uint32_t tex_buf_size = header.last_resource_end - header.resource_offset;
    uint8_t* tex_buf = calloc(1, tex_buf_size);
    if (tex_buf == NULL) {
        free(offset_array);
        free(entries);
        fclose(alr);
        return false;
    }
    fseek(alr, header.resource_offset, SEEK_SET);
    fread(tex_buf, tex_buf_size, 1, alr);

    // Print some useful info about the file's structure
    if (options.info_mode) {
        LOG_MSG(info, "Resource section = 0x%x\n", header.resource_offset);
        LOG_MSG(info, "Resources end @ 0x%x\n", header.last_resource_end);
        LOG_MSG(info, "%d resources\n", res_header.array_size);
        LOG_MSG(info, "%d internal files\n\n", header.offset_array_size);
        if (options.layout) {
            for (unsigned int i = 0; i < header.offset_array_size; i++) {
                LOG_MSG(info, "File %u: 0x%x\n", i, offset_array[i]);
            }
        }
    }

    // First offset generally points right after the resource layout chunk.
    // This is just to alert us of anomalies.
    if (offset_array[0] != ftell(alr)) {
        LOG_MSG(warning, "Gap between first data chunk & offset!\n");
        LOG_MSG(debug, "data chunk = 0x%x, offset = 0x%x\n", offset_array[0], ftell(alr));
    }

    for (unsigned int i = 0; i < header.offset_array_size; i++) {
        // Some offsets are 0. Don't know why, it's really weird.
        if (offset_array[i] == 0) {
            LOG_MSG(warning, "Offset %d was 0, skipping...\n", i);
            continue;
        }
        fseek(alr, offset_array[i], SEEK_SET); // Jump to offset
        // Read our first chunk to start up the loop
        chunk_generic chunk = {0};

        // Breaks when chunk ID 0 is found.
        // A while(true) here is a little gross but it avoids some duplication.
        while (true) {
            uint64_t chunk_start = ftell(alr);
            fread(&chunk, sizeof(chunk), 1, alr);
            if (chunk.id == 0) {
                break;
            }
            // Need to subtract sizeof(chunk) because the size includes size & ID
            uint64_t buf_size = chunk.size - sizeof(chunk);
            uint8_t* chunk_buf = calloc(1, buf_size);
            if (chunk_buf == NULL) {
                LOG_MSG(error, "Failed to allocate 0x%x bytes for chunk buffer!\n", buf_size);
                break;
            }
            fread(chunk_buf, buf_size, 1, alr);

            // Jump to the next chunk regardless of how our previous reads went
            fseek(alr, chunk_start + chunk.size, SEEK_SET);

            if (chunk.id > 0x16) {
                LOG_MSG(warning, "Invalid chunk ID 0x%x at 0x%x (internal file %d, %s)!\n", chunk.id, ftell(alr), i, alr_filename);
                return false;
            }
            if (options.info_mode && options.layout) {
                LOG_MSG(debug, "0x%x chunk at 0x%x\n", chunk.id, ftell(alr));
            }

            switch (chunk.id) {
                case 0xD:
                    // Just some debug printing in here
                    chunk_0xD(chunk);
                    break;
                case 0x10:
                    chunk_texture(chunk, chunk_buf, tex_buf, tex_buf_size, entries, options);
                    break;
                default:
                    break;
            }
        }

        // I would be very concerned to see a non-empty 0x0 chunk.
        if (chunk.id == 0 && chunk.size != 8) {
            LOG_MSG(debug, "Non-empty chunk! Size is %d bytes rather than the usual 8 bytes\n", chunk.size);
        }
    }
    free(offset_array);
    free(entries);
    fclose(alr);

    return true;
}

