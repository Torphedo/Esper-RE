#include <stdlib.h>
#include <stdio.h>
#include <stdbool.h>
#include <sys/stat.h>

#include <logging.h>

#include "parsers.h"
#include "alr.h"
#include "images.h"

// Writes each distinct section of an ALR to separate files on disk
bool split_alr(char* alr_filename) {
    FILE* alr = fopen(alr_filename, "rb");
    if (alr == NULL) {
        log_error(CRITICAL, "split_alr(): Failed to open %s\n", alr_filename);
        return false;
    }

    chunk_layout header = {0};
    // Read header
    fread(&header, sizeof(chunk_layout), 1, alr);
    uint32_t* pointer_array = malloc(header.offset_array_size * sizeof(uint32_t));
    if (pointer_array == NULL) {
        log_error(CRITICAL, "split_alr(): Failed to allocate pointer array with %d elements\n", header.offset_array_size);
        return false;
    }
    else {
    // Read in pointer array
    fread(pointer_array, sizeof(uint32_t), header.offset_array_size, alr);

        // Write each large chunk of data to a separate file.
        for (uint32_t i = 0; i < header.offset_array_size; i++) {
            // Length between the current and next pointer.
            int32_t size;
            if (i < header.offset_array_size - 1) {
                size = pointer_array[i + 1] - pointer_array[i];
            }
            else {
                size = header.resource_offset - pointer_array[i];
            }

            if (size < 0) {
                log_error(WARNING, "split_alr(): Adjusting for negative chunk size at chunk %d.\n", i);
                size *= -1;
            }

            // 256MiB
            if (size > 0x10000000) {
                log_error(CRITICAL, "split_alr(): Chunk size at chunk %d was unreasonably high (%d bytes)!\n", i, size);
                return false;
            }
            char* buffer = malloc(size);
            fseek(alr, (long) pointer_array[i], SEEK_SET);
            fread(buffer, size, 1, alr);

            // Because i is a uint32_t, the longest possible filename is
            // 4294967295.bin, which is 14 characters
            char filename[16];
            if (snprintf(filename, 15, "%u.bin", i) > 0) {
                FILE *dump = fopen(filename, "wb");
                fwrite(buffer, size, 1, dump);
                fclose(dump);
            }
            free(buffer);
        }
	free(pointer_array);
    }

    // Dump all resources to separate files
    fseek(alr, sizeof(chunk_layout) + (header.offset_array_size * sizeof(uint32_t)), SEEK_SET);
    resource_layout_header resource_header = {0};
    fread(&resource_header, sizeof(resource_layout_header), 1, alr);
    resource_entry* resources = calloc(resource_header.array_size, sizeof(resource_entry));
    if (resources == NULL) {
        log_error(CRITICAL, "split_alr(): Failed to allocate memory for resource entry table (%d bytes)\n", resource_header.array_size * sizeof(resource_entry));
    }
    else {
        fread(resources, sizeof(resource_entry), resource_header.array_size, alr);
        for (uint32_t i = 0; i < resource_header.array_size; i++) {
            uint32_t res_size = 0;
            if (i == resource_header.array_size - 1) {
                res_size = header.last_resource_end - resources[i].data_ptr;
            }
            else {
                res_size = resources[i + 1].data_ptr - resources[i].data_ptr; // next_offset - current_offset
            }
            // 256 MiB
            if (res_size > 0x10000000) {
                log_error(CRITICAL, "split_alr(): Size of resource %d was unreasonably high (%d bytes)!\n", i, res_size);
                return false;
            }
            char* buffer = calloc(1, res_size);
            if (buffer == NULL) {
                log_error(CRITICAL, "split_alr(): Failed to allocate %d bytes for resource #%d\n", res_size, i);
            }
            else {
                fseek(alr, header.resource_offset + resources[i].data_ptr, SEEK_SET);
                fread(buffer, res_size, 1, alr);
                // Because i is a uint32_t, the longest possible filename is resource_4294967295.bin, which is 23 characters
                char filename[32];
                if (snprintf(filename, 24, "resource_%u.bin", i) > 0) {
                    log_error(INFO, "Resource %d: %d (0x%x) bytes\n", i, res_size, res_size);
                    FILE* dump = fopen(filename, "wb");
                    fwrite(buffer, res_size, 1, dump);
                    fclose(dump);
                }
            }
        }
    }
	fclose(alr);
	return true;
}

