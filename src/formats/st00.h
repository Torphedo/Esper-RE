#pragma once
#include <assert.h>
#include <common/int.h>
#include <common/file.h>
#include "alr.h" // For vector struct

// Object ID 0 is the mesh at this index in the ALR file
static const u32 FIRST_OBJ_IDX = 8;
static const u32 st00_magic = MAGIC('S', 'T', '0', '0');
static const u32 nm00_magic = MAGIC('N', 'M', '0', '0');
static const u32 ps01_magic = MAGIC('P', 'S', '0', '1');
static const u32 cp00_magic = MAGIC('C', 'P', '0', '0');

typedef struct {
    s32 object_id;
    u32 unk1;
    u32 pad;
    // Object position
    vec3f pos;
    // Euler rotation in radians
    vec3f rotation;
}ps01_entry;
static_assert(sizeof(ps01_entry) == 0x24, "Wrong PS01 entry size!");

typedef struct {
    vec3f player_pos;
    float unk;
    float unk2;
    vec3f capsules[3];
    float unk3; // Often NaN (FF FF FF FF)
    u32 unk4;
    u32 unk5;
    u32 unk6;
    u32 unk7;
    float unk8;
}cp00_entry;
static_assert(sizeof(cp00_entry) == 0x50, "Wrong CP00 entry size!");

typedef struct {
    u32 magic; // 'CP00'
    u16 num_entries;
    u16 array_size; // num_entries * sizeof(cp00_entry)
    cp00_entry entries[];
}cp00_t;
// Flexible array member isn't counted in size
static_assert(sizeof(cp00_t) == 0x8, "Wrong CP00 header size!");

typedef struct {
    u16 unk[4];
    vec3f pos;
}oc00_entry;
static_assert(sizeof(oc00_entry) == 0x14, "Wrong OC00 entry size!");

typedef struct {
    u32 magic; // 'OC00'
    u8 unk1[4];
    u32 size;
    u32 pad;
    float unk_pos[10];
    u16 num_entries;
    u16 unk2[3];
}oc00_t;
static_assert(sizeof(oc00_t) == 0x40);

// Offsets in this header are set to -1 if the thing they point to doesn't exist
// in that file. In st00, they're set to 0 instead.
// Some offsets point to things that only exist in st09 area files, and are set
// to -1 or 0 in all other files.
typedef struct {
    u32 magic;
    s32 chunk_size;
    // Only used in st09 area files. Only set to 1, 2, 3, or 9.
    s32 unk1;
    // Points to the byte after the 'PS01' magic (if it exists).
    s32 ps01_offset;
    // Only used in st24, where it's set to 0x140. This is a common header size
    // in some other files, but st24 has a header size of -1 for some reason.
    s32 unk2;
    // In area files only, points to an ALR-like structure starting with u32(0x20).
    s32 unk_area;
    // In area files only, points to an 0x9 ALR chunk (followed by an 0x0 chunk).
    s32 unk_area2;
    // Only used in st09_02.dat, where it points to an 0x20 chunk.
    s32 unk_area3;
    // Only used in st00, where the 2nd value points to the end of the file
    s32 unk3[4];
    s32 ps00_count; // Each entry is 0x24 bytes (the count may be -1)
    // Only used in st00
    s32 unk4;
    s32 ps01_count;
    // Usually points to the end of the file (== file size), or -1.
    // Has some other unrelated value in st00 and st24.
    s32 unk5;
    s32 unk6[7];
    s32 nm00_offset;
    s32 nm00_count;

    // These all seem to be offsets to chunks of data
    s32 OC00_offset;
    s32 OC01_offset;
    s32 OC02_offset;

    s32 OE00_offset;
    s32 EF00_offset1;

    s32 OE02_offset;
    s32 OA00_offset;
    s32 OA01_offset;

    s32 OA02_offset;
    s32 unk7[2];
    s32 OB00_offset;
    s32 unk8;
    s32 EF00_offset2;
    s32 SP00_offset;
    s32 MD00_offset;

    s32 CP00_offset1;
    s32 CP00_offset2;
    s32 CP00_offset3;
    s32 unk9[2];
    s32 CP00_offset4;
    s32 CP00_offset5;
    s32 unkA[31];
}st00_t;
static_assert(sizeof(st00_t) == 0x13C, "Map header size is wrong!");
