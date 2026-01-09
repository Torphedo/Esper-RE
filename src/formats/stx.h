#pragma once
// Most of this information comes from vgmstream's sthd decoder:
//      https://github.com/vgmstream/vgmstream/blob/master/src/meta/sthd.c
// However, their information is a bit scattered and they don't have any usable
// C structures.

#ifdef __cplusplus
extern "C" {
#endif

#include <common/int.h>
#include "sth2.h"

enum {
    STX_MAGIC = MAGIC('S', 'T', 'H', 'D'),
    STX_MAX_CHANNELS = 8,
    STX_CHANNEL_NAME_SIZE = 0x20,
    STX_PC_SAMPLE_RATE = PD_SAMPLE_RATE_UWP * 2,

    // Use these if you want to be as close as possible to the PC files
    STX_PC_BUILD_DAY = 0x23,
    STX_PC_BUILD_MONTH = 0x03,
    STX_PC_BUILD_YEAR = 0x2017,

    // Number of samples per channel per block.
    STX_BLOCK_SAMPLES = 504,

    // The total number of samples per block, assuming 2 channels.
    STX_TOTAL_BLOCK_SAMPLES = STX_BLOCK_SAMPLES * 2,
};

// All offsets are relative.
typedef struct {
    u32 magic; // STX_MAGIC enum
    // In the first (header) block, this is the offset of the next block. In
    // all other blocks, it's the offset where the audio data starts.
    union {
        u16 start;
        u16 next_block;
    }offset;
    s16 channel_count; // Usually 2 (stereo L/R)

    // Build timestamp is just for reading in a hex editor. 9-12-2004 would be
    // 09 12 04 20, even though 0x12 = 18 and 0x2004 = 8196.
    // If you display the timestamp, format it in hexidecimal.
    u8 build_day;
    u8 build_month;
    u16 build_year;

    u32 unknown; // Usually 0 (see https://github.com/vgmstream/vgmstream/pull/1429)
    u16 block_count; // Total block count of the file
    u16 block_idx;
    u16 pad;
    u16 channel_size; // Measured in samples
    u16 block_number; // Always block_idx + 1, probably for convenience.
    u16 loop_start_block;
    u16 loop_end_block;
    u16 pad2;
}stx_block_header;

// After the first (header) block, there are 8 of this structure.
typedef struct {
    u32 sample_rate; // Measured in Hz
    u8 volume;
    // First byte is left side volume, second is right side.
    s8 pan[2];

    // Makes structure 0x40 bytes. Could also be more fields that we've only
    // seen set to 0.
    u16 padding[0x1C];
}stx_channel;

// This is a shortcut to read all the header information of an STX file.
// Address of next block will be header.offset.next_block
typedef struct {
    stx_block_header header;
    stx_channel channels[STX_MAX_CHANNELS];
    unsigned char channel_names[STX_MAX_CHANNELS][STX_CHANNEL_NAME_SIZE];
}stx_first_block;

// This is separate because we need struct sizes
enum {
    STX_BLOCK_SIZE = sizeof(stx_block_header) + STX_TOTAL_BLOCK_SAMPLES * sizeof(u16),
    STX_FIRST_OFFSET = STX_BLOCK_SIZE,
};

/// @brief Export an STX file to WAV
/// @param out_file Path to save WAV file
/// @param data STX data
/// @param size STX size
bool dump_stx(const char* out_file, const u8* data, u32 size);

/// @brief Generate an STX block header
///
/// The first block in the file gets special treatment in some fields, so make
/// sure to pass a block index of 0 to get a valid header.
/// @param total_num_blocks Total number of STX blocks in this file
/// @param idx Index of this block
/// @return Block header data
stx_block_header stx_block_create(u16 total_num_blocks, u16 idx);

/// @brief Calculate the number of blocks in the STX file
/// @param total_samples The total number of samples across all channels
static u32 stx_num_blocks_from_samples(u32 total_samples) {
    const u32 num_blocks = ALIGN_UP(total_samples, STX_TOTAL_BLOCK_SAMPLES) / STX_TOTAL_BLOCK_SAMPLES;
    return num_blocks;
}

/// @brief Calculate the expected size of an STX file
/// @param total_samples The total number of samples across all channels
static s64 stx_size_from_sample_count(u32 total_samples) {
    const u32 num_blocks = stx_num_blocks_from_samples(total_samples);
    return STX_FIRST_OFFSET + STX_BLOCK_SIZE * num_blocks;
}

#ifdef __cplusplus
}
#endif