// Function to generically skip over any chunk
static void chunk_skip(FILE* file) {
    chunk_generic chunk = {0};
    fread(&chunk, sizeof(chunk), 1, file);

    // Skip to the end of the chunk
    fseek(file, chunk.size, SEEK_CUR);
}

// chunk_skip() but with some error checking
static void chunk_0x0(FILE* file) {
    chunk_generic chunk = {0};
    fread(&chunk, sizeof(chunk), 1, file);

    if (chunk.size != 8) {
        log_error(DEBUG, "chunk_0x0(): Non-empty chunk! Size is %d bytes rather than the usual 8 bytes\n", chunk.size);
    }

    // Skip to the end of the chunk
    fseek(file, chunk.size, SEEK_CUR);
}

// chunk_skip() but with some error checking
static void chunk_0xD(FILE* file) {
    chunk_generic chunk = {0};
    fread(&chunk, sizeof(chunk), 1, file);

    // This isn't really a *problem*, but the warning status helps it stand out
    if (chunk.size != 0xC) {
        log_error(WARNING, "chunk_0xD(): Non-empty chunk! Size is %d bytes rather than the usual 12 bytes\n", chunk.size);
    }

    // Skip to the end of the chunk
    fseek(file, chunk.size, SEEK_CUR);
}

// Extra checks for 0x16 chunks
static void chunk_0x16(FILE* file) {
    uint64_t chunk_start = ftell(file);

    resource_layout_header header = {0};
    fread(&header, sizeof(header), 1, file);

    if (header.array_size > 0) {
        log_error(INFO, "chunk_0x16(): Found %d resource entry(s).\n", header.array_size);
        resource_entry_0x16* entries = calloc(header.array_size, sizeof(*entries));
        fread(entries, sizeof(*entries), header.array_size, file);

        for (uint32_t i = 0; i < header.array_size; i++) {
            if (entries[i].pad != 0) {
                log_error(WARNING, "chunk_0x16(): Apparent padding at 0x8 in entry %d was 0x%08x, not 0.\n");
            }
            if (entries[i].pad2 != 0) {
                log_error(WARNING, "chunk_0x16(): Apparent padding at 0x18 in entry %d was 0x%08x, not 0.\n");
            }
            log_error(INFO, "0x16 resource %2d: flags 0x%08x, unknown3 0x%08x, unknown 0x%08x, unknown2 0x%08x, data_ptr 0x%08x\n", i, entries[i].flags, entries[i].unknown3, entries[i].unknown, entries[i].unknown2, entries[i].data_ptr);
        }

        free(entries);
    }
    // Jump to next chunk
    fseek(file, chunk_start + header.chunk_size, SEEK_SET);
}

// Reads 0x5 animation / mesh chunks
static void chunk_animation(FILE* file, flags options) {
    uint64_t chunk_start_pos = ftell(file);
    anim_header header = {0};
    fread(&header, sizeof(header), 1, file);

    keyframe_3* translation_keys = calloc(header.translation_key_count, header.translation_key_size);
    anim_rotation_keys* rotation_keys = calloc(header.rotation_key_count, sizeof(anim_rotation_keys));
    keyframe_3* scale_keys = calloc(header.scale_key_count, sizeof(keyframe_3));

    if (translation_keys == NULL || rotation_keys == NULL || scale_keys == NULL) {
        log_error(CRITICAL, "chunk_animation(): Failed to allocate memory for animation keys.\n");
        free(translation_keys);
        free(rotation_keys);
        free(scale_keys);
        fseek(file, chunk_start_pos + header.size, SEEK_SET); // Jump to next chunk
        return;
    }

    if (options.animation) {
        printf("\n");
        log_error(DEBUG, "chunk_animation(): Total Time: %f\n", header.total_time);
        log_error(DEBUG, "chunk_animation(): Geometry  %hi\n", header.unknown_settings1);
        log_error(DEBUG, "chunk_animation(): Flag      0x%04x\n", header.array_width_1);
        log_error(DEBUG, "chunk_animation(): Flag 2    0x%04x\n", header.translation_key_size);
        for (uint32_t i = 0; i < header.translation_key_count; i++) {
            log_error(INFO, "chunk_animation(): Translation Key frame %f: \t", translation_keys[i].frame, translation_keys[i].x, translation_keys[i].y, translation_keys[i].z);

            switch (header.translation_key_size) {
                case 0x8:
                    printf("%f ", ((keyframe_1*)translation_keys)[i].x);
                    break;
                case 0xC:
                    printf("%f %f ", ((keyframe_2*)translation_keys)[i].x, ((keyframe_2*)translation_keys)[i].y);
                    break;
                case 0x10:
                    printf("%f %f %f ", ((keyframe_3*)translation_keys)[i].x, ((keyframe_3*)translation_keys)[i].y, ((keyframe_3*)translation_keys)[i].z);
                    break;
            }
            printf("\n");
        }
        for (uint32_t i = 0; i < header.rotation_key_count; i++) {
            log_error(INFO, "chunk_animation(): Rotation Key frame:%f \t%04x %04x %04x\n", rotation_keys[i].frame, rotation_keys[i].unk1, rotation_keys[i].unk2, rotation_keys[i].unk3);
        }
        for (uint32_t i = 0; i < header.scale_key_count; i++) {
            log_error(INFO, "chunk_animation(): Scale Key frame:%f \t%f %f %f\n", scale_keys[i].frame, scale_keys[i].x, scale_keys[i].y, scale_keys[i].z);
        }
    }

    free(translation_keys);
    free(rotation_keys);
    free(scale_keys);
    fseek(file, chunk_start_pos + header.size, SEEK_SET); // Jump to next chunk
}

