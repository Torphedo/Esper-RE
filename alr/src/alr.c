#include <stdlib.h>
#include <stdio.h>
#include <stdbool.h>

#include "logging.h"

#include "int_shorthands.h"
#include "filesystem.h"
#include "alr.h"

bool alr_parse(char* alr_filename, flags options, alr_interface handlers) {
    FILE* alr = fopen(alr_filename, "rb");
    if (alr == NULL) {
        LOG_MSG(error, "Couldn't open %s.\n", alr_filename);
        return false;
    }

    LOG_MSG(debug, "Loaded %s (%d bytes)\n", alr_filename, filesize(alr_filename));

    // Read the file header & offset array
    chunk_layout header = {0};
    fread(&header, sizeof(header), 1, alr);
    u32* offset_array = calloc(header.offset_array_size, sizeof(*offset_array));
    if (offset_array == NULL) {
        fclose(alr);
        return false;
    }
    fread(offset_array, sizeof(*offset_array), header.offset_array_size, alr);

    // Get resource metadata. Basically same as the file header but for buffers
    // at the end of the file
    resource_layout_header res_header = {0};
    fread(&res_header, sizeof(res_header), 1, alr);
    u8* res_chunk_buf = calloc(1, (res_header.array_size * sizeof(resource_entry)) + sizeof(u32));
    if (res_chunk_buf == NULL) {
        free(offset_array);
        fclose(alr);
        return false;
    }
    resource_entry* entries = (resource_entry*)(res_chunk_buf + sizeof(u32));

    fseek(alr, sizeof(u32) * -1, SEEK_CUR);
    long pos = ftell(alr);
    fread(res_chunk_buf, (sizeof(*entries) * res_header.array_size) + sizeof(u32), 1, alr);

    chunk_generic res_chunk_header = *(chunk_generic*)&res_header;
    handlers.chunk_0x15(res_chunk_header, res_chunk_buf, 0);

    // First offset generally points right after the resource layout chunk.
    // This is just to alert us of anomalies.
    if (offset_array[0] != ftell(alr)) {
        LOG_MSG(warning, "Gap between first data chunk & offset!\n");
        LOG_MSG(debug, "data chunk = 0x%x, offset = 0x%x\n", offset_array[0], ftell(alr));
    }

    u32 tex_buf_size = header.resource_size;
    u8* tex_buf = calloc(1, tex_buf_size);
    if (tex_buf == NULL) {
        free(offset_array);
        free(res_chunk_buf);
        fclose(alr);
        return false;
    }
    fseek(alr, header.resource_offset, SEEK_SET);
    fread(tex_buf, tex_buf_size, 1, alr);

    // Print some useful info about the file's structure
    if (options.info_mode) {
        LOG_MSG(info, "Resource section = 0x%x\n", header.resource_offset);
        LOG_MSG(info, "Resources end @ 0x%x\n", header.resource_size);
        LOG_MSG(info, "%d resources\n", res_header.array_size);
        LOG_MSG(info, "%d internal files\n\n", header.offset_array_size);
        if (options.layout) {
            for (u32 i = 0; i < header.offset_array_size; i++) {
                LOG_MSG(info, "File %u: 0x%x\n", i, offset_array[i]);
            }
        }
    }

    for (u32 i = 0; i < header.offset_array_size; i++) {
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
            u64 chunk_start = ftell(alr);
            fread(&chunk, sizeof(chunk), 1, alr);
            if (chunk.id == 0) {
                break;
            }
            // Need to subtract sizeof(chunk) because the size includes size & ID
            u64 buf_size = chunk.size - sizeof(chunk);
            u8* chunk_buf = calloc(1, buf_size);
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
                case 0x1:
                    handlers.chunk_0x1(chunk, chunk_buf, i);
                    break;
                case 0x2:
                    handlers.chunk_0x2(chunk, chunk_buf, i);
                    break;
                case 0x3:
                    handlers.chunk_0x3(chunk, chunk_buf, i);
                    break;
                case 0x4:
                    handlers.chunk_0x4(chunk, chunk_buf, i);
                    break;
                case 0x5:
                    handlers.chunk_0x5(chunk, chunk_buf, i);
                    break;
                case 0x6:
                    handlers.chunk_0x6(chunk, chunk_buf, i);
                    break;
                case 0x7:
                    handlers.chunk_0x7(chunk, chunk_buf, i);
                    break;
                case 0x8:
                    handlers.chunk_0x8(chunk, chunk_buf, i);
                    break;
                case 0x9:
                    handlers.chunk_0x9(chunk, chunk_buf, i);
                    break;
                case 0xA:
                    handlers.chunk_0xA(chunk, chunk_buf, i);
                    break;
                case 0xB:
                    handlers.chunk_0xB(chunk, chunk_buf, i);
                    break;
                case 0xC:
                    handlers.chunk_0xC(chunk, chunk_buf, i);
                    break;
                case 0xD:
                    handlers.chunk_0xD(chunk, chunk_buf, i);
                    break;
                case 0xE:
                    handlers.chunk_0xE(chunk, chunk_buf, i);
                    break;
                case 0xF:
                    handlers.chunk_0xF(chunk, chunk_buf, i);
                    break;
                case 0x10:
                    handlers.chunk_0x10(chunk, chunk_buf, i);
                    break;
                // 0x11 is the header and isn't in the chunk loop
                case 0x12:
                    handlers.chunk_0x12(chunk, chunk_buf, i);
                    break;
                case 0x13:
                    handlers.chunk_0x13(chunk, chunk_buf, i);
                    break;
                case 0x14:
                    handlers.chunk_0x14(chunk, chunk_buf, i);
                    break;
                case 0x15:
                    handlers.chunk_0x15(chunk, chunk_buf, i);
                    break;
                case 0x16:
                    handlers.chunk_0x16(chunk, chunk_buf, i);
                    break;
                default:
                    LOG_MSG(error, "Unhandled chunk type 0x%x!\n", chunk.id);
                    break;
            }
        }

        // I would be very concerned to see a non-empty 0x0 chunk.
        if (chunk.id == 0 && chunk.size != 8) {
            LOG_MSG(debug, "Non-empty chunk! Size is %d bytes rather than the usual 8 bytes\n", chunk.size);
        }
    }
    free(offset_array);

    for (u32 i = 0; i < res_header.array_size; i++) {
        u32 tex_size = 0;
        if (i == res_header.array_size - 1) {
            // end - current
            tex_size = tex_buf_size - entries[i].data_ptr;
        }
        else {
            // next - current
            tex_size = entries[i + 1].data_ptr - entries[i].data_ptr;
        }
        u8* cur_tex = tex_buf + entries[i].data_ptr;

        handlers.tex_handler(cur_tex, tex_size, i);
    }

    free(tex_buf);
    free(res_chunk_buf);
    fclose(alr);

    return true;
}

