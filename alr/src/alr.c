#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <stdbool.h>

#include "logging.h"

#include "int_shorthands.h"
#include "filesystem.h"
#include "alr.h"

bool alr_edit(char* alr_filename, char* out_filename, flags options, alr_interface handlers) {
    FILE* alr = fopen(alr_filename, "rb");
    FILE* alr_out = fopen(out_filename, "wb");
    if (alr == NULL || alr_out == NULL) {
        LOG_MSG(error, "The input or output file failed to open: %s->%s.\n", alr_filename, out_filename);
        return false;
    }

    LOG_MSG(debug, "Starting ALR edit with output file %s\n", out_filename);
    LOG_MSG(debug, "Loading %s (%d bytes)\n", alr_filename, filesize(alr_filename));

    // Start with a fake 0x0 chunk
    chunk_generic chunk = {
        .id = 0,
        .size = 8
    };
    u32 texbuf_offset = 0;
    u32 texbuf_size = 0;
    u32 tex_entry_count = 0;
    resource_entry* entries = NULL;

    while (chunk.size > 0) {
        fread(&chunk, sizeof(chunk), 1, alr);

        // Trying to allocate with 0 size will cause lots of problems
        if (chunk.size == 0) {
            break;
        }

        // Don't try to read 0 bytes, it also causes problems
        if (chunk.size - sizeof(chunk) == 0) {
            continue;
        }

        // LOG_MSG(debug, "Got chunk id %d with size 0x%X @ 0x%X\n", chunk.id, chunk.size, ftell(alr) - sizeof(chunk));
        u8* chunk_buf = calloc(1, chunk.size);
        if (chunk_buf == NULL) {
            // We probably got off-track and read the wrong value as the size
            // somehow.
            long pos = ftell(alr);
            LOG_MSG(error, "Failed to allocate %d bytes for chunk buffer at 0x%X\n", chunk.size, pos);
            break;
        }

        fread(chunk_buf, chunk.size - sizeof(chunk), 1, alr);

        // Special handling for alr and texture header chunks
        switch(chunk.id) {
            case 0x11:
                // This scope is required by some compilers to declare new
                // variables in a switch case. Needed to compile with "zig cc".
                {
                    // Save resource offset for later
                    alr_header* header = (alr_header*)chunk_buf;
                    texbuf_offset = header->resource_offset;
                    texbuf_size = header->resource_size;

                    if (handlers.chunk_handlers[0x11] != NULL) {
                        (handlers.chunk_handlers[0x11])(chunk, chunk_buf, 0);
                    }
                }
                break;
            case 0x15:
                // Save texture entry info for later
                tex_entry_count = *(u32*)chunk_buf;
                u32 entries_size = tex_entry_count * sizeof(resource_entry);
                u8* entries_buf = (chunk_buf + sizeof(u32));

                // Copy entry data to new buffer to be used & freed later.
                entries = calloc(1, entries_size);
                if (entries == NULL) {
                    LOG_MSG(error, "Couldn't allocate %d bytes for texture entries\n", entries_size);
                }
                memcpy(entries, entries_buf, entries_size);

                if (handlers.chunk_handlers[0x15] != NULL) {
                    (handlers.chunk_handlers[0x15])(chunk, chunk_buf, 0);
                }
                break;
        }

        if (chunk.id > ALR_MAX_CHUNK_ID) {
            LOG_MSG(error, "Invalid chunk ID 0x%X at 0x%X\n", chunk.id, ftell(alr));
            return false;
        }

        // Call handler functions through the interface
        if (handlers.chunk_handlers[chunk.id] != NULL) {
            (handlers.chunk_handlers[chunk.id])(chunk, chunk_buf, 0);
        }

        // Copy (potentially modified) chunk to output file
        fwrite(&chunk, sizeof(chunk), 1, alr_out);
        fwrite(chunk_buf, chunk.size - sizeof(chunk), 1, alr_out);

        free(chunk_buf);
    }

    // Read in the texture buffer
    u8* tex_buf = calloc(1, texbuf_size);
    if (tex_buf == NULL) {
        LOG_MSG(error, "Failed to allocate %d bytes for texture buffer.\n", texbuf_size);
        fclose(alr);
        return false;
    }
    fseek(alr, texbuf_offset, SEEK_SET);
    fread(tex_buf, texbuf_size, 1, alr);
    fclose(alr); // At this point we're done with the input, and can close it
    LOG_MSG(debug, "Read 0x%X bytes into texture buffer\n", texbuf_size);
    LOG_MSG(debug, "%d texture entries\n", tex_entry_count);

    for (u32 i = 0; i < tex_entry_count; i++) {
        LOG_MSG(debug, "data_ptr = 0x%X\n", entries[i].data_ptr);
        u32 tex_size = 0;
        if (i == (tex_entry_count - 1)) {
            // end - current
            tex_size = texbuf_size - entries[i].data_ptr;
        }
        else {
            // next - current
            tex_size = entries[i + 1].data_ptr - entries[i].data_ptr;
        }
        u8* cur_tex = tex_buf + entries[i].data_ptr;

        // Call handler to maybe modify this texture
        if (handlers.tex_handler != NULL) {
            (handlers.tex_handler)(cur_tex, tex_size, i);
        }

        // Write (maybe modified) texture to ouptut file
        // Position needs to be 1 before the intended address to start writing
        u32 tex_offset = texbuf_offset + entries[i].data_ptr - 1;
        LOG_MSG(debug, "writing %d bytes @ 0x%X for texture %d\n", tex_size, tex_offset, i);
        LOG_MSG(debug, "texbuf_offset = 0x%X, data_ptr = 0x%X\n", texbuf_offset, entries[i].data_ptr);
        fseek(alr_out, tex_offset, SEEK_SET);
        fwrite(cur_tex, tex_size, 1, alr_out);
    }

    // Can't forget to free :P
    free(entries);
    free(tex_buf);

    fclose(alr_out);

    return true;
}

