#pragma once
#include "alr.h"

// Stub function to skip chunks we don't care about for texture dumping.
static void chunk_any(void* ctx, chunk_generic chunk, u8* chunk_buf, u32 idx) {
    return;
}

// Sanity checks for 0xD chunks that ought to be empty.
void chunk_0xD(void* ctx, chunk_generic chunk, u8* chunk_buf, u32 idx);

// 0x10 (Texture metadata) chunk handling. Saves metadata to dump.c's internal
// state.
void chunk_texture(void* ctx, chunk_generic header, u8* chunk_buf, u32 idx);

// 0x15 (Resource/texture buffer layout) chunk handling. Saves information to
// dump.c's internal state.
void res_layout(void* ctx, chunk_generic chunk, u8* chunk_buf, u32 idx);

// This is run for each resource/texture buffer and dumps it to a DDS file as
// best it can. Metadata from 0x10 chunks will be used if available, and
// estimates from 0x15 chunks will be used as a fallback.
void process_texture(void* ctx, u8* buf, u32 size, u32 idx);

// Interface to call parse_alr() with to trigger texture dumping.
const alr_interface dump_interface = {
    .chunk_handlers = {
        chunk_any,
        chunk_any,
        chunk_any,
        chunk_any,
        chunk_any,
        chunk_any,
        chunk_any,
        chunk_any,
        chunk_any,
        chunk_any,
        chunk_any,
        chunk_any,
        chunk_any,
        chunk_0xD,
        chunk_any,
        chunk_any,
        chunk_texture,
        chunk_any,
        chunk_any,
        chunk_any,
        chunk_any,
        res_layout,
        chunk_any
    },

    .tex_handler = process_texture
};