// Reads 0x10 chunks and uses their metadata to write texture data to disk
static void chunk_texture(FILE* file, unsigned int texture_buffer_ptr, image_type format, flags options) {
    uint64_t chunk_start_pos = ftell(file); // Save our place

    // Get contents of 0x15 sub-chunks
    chunk_layout file_header = {0};
    fseek(file, 0, SEEK_SET); // Jump to start of file
    fread(&file_header, sizeof(file_header), 1, file);
    fseek(file, file_header.chunk_size - sizeof(file_header), SEEK_CUR); // Jump to 0x15 chunk
    resource_layout_header res_header = {0};
    fread(&res_header, sizeof(res_header), 1, file);

    resource_entry* resources = calloc(res_header.array_size, sizeof(*resources));
    uint32_t buf_size = file_header.last_resource_end - file_header.resource_offset;
    uint8_t* texture_buffer = calloc(buf_size, 1);
    if (resources == NULL || texture_buffer == NULL) {
        free(resources);
        free(texture_buffer);
        return;
    }
    fread(&resources, sizeof(*resources), res_header.array_size, file);

    // Read texture buffer
    fseek(file, file_header.resource_offset, SEEK_SET);
    fread(texture_buffer, buf_size, 1, file);

    // Go back to chunk
    fseek(file, chunk_start_pos, SEEK_SET);

    texture_metadata_header header = {0};
    fread(&header, sizeof(header), 1, file);
    log_error(INFO, "Surface Count: %d Image Count: %d\n\n", header.surface_count, header.texture_count);

    // DDS filenames + the mystery data attached to them are 0x20 long each
    char* dds_names = calloc(header.surface_count, 0x20);
    fread(dds_names, 0x20, header.surface_count, file);

    surface_info* surfaces = calloc(header.surface_count, sizeof(surface_info));
    texture_header* textures = calloc(header.texture_count, sizeof(texture_header));
    if (surfaces == NULL || textures == NULL) {
        free(surfaces);
        free(textures);
        return;
    }

    for (uint32_t i = 0; i < header.surface_count; i++) {
        uint32_t pixel_count = surfaces[i].width * surfaces[i].height;
        uint32_t texture_id = textures[i].index;
        uint32_t res_size = 0;
        if (i == header.surface_count - 1) {
            res_size = file_header.last_resource_end - resources[i].data_ptr;
        }
        else {
            res_size = resources[i + 1].data_ptr - resources[i].data_ptr; // next_offset - current_offset
        }

        uint8_t bits_per_pixel = (res_size / pixel_count) * 8;

        uint32_t total_pixel_count = full_pixel_count(surfaces[i].width, surfaces[i].height, surfaces[i].mipmap_count);

        log_error(INFO, "Surface %2d %-32s: %2d mipmap(s), estimated %2d bpp, 0x%05x pixels, %4dx%-4d (", i, &dds_names[i * 0x20], surfaces[i].mipmap_count, bits_per_pixel, total_pixel_count, surfaces[i].width, surfaces[i].height, res_size);

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
        printf(" buffer @ 0x%x)\n", texture_buffer_ptr + resources[i].data_ptr);

        if (resources[i].pad != 0) {
            log_error(DEBUG, "chunk_texture(): Resource meta (0x15) anomaly, apparent padding at sub-chunk %d was 0x%x\n", i, resources[i].pad);
        }
        if (resources[i].flags != 0x00040001) {
            log_error(DEBUG, "chunk_texture(): Resource meta (0x15) anomaly, ID at sub-chunk %d was 0x%x\n", i, resources[i].flags);
        }

        if (!options.info_mode) {
            texture_info info = {
                .filename = &dds_names[i * 0x20],
                .bits_per_pixel = bits_per_pixel,
                .mipmap_count = surfaces[i].mipmap_count,
                .image_data = (char *) texture_buffer + resources[i].data_ptr,
                .width = surfaces[i].width,
                .height = surfaces[i].height,
                .format = format
            };
            write_texture(info);
        }
    }
    printf("\n");

    uint32_t total_image_pixels = 0;
	for (unsigned int i = 0; i < header.texture_count; i++) {
            log_error(INFO, "%-32s: Width %4hi, Height %4hi (Surface %2d)\n", textures[i].filename, textures[i].width, textures[i].height, textures[i].index);
            total_image_pixels += textures[i].width * textures[i].height;
	}
    log_error(DEBUG, "Total pixel count for all textures: 0x%08x\n", total_image_pixels);

    // Cleanup
    free(surfaces);
    free(textures);
    free(texture_buffer);
    fseek(file, chunk_start_pos + header.size, SEEK_SET); // Set position to end of chunk
}

