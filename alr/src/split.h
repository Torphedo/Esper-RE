#pragma once
#include <stdbool.h>
#include "alr.h"

// Writes each distinct section of an ALR to separate files on disk.
// bool split_alr(char* alr_filename);

void split_generic_chunk(chunk_generic chunk, u8* chunk_buf, u32 idx);
void split_resource(u8* buf, u32 size, u32 idx);

alr_interface split_interface = {
    .chunk_0x1 = split_generic_chunk,
    .chunk_0x2 = split_generic_chunk,
    .chunk_0x3 = split_generic_chunk,
    .chunk_0x4 = split_generic_chunk,
    .chunk_0x5 = split_generic_chunk,
    .chunk_0x6 = split_generic_chunk,
    .chunk_0x7 = split_generic_chunk,
    .chunk_0x8 = split_generic_chunk,
    .chunk_0x9 = split_generic_chunk,
    .chunk_0xA = split_generic_chunk,
    .chunk_0xB = split_generic_chunk,
    .chunk_0xC = split_generic_chunk,
    .chunk_0xD = split_generic_chunk,
    .chunk_0xE = split_generic_chunk,
    .chunk_0xF = split_generic_chunk,
    .chunk_0x10 = split_generic_chunk,
    .chunk_0x11 = split_generic_chunk,
    .chunk_0x12 = split_generic_chunk,
    .chunk_0x13 = split_generic_chunk,
    .chunk_0x14 = split_generic_chunk,
    .chunk_0x15 = split_generic_chunk,
    .chunk_0x16 = split_generic_chunk,

    .tex_handler = split_resource
};

