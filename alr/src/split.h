#pragma once
#include "alr_interface.h"

// Write chunks to separate files based on their index in the header's offset
// array.
void split_generic_chunk(void* ctx, chunk_generic chunk, u8* chunk_buf, u32 idx);

// Write each texture buffer to its own file. Useful for pasting into image
// preview tools, and hand-crafting your own image headers.
void split_resource(void* ctx, u8* buf, u32 size, u32 idx);

// Interface to call parse_alr() with to trigger splitting behaviour.
alr_interface split_interface = {
    .chunk_handlers = {
        split_generic_chunk,
        split_generic_chunk,
        split_generic_chunk,
        split_generic_chunk,
        split_generic_chunk,
        split_generic_chunk,
        split_generic_chunk,
        split_generic_chunk,
        split_generic_chunk,
        split_generic_chunk,
        split_generic_chunk,
        split_generic_chunk,
        split_generic_chunk,
        split_generic_chunk,
        split_generic_chunk,
        split_generic_chunk,
        split_generic_chunk,
        split_generic_chunk,
        split_generic_chunk,
        split_generic_chunk,
        split_generic_chunk,
        split_generic_chunk,
        split_generic_chunk
    },

    .tex_handler = split_resource
};

