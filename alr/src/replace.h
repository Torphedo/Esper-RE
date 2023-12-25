#pragma once
#include "alr.h"

void replace_chunk(chunk_generic chunk, u8* chunk_buf, u32 idx);

void replace_texture(u8* buf, u32 size, u32 idx);

// Interface to call parse_alr() with to trigger texture replacement
const alr_interface replace_interface = {
    .chunk_0x1 = replace_chunk,
    .chunk_0x2 = replace_chunk,
    .chunk_0x3 = replace_chunk,
    .chunk_0x4 = replace_chunk,
    .chunk_0x5 = replace_chunk,
    .chunk_0x6 = replace_chunk,
    .chunk_0x7 = replace_chunk,
    .chunk_0x8 = replace_chunk,
    .chunk_0x9 = replace_chunk,
    .chunk_0xA = replace_chunk,
    .chunk_0xB = replace_chunk,
    .chunk_0xC = replace_chunk,
    .chunk_0xD = replace_chunk,
    .chunk_0xE = replace_chunk,
    .chunk_0xF = replace_chunk,
    .chunk_0x10 = replace_chunk,
    .chunk_0x11 = replace_chunk,
    .chunk_0x12 = replace_chunk,
    .chunk_0x13 = replace_chunk,
    .chunk_0x14 = replace_chunk,
    .chunk_0x15 = replace_chunk,
    .chunk_0x16 = replace_chunk,

    .tex_handler = replace_texture
};

