#include <stdlib.h>
#include <stdio.h>
#include <string.h>

#include <common/logging.h>
#include <common/int.h>
#include <common/filesystem.h>

// Size of DDS header and pixel format header
#define DDS_HEADER_SIZE (0x7F)

u32 u32_min(u32 x, u32 y) {
    if (x < y) {
        return x;
    }
    return y;
}

void replace_texture(void* ctx, u8* buf, u32 size, u32 idx) {
    char* path = (char*)ctx;
    u32 dot_idx = 0;
    u32 name_offset = 0;
    for (u32 i = strlen(path); i > 0; i--) {
        if (path[i] == '.') {
            dot_idx = i;
        }
        if (is_dirsep(path[i])) {
            name_offset = i;
            break;
        }
    }
    path[dot_idx] = 0x00;
    char filename[256] = {0};
    snprintf(filename, sizeof(filename), "textures/%s_%d.dds", path + name_offset, idx);
    path[dot_idx] = '.';

    // Make the directory if it doesn't exist.
    if (file_exists(filename)) {
        u32 tex_size = filesize(filename);

        // Load the mod texture data
        u8* mod_dds = file_load(filename);
        if (mod_dds == NULL) {
            LOG_MSG(error, "Failed to load %d bytes from %s\n", tex_size, filename);
        } else {
            // Raw buffer without DDS header
            u8* raw_mod_tex = mod_dds + DDS_HEADER_SIZE;

            // Copy as much as will fit in the buffer or as much as possible
            // from the file, whichever is safer
            u32 copy_size = u32_min(size, tex_size - DDS_HEADER_SIZE);
            LOG_MSG(info, "Replacing %d bytes in texture %d\n", copy_size, idx);
            memcpy(buf, raw_mod_tex, copy_size);
            free(mod_dds); // Can't forget to free :P
        }
    } else {
        LOG_MSG(info, "%s not found, skipping\n", filename);
    }
}

