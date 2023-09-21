#pragma once
#include <stdint.h>

#include "int_shorthands.h"

// All ALR files begin with this structure.
typedef struct {
    u32 id;                // 0x11
    u32 chunk_size;        // Size of this entire chunk
    u32 flags;             // Unknown
    u32 resource_offset;   // Offset of resource section (at end of file)
    u32 offset_array_size; // Number of offsets in the array
    u32 resource_size; // Offset from resource_offset where the last resource ends. This should point to the end of the file
    u64 pad;
}chunk_layout;

// Next, there is an array of u32 absolute offsets to various chunks of
// data throughout the file. It's unclear why these are split into these large
// chunks or if there's any pattern to the grouping. The size listed in the
// chunk layout struct includes this array, which has a size of
// (chunk_layout.offset_array_size * 0x4).

// After the chunk layout structure, there is a (poorly understood) structure
// containing several absolute offsets into the resource section, which is
// known to store raw textures.

typedef struct {
    u32 id;         // 0x15
    u32 chunk_size; // Size of this entire chunk
    u32 array_size;
}resource_layout_header;

// Next, there are [array_size] instances of this structure:

typedef enum {
    FORMAT_RGBA8 = 0b0110,
    FORMAT_DXT5 = 0b1111,
}alr_pixel_format;

typedef struct {
    u32 flags;    // Unknown, always 01 00 04 00 so far
    u32 data_ptr; // Offset to some data in the resource section (relative to chunk_layout.resource_offset)
    u32 pad;      // Always 0 (so far)
    u8 unknown; // Usually 0x29
    u8 pixel_format; // May be mip count, unsure.
    u8 unknown2;
    u8 resolution_pwr; // 1 << [resolution_pwr] == height/width
    u32 unknown3; // Often 0
    u32 ID; // Same in all variations of the same model. Use to identify specific ALRs in memory.
    u32 pad2;
}resource_entry;

typedef struct {
    u32 flags;
    u32 unknown3;
    u32 unknown;
    u32 pad;
    u32 unknown2;
    u32 data_ptr; // This is speculation
    u32 pad2;
}resource_entry_0x16;


// The header of an 0x10 ALR chunk, which stores information about textures in
// the file
typedef struct {
    u32 surface_count;
    u32 texture_count;
    unsigned char alr_name[0x10];
}texture_metadata_header;

typedef struct {
    unsigned char name[0x10];
    u32 unk1;
    u32 unk2;
    u32 unk3;
    u32 unk4;
}tex_name;

// A chunk with metadata about textures stored in the resource section.
typedef struct {
    u32 index;
    unsigned char filename[32];
    u32 padding[2]; // Can't be a u64 because of struct padding
    float unknown[2]; // This is often 1.0f
    u32 width;
    u32 height;
}tex_info;

typedef struct {
    u16 width;
    u16 height;
    u32 flags; // Unknown
    u32 mipmap_count;
    u32 unknown; // Often 4 or 8, sometimes counts up from 13?
    u32 pad;
}surface_info;

// This whole structure appears to hold animation data, or MAYBE mesh data.
typedef struct {
    u32 id; // 0x5
    u32 size;
    float total_time; // This often matches the number of frames(?)
    u16 unknown_settings1;
    u16 array_width_1; // # of bytes in each element of the second array
    u32 translation_key_count; // Derived from 0x000DDFF3 in pdpxb20031024saito_d.xbe (offset 0xCDFF3 in the file)
    u32 rotation_key_count;    // Derived from 0x000DE04E in pdpxb20031024saito_d.xbe (offset 0xCE04E in the file)
    u32 scale_key_count; // Hasn't been tested yet
    u16 unknown_settings2;
    u16 translation_key_size;
}anim_header;

// I don't know why they would use floats for indices... but it seems like
// that's what they did.
typedef struct {
    float frame;
    float x;
}keyframe_1;

typedef struct {
    float frame;
    float x;
    float y;
}keyframe_2;

// The ordering of X, Y, and Z here may be wrong
typedef struct {
    float frame;
    float x;
    float y;
    float z;
}keyframe_3;

typedef struct {
    u16 frame;
    u16 unk1;
    u16 unk2;
    u16 unk3;
}anim_rotation_keys;

typedef struct {
    u32 id;
    u32 chunk_size;
    u16 sub_chunk_count; // Each sub-chunk is 0x4C large
    u16 unknown;
}chunk_0x1_header;

typedef struct {
    u32 id;
    u32 size;
}chunk_generic;

