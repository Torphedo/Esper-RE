#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <stdbool.h>

#include <common/logging.h>
#include <common/int.h>
#include <common/file.h>
#include <formats/pd_common.h>

#include "alr_interface.h"

void fix_alr_name(char* path) {
    u32 len = strlen(path);
    u32 last_slash_idx = 0;
    bool found_slash = false;
    for (u32 i = 0; i < len; i++) {
        char c = path[i];
        if (c == '/' || c == '\\') {
            last_slash_idx = i;
            found_slash = true;
        }
    }
    if (found_slash) {
        memmove(path, &path[last_slash_idx + 1], len - last_slash_idx - 1);
        path[len - last_slash_idx - 1] = 0x00;
    }
}

bool alr_edit(flags options, alr_interface handlers) {
    FILE* alr = fopen(options.input_path, "rb");
    FILE* alr_out = fopen(options.output_path, "wb");
    if (alr == NULL) {
        LOG_MSG(error, "The input file \"%s\" failed to open\n", options.input_path);
        return false;
    }
    if (alr_out == NULL && options.output_path != NULL) {
        LOG_MSG(error, "The output file failed to open: %s->%s.\n", options.input_path, options.output_path);
        return false;
    }

    if (alr_out != NULL) {
        LOG_MSG(debug, "Starting ALR edit with output file %s\n", options.output_path);
    }
    LOG_MSG(debug, "Loading %s (%d bytes)\n", options.input_path, file_size(options.input_path));

    // Read header
    chunk_layout header = {0};
    fread(&header, sizeof(header), 1, alr);
    if (header.chunk_size <= sizeof(chunk_generic) || header.id != 0x11) {
        return false;
    }
    if (header.texbuf_size == 0) {
        // Some ALRs don't set this field... not sure why.
        header.texbuf_size = file_size(options.input_path) - header.texbuf_offset;
    }
    fseek(alr, header.chunk_size, SEEK_SET); // Jump to next chunk
    fix_alr_name(options.input_path);

    // Read texture header
    resource_layout_header resheader = {0};
    fread(&resheader, sizeof(resheader), 1, alr);
    if (resheader.chunk_size <= sizeof(chunk_generic) || resheader.id != 0x15) {
        return false;
    }
    if (resheader.array_size == 0 && resheader.chunk_size > sizeof(resheader)) {
        resheader.array_size = (resheader.chunk_size - sizeof(resheader)) / sizeof(resource_entry);
    }
    const u32 entries_size = resheader.array_size * sizeof(resource_entry);

    // Read texture metadata entries
    resource_entry* entries = calloc(1, entries_size);
    if (entries == NULL) {
        LOG_MSG(error, "Failed to alloc %d bytes for texture entries\n", entries_size);
        return false;
    }
    if (resheader.array_size > 0) {
        fread(entries, entries_size, 1, alr);
        if (handlers.resheader_handler != NULL) {
            (handlers.resheader_handler)(resheader, entries);
        }
    }

    // Start with a fake 0x0 chunk
    chunk_generic chunk = {
        .id = 0,
        .size = 8
    };
    while (chunk.size > 0) {
        if (fread(&chunk, sizeof(chunk), 1, alr) == 0) {
            break; // EOF reached
        }

        // Since size includes the chunk size, a size any less than that will
        // try to read zero (or negative) bytes which doesn't make any sense.
        if (chunk.size < sizeof(chunk)) {
            break; // This is an error, something's going wrong if this happens
        } else if (chunk.size == sizeof(chunk)) {
            continue; // This is just an empty chunk, we can move on.
        }

        // LOG_MSG(debug, "Got chunk id %d with size 0x%X @ 0x%X\n", chunk.id, chunk.size, ftell(alr) - sizeof(chunk));
        u8* chunk_buf = calloc(1, chunk.size);
        if (chunk_buf == NULL || chunk.size == 0) {
            // We probably got off-track and read the wrong value as the size
            // somehow.
            const long pos = ftell(alr);
            LOG_MSG(error, "Failed to allocate %d bytes for chunk buffer at 0x%X\n", chunk.size, pos);
            break;
        }

        if (fread(chunk_buf, chunk.size - sizeof(chunk), 1, alr) == 0) {
            break; // EOF reached
        }

        if (chunk.id > ALR_MAX_CHUNK_ID) {
            LOG_MSG(error, "Invalid chunk ID 0x%X at 0x%X\n", chunk.id, ftell(alr));
            return false;
        }

        // Call handler functions through the interface
        if (handlers.chunk_handlers[chunk.id] != NULL) {
            (handlers.chunk_handlers[chunk.id])(options.input_path, chunk, chunk_buf, 0);
        }

        // Copy (potentially modified) chunk to output file
        if (alr_out != NULL) {
            fwrite(&chunk, sizeof(chunk), 1, alr_out);
            fwrite(chunk_buf, chunk.size - sizeof(chunk), 1, alr_out);
        }

        free(chunk_buf);
    }

    // Read in the texture buffer
    if (header.texbuf_size == 0) {
        // Can't forget to free :P
        if (resheader.array_size > 0) {
            free(entries);
        }
        if (alr_out != NULL) {
            fclose(alr_out);
        }

        return true; // Silently exit
    }
    u8* tex_buf = calloc(1, header.texbuf_size);
    if (tex_buf == NULL) {
        LOG_MSG(error, "Failed to allocate %d bytes for texture buffer.\n", header.texbuf_size);
        fclose(alr);
        return false;
    }
    fseek(alr, header.texbuf_offset, SEEK_SET);
    fread(tex_buf, header.texbuf_size, 1, alr);
    fclose(alr); // At this point we're done with the input, and can close it
    LOG_MSG(debug, "Read 0x%X bytes into texture buffer\n", header.texbuf_size);
    LOG_MSG(debug, "%d texture entries\n", resheader.array_size);

    for (u32 i = 0; i < resheader.array_size; i++) {
        LOG_MSG(debug, "data_ptr = 0x%X\n", entries[i].data_ptr);
        u32 tex_size = 0;
        if (i == (resheader.array_size - 1)) {
            // end - current
            tex_size = header.texbuf_size - entries[i].data_ptr;
        } else {
            // next - current
            tex_size = entries[i + 1].data_ptr - entries[i].data_ptr;
        }
        u8* cur_tex = tex_buf + entries[i].data_ptr;

        decoded_text text = {0};
        decode_single32(text.data, entries[i].text1);
        decode_single32(&text.data[6], entries[i].text2);

        // Call handler to maybe modify this texture
        if (handlers.tex_handler != NULL) {
            (handlers.tex_handler)(options.input_path, cur_tex, tex_size, text.data, i);
        }

        // Write (maybe modified) texture to ouptut file
        // Position needs to be 1 before the intended address to start writing
        if (alr_out != NULL) {
            const u32 tex_offset = header.texbuf_offset + entries[i].data_ptr - 1;
            LOG_MSG(debug, "writing %d bytes @ 0x%X for texture %d\n", tex_size, tex_offset, i);
            LOG_MSG(debug, "texbuf_offset = 0x%X, data_ptr = 0x%X\n", header.texbuf_offset, entries[i].data_ptr);
            fseek(alr_out, tex_offset, SEEK_SET);
            fwrite(cur_tex, tex_size, 1, alr_out);
        }
    }

    // Can't forget to free :P
    if (resheader.array_size > 0) {
        free(entries);
    }
    free(tex_buf);

    if (alr_out != NULL) {
        fclose(alr_out);
    }

    return true;
}

void create_alr_tex_folder(char* alr_path) {
    // Find the position of the '.' in the filename.
    u32 dot_idx = 0;
    for (u32 i = strlen(alr_path); i > 0; i--) {
        if (alr_path[i] == '.') {
            dot_idx = i;
        }
    }
    // We temporarily replace the '.' with a null terminator so it's not in the
    // output filename.
    alr_path[dot_idx] = 0x00;

    // Make the directory if it doesn't exist.
    if (!path_is_dir("textures")) {
        system("mkdir textures");
    }

    char filename[256] = {0};
    char dirsep = '/';
#ifdef _WIN32
    dirsep = '\\';
#endif
    snprintf(filename, sizeof(filename), "textures%c%s", dirsep, alr_path);
    if (!path_is_dir(filename)) {
        snprintf(filename, sizeof(filename), "mkdir textures%c%s", dirsep, alr_path);
        system(filename);
    }

    alr_path[dot_idx] = '.'; // Put the file extension back
}
