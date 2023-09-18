#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <malloc.h>

#include "logging.h"
#include "alr.h"

// Writes each distinct section of an ALR to separate files on disk
bool split_alr(char* alr_filename) {
    FILE* alr = fopen(alr_filename, "rb");
    if (alr == NULL) {
        LOG_MSG(error, "Failed to open %s\n", alr_filename);
        return false;
    }

    chunk_layout header = {0};
    // Read header
    fread(&header, sizeof(chunk_layout), 1, alr);
    uint32_t* pointer_array = malloc(header.offset_array_size * sizeof(uint32_t));
    if (pointer_array == NULL) {
        LOG_MSG(error, "Failed to allocate pointer array with %d elements\n", header.offset_array_size);
        fclose(alr);
        return false;
    }
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
            LOG_MSG(warning, "Adjusting for negative chunk size at chunk %d.\n", i);
            size *= -1;
        }

        // 256MiB
        if (size > 0x10000000) {
            LOG_MSG(error, "Chunk size at chunk %d was unreasonably high (%d bytes)!\n", i, size);
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

    // Dump all resources to separate files
    fseek(alr, sizeof(chunk_layout) + (header.offset_array_size * sizeof(uint32_t)), SEEK_SET);
    resource_layout_header resource_header = {0};
    fread(&resource_header, sizeof(resource_layout_header), 1, alr);
    resource_entry* resources = calloc(resource_header.array_size, sizeof(resource_entry));
    if (resources == NULL) {
        LOG_MSG(error, "Failed to allocate memory for resource entry table (%d bytes)\n", resource_header.array_size * sizeof(resource_entry));
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
                LOG_MSG(error, "Size of resource %d was unreasonably high (%d bytes)!\n", i, res_size);
                return false;
            }
            char* buffer = calloc(1, res_size);
            if (buffer == NULL) {
                LOG_MSG(error, "Failed to allocate %d bytes for resource #%d\n", res_size, i);
            }
            else {
                fseek(alr, header.resource_offset + resources[i].data_ptr, SEEK_SET);
                fread(buffer, res_size, 1, alr);
                // Because i is a uint32_t, the longest possible filename is resource_4294967295.bin, which is 23 characters
                char filename[32];
                if (snprintf(filename, 24, "resource_%u.bin", i) > 0) {
                    LOG_MSG(info, "Resource %d: %d (0x%x) bytes\n", i, res_size, res_size);
                    FILE* dump = fopen(filename, "wb");
                    fwrite(buffer, res_size, 1, dump);
                    fclose(dump);
                }
            }
        }
        free(resources);
    }
    fclose(alr);
    return true;
}

