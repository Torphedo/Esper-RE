#pragma once
#include <common/int.h>

typedef struct {
    u32 magic;
    u32 size;
    u32 unk1;
    u32 unk2;

    u8 pad[16];
    u32 unk3;
    u32 unk4;
    u32 wave_offset;

    u32 unk5[5];
}sth2_header;

typedef struct {
    u32 magic;
    u32 size;
    u32 num_offsets;
    u32 unk2;
    u32 offsets[];
}sth2_wave_header;

enum {
    PD_SAMPLE_RATE_UWP = 22050,
};
