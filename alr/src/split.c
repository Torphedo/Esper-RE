#include <stdio.h>
#include <stdlib.h>

#include "filesystem.h"
#include "logging.h"
#include "alr.h"

void split_generic_chunk(void* ctx, chunk_generic chunk, u8* chunk_buf, u32 idx) {
    if (!dir_exists("resources")) {
        system("mkdir resources");
    }

    char filename[256] = {0};
    if (snprintf(filename, 256, "resources/%u.bin", idx) < 0) {
        LOG_MSG(error, "Couldn't print output name for resource %d\n", idx);
        return;
    }

    FILE* dump = fopen(filename, "ab"); // Append to the file.
    if (dump == NULL) {
        LOG_MSG(error, "Failed to open %s for appended writing!\n", filename);
        return;
    }
    fwrite(chunk_buf, chunk.size, 1, dump); 
    fclose(dump);
}

void split_resource(void* ctx, u8* buf, u32 size, u32 idx) {
    if (!dir_exists("resources")) {
        system("mkdir resources");
    }

    char filename[256] = {0};
    if (snprintf(filename, 256, "resources/resource_%u.bin", idx) < 0) {
        LOG_MSG(error, "Couldn't print output name for resource %d\n", idx);
        return;
    }

    FILE* dump = fopen(filename, "wb");
    if (dump == NULL) {
        LOG_MSG(error, "Failed to open %s for writing!\n", filename);
        return;
    }
    fwrite(buf, size, 1, dump);
    fclose(dump);
}

