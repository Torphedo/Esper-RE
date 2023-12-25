#include <stdlib.h>
#include <stdio.h>
#include <string.h>

#include "logging.h"

#include "int_shorthands.h"
#include "filesystem.h"
#include "alr.h"

// Size of DDS header and pixel format header
#define DDS_HEADER_SIZE (0x7F)

u32 u32_min(u32 x, u32 y) {
    if (x < y) {
        return x;
    }
    return y;
}

void replace_texture(u8* buf, u32 size, u32 idx) {
    char path[256] = {0};
    snprintf(path, sizeof(path), "textures/%d.dds", idx);
    if (file_exists(path)) {
        u32 tex_size = filesize(path);

        // Load the mod texture data
        u8* mod_dds = file_load(path);
        if (mod_dds == NULL) {
            LOG_MSG(error, "Failed to load %d bytes from %s\n", tex_size, path);
        }
        else {
            // Raw buffer without DDS header
            u8* raw_mod_tex = mod_dds + DDS_HEADER_SIZE;

            // Copy as much as will fit in the buffer or as much as possible
            // from the file, whichever is safer
            u32 copy_size = u32_min(size, tex_size - DDS_HEADER_SIZE);
            LOG_MSG(info, "Replacing %d bytes in texture %d\n", copy_size, idx);
            memcpy(buf, raw_mod_tex, copy_size);
            free(mod_dds); // Can't forget to free :P
        }
    }
    else {
        LOG_MSG(info, "%s not found, skipping\n", path);
    }
}

