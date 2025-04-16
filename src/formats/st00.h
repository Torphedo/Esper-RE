#pragma once
#include <assert.h>
#include <common/int.h>

static const char* st00_magic = "ST00";

typedef struct {
    u32 magic;
    s32 chunk_size;
    s32 unk1;
    s32 ps01_offset;
    s32 unk2[8];
    s32 entry_count; // Each entry is 0x24 bytes
    s32 unk3[3];
    s32 unk6[7];
    s32 nm00_offset;
    s32 string_count;

    // These all seem to be offsets to chunks of data
    s32 unk7[54];
}st00_t;
static_assert(sizeof(st00_t) == 0x13C);