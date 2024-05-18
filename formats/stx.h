#pragma once
// Most of this information comes from vgmstream's sthd decoder:
//      https://github.com/vgmstream/vgmstream/blob/master/src/meta/sthd.c
// However, their information is a bit scattered and they don't have any usable
// C structures.

#include <common/int.h>
#include <common/filesystem.h>

enum {
    STX_MAGIC = MAGIC('S', 'T', 'H', 'D'),
    STX_MAX_CHANNELS = 8,
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
    u16 channel_size; // In bytes, per channel (so double to get full size)
    u16 block_number; // Always block_idx + 1, probably for convenience.
    u16 loop_start_block;
    u16 loop_end_block;
    u16 pad2;
}stx_block_header;

// After the first (header) block, there are 8 of this structure.
typedef struct {
    u32 sample_rate; // Measured in Hz
    // Not always set, speculated to be volume as a percentage of UINT16_MAX
    // Labelled as both "volume" and "pan" in vgmstream comments
    u16 volume1;
    u16 volume2;

    // Makes structure 0x40 bytes. Could also be more fields that we've only
    // seen set to 0.
    u16 padding[0x1C];
}stx_channel;

// This is a shortcut to read all the header information of an STX file.
// Address of next block will be header.offset.next_block
typedef struct {
    stx_block_header header;
    stx_channel channels[STX_MAX_CHANNELS];
    unsigned char channel_names[STX_MAX_CHANNELS][0x20];
}stx_first_block;