bool alr_parse(char* alr_filename, flags options, alr_interface handlers) {
    FILE* alr = fopen(alr_filename, "rb");
    if (alr == NULL) {
        LOG_MSG(error, "Couldn't open %s.\n", alr_filename);
        return false;
    }

    LOG_MSG(debug, "Loading %s (%d bytes)\n", alr_filename, filesize(alr_filename));

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
    if (handlers.chunk_handlers[0x15] != NULL) {
        (handlers.chunk_handlers[0x15])(res_chunk_header, res_chunk_buf, 0);
    }

    // First offset generally points right after the resource layout chunk.
    // This is just to alert us of anomalies.
    if (offset_array[0] != ftell(alr)) {
        LOG_MSG(warning, "Gap between first data chunk & offset!\n");
        LOG_MSG(debug, "data chunk = 0x%X, offset = 0x%X\n", offset_array[0], ftell(alr));
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
    LOG_MSG(info, "texbuf is 0x%X bytes @ 0x%X, holding %d textures\n", header.resource_size, header.resource_offset, res_header.array_size);
    LOG_MSG(info, "%d internal files\n\n", header.offset_array_size);

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
                LOG_MSG(error, "Failed to allocate 0x%X bytes for chunk buffer!\n", buf_size);
                break;
            }
            fread(chunk_buf, buf_size, 1, alr);

            // Jump to the next chunk regardless of how our previous reads went
            fseek(alr, chunk_start + chunk.size, SEEK_SET);

            if (chunk.id > ALR_MAX_CHUNK_ID) {
                LOG_MSG(error, "Invalid chunk ID 0x%X at 0x%X (internal file %d, %s)!\n", chunk.id, ftell(alr), i, alr_filename);
                return false;
            }

            // This clutters up the log a lot.
            // LOG_MSG(debug, "0x%X chunk @ 0x%X\n", chunk.id, ftell(alr));

            if (handlers.chunk_handlers[chunk.id] != NULL) {
                (handlers.chunk_handlers[chunk.id])(chunk, chunk_buf, 0);
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

        if (handlers.tex_handler != NULL) {
            (handlers.tex_handler)(cur_tex, tex_size, i);
        }
    }

    free(tex_buf);
    free(res_chunk_buf);
    fclose(alr);

    return true;
}