bool chunk_parse_all(char* alr_filename, flags options) {
    FILE* alr = fopen(alr_filename, "rb");
    if (alr != NULL) {
        struct stat st;
        if (stat(alr_filename, &st) != 0) {
            log_error(CRITICAL, "chunk_parse_all(): Failed to get file metadata for %s", alr_filename);
            return false;
        }
        unsigned int filesize = st.st_size;

        log_error(DEBUG, "chunk_parse_all(): Loaded %s (%d bytes)\n", alr_filename, filesize);

        uint8_t image_format = DDS;
        if (options.tga) { image_format = TGA; }

        chunk_layout header = {0};
        fread(&header, sizeof(header), 1, alr);
        uint32_t* offset_array = calloc(header.offset_array_size, sizeof(*offset_array));
        if (offset_array == NULL) {
            fclose(alr);
            return false;
        }
        fread(offset_array, sizeof(*offset_array), header.offset_array_size, alr);

        if (options.info_mode) {
            log_error(INFO, "Resource Section Offset: 0x%x\n", header.resource_offset);
            log_error(INFO, "Last Resource End Offset: 0x%x\n", header.last_resource_end);
            log_error(INFO, "File Count: %d\n\n", header.offset_array_size);
            if (options.layout) {
                for (unsigned int i = 0; i < header.offset_array_size; i++) {
                    log_error(INFO, "File %u: 0x%x\n", i, offset_array[i]);
                }
            }
        }
        for (unsigned int i = 0; i < header.offset_array_size; i++) {
            chunk_generic chunk = {0};
            fread(&chunk, sizeof(chunk), 1, alr);
            if (offset_array[i] == 0) {
                log_error(WARNING, "chunk_parse_all(): Offset %d was 0, skipping...\n", i);
                continue;
            }
            fseek(alr, offset_array[i], SEEK_SET);
            while (chunk.id != 0) {
                if (chunk.id > 0x16) {
                    log_error(WARNING, "chunk_parse_all(): Invalid chunk ID 0x%x at 0x%x (internal file %d, %s)!\n", chunk.id, ftell(alr), i, alr_filename);
                    return false;
                }
                if (options.info_mode && options.layout) {
                    log_error(DEBUG, "chunk_parse_all(): 0x%x chunk at 0x%x\n", chunk.id, ftell(alr));
                }

                switch (chunk.id) {
                    case 0x0:
                        chunk_0x0(alr); // Unreachable because of the while() loop
                        break;
                    case 0x5:
                        chunk_animation(alr, options);
                        break;
                    case 0xD:
                        chunk_0xD(alr);
                        break;
                    case 0x10:
                        chunk_texture(alr, header.resource_offset, image_format, options);
                        break;
                    case 0x16:
                        chunk_0x16(alr);
                        break;
                    default:
                        chunk_skip(alr);
                        break;
                }

                // Read next chunk header but don't move file pos
                fread(&chunk, sizeof(chunk), 1, alr);
                fseek(alr, sizeof(chunk) * -1.0f, SEEK_CUR);
            }
        }
        fclose(alr);
        free(offset_array);
    }
    else {
        log_error(CRITICAL, "chunk_parse_all(): Couldn't open %s.\n", alr_filename);
        return false;
    }
    return true;
}

