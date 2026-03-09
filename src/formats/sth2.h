#pragma once
#ifdef __cplusplus
extern "C" {
#endif

#include "shut_up_msvc.h"
#include <assert.h>
#include <stdbool.h>
#include "data_types.h"

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
static_assert(sizeof(sth2_header) == 0x40, "Wrong STH2 header size!");

typedef struct {
    u32 magic;
    u32 unknown;
    u32 num_offsets;
    u32 pad;

    s32 offsets[];
}sth2_real_header;
static_assert(sizeof(sth2_real_header) == 0x10, "Wrong STH2 'REAL' header size!");

typedef struct {
    u32 magic;
    u32 size;
    u32 num_entries;
    u32 unk1;
    u32 unk2;
    u32 unk3;
    u8 pad[0x18];
}trat_header;
static_assert(sizeof(trat_header) == 0x30, "Wrong STH2 'TRAT' header size!");

typedef struct {
    u32 magic;
    u8 unk5[0xFC];
}evnt_header;
static_assert(sizeof(evnt_header) == 0x100, "Wrong STH2 'EVNT' header size!");

typedef struct {
    u32 magic;
    u32 size;
    u32 num_entries;
    u32 pad1;
    u32 unk1;
    u32 unk2;
    u8 pad2[0x18];
    u32 offsets[];
}reat_header;
static_assert(sizeof(reat_header) == 0x30, "Wrong STH2 'REAT' header size!");

typedef struct {
    u32 magic;
    u32 size;
    u32 num_offsets;
    u32 unk2;
    u32 offsets[];
}sth2_wave_header;
static_assert(sizeof(sth2_wave_header) == 0x10, "Wrong STH2 'WAVE' header size!");

enum {
    PD_SAMPLE_RATE_UWP = 22050,
};

/// @brief Export a .bin (STH2) sound effect file to WAV
/// @param data File data
/// @param size File size
/// @param outpath Path to save the WAV file
/// @param sample_rate Expected audio sample rate in Hz
bool extract_sth2(const u8* data, u32 size, const char* outpath, u32 sample_rate);

#ifdef __cplusplus
}
#endif