#pragma once
#include "alr.h"

// Stub function to skip chunks we don't care about for texture dumping.
static void chunk_any(chunk_generic chunk, u8* chunk_buf, u32 idx) {
    return;
}

// Sanity checks for 0xD chunks that ought to be empty.
void chunk_0xD(chunk_generic chunk, u8* chunk_buf, u32 idx);

// 0x10 (Texture metadata) chunk handling. Saves metadata to dump.c's internal
// state.
void chunk_texture(chunk_generic header, u8* chunk_buf, u32 idx);

// 0x15 (Resource/texture buffer layout) chunk handling. Saves information to
// dump.c's internal state.
void res_layout(chunk_generic chunk, u8* chunk_buf, u32 idx);

// This is run for each resource/texture buffer and dumps it to a DDS file as
// best it can. Metadata from 0x10 chunks will be used if available, and
// estimates from 0x15 chunks will be used as a fallback.
void process_texture(u8* buf, u32 size, u32 idx);

// Interface to call parse_alr() with to trigger texture dumping.
const alr_interface dump_interface = {
    .chunk_0x1 = chunk_any,
    .chunk_0x2 = chunk_any,
    .chunk_0x3 = chunk_any,
    .chunk_0x4 = chunk_any,
    .chunk_0x5 = chunk_any,
    .chunk_0x6 = chunk_any,
    .chunk_0x7 = chunk_any,
    .chunk_0x8 = chunk_any,
    .chunk_0x9 = chunk_any,
    .chunk_0xA = chunk_any,
    .chunk_0xB = chunk_any,
    .chunk_0xC = chunk_any,
    .chunk_0xD = chunk_0xD,
    .chunk_0xE = chunk_any,
    .chunk_0xF = chunk_any,
    .chunk_0x10 = chunk_texture,
    .chunk_0x11 = chunk_any,
    .chunk_0x12 = chunk_any,
    .chunk_0x13 = chunk_any,
    .chunk_0x14 = chunk_any,
    .chunk_0x15 = res_layout,
    .chunk_0x16 = chunk_any,

    .tex_handler = process_texture
};

