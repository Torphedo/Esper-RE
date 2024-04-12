#pragma once
#include "alr.h"

// Stub to fill out the interface
// TODO: Allow NULL interface functions and replace them with a stub in the alr
// functions at runtime. Will make the declarations much simpler and clearer.
static void replace_chunk(void* ctx, chunk_generic chunk, u8* chunk_buf, u32 idx) {
    return;
}

void replace_texture(void* ctx, u8* buf, u32 size, u32 idx);

// Interface to call parse_alr() with to trigger texture replacement
const alr_interface replace_interface = {
    .chunk_handlers = {
        replace_chunk,
        replace_chunk,
        replace_chunk,
        replace_chunk,
        replace_chunk,
        replace_chunk,
        replace_chunk,
        replace_chunk,
        replace_chunk,
        replace_chunk,
        replace_chunk,
        replace_chunk,
        replace_chunk,
        replace_chunk,
        replace_chunk,
        replace_chunk,
        replace_chunk,
        replace_chunk,
        replace_chunk,
        replace_chunk,
        replace_chunk,
        replace_chunk,
        replace_chunk
    },

    .tex_handler = replace_texture
};

