#pragma once
#include "data_types.h"
// Do not use these structures with ALR files!
// These are for a slightly older format (about 2002 - 2003), when most of the
// ALR data was scattered in multiple files. The formatting is mostly the same,
// but the structures that are different are in this file.

// There's no size or format data, since the file just points to a DDS.
typedef struct {
    u32 unk1;
    u16 unk2;
    u16 unk3;
    u32 text1; // Texture filename without the ".dds" extension
    u32 text2;
}alm_texture_entry;

typedef struct {
    u32 id; // 6
    u32 size;
    u32 num_entries;
    alm_texture_entry entries[];
}alm_texture_header;

typedef struct {
    u32 id;
    u32 size;
    u8 format;
    u8 vert_size;
    u16 unk1;
    u32 unk2;
    u32 vert_count;
    u32 unk3;

    // There was no vertex buffer in ALM files, they were stored inline like
    // index buffers.
    u8 vertices[];
}alm_vertbuf;