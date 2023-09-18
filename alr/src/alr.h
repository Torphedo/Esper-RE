#pragma once

#include <bits/stdint-uintn.h>
#include <stdint.h>

// All ALR files begin with this structure.
typedef struct {
    uint32_t id;                // 0x11
    uint32_t chunk_size;        // Size of this entire chunk
    uint32_t flags;             // Unknown
    uint32_t resource_offset;   // Offset of resource section (at end of file)
    uint32_t offset_array_size; // Number of offsets in the array
    uint32_t last_resource_end; // Offset from resource_offset where the last resource ends. This should point to the end of the file
    uint64_t pad;
}chunk_layout;

// Next, there is an array of uint32_t absolute offsets to various chunks of
// data throughout the file. It's unclear why these are split into these large
// chunks or if there's any pattern to the grouping. The size listed in the
// chunk layout struct includes this array, which has a size of
// (chunk_layout.offset_array_size * 0x4).

// After the chunk layout structure, there is a (poorly understood) structure
// containing several absolute offsets into the resource section, which is
// known to store raw textures.

typedef struct {
    uint32_t id;         // 0x15
    uint32_t chunk_size; // Size of this entire chunk
    uint32_t array_size;
}resource_layout_header;

// Next, there are [array_size] instances of this structure:

typedef struct {
    uint32_t flags;    // Unknown, always 01 00 04 00 so far
    uint32_t data_ptr; // Offset to some data in the resource section (relative to chunk_layout.resource_offset)
    uint32_t pad;      // Always 0 (so far)
    uint32_t unknown;
    uint32_t unknown2; // Often 0
    uint32_t ID; // Same in all variations of the same model. Use to identify specific ALRs in memory.
    uint32_t unknown3;
}resource_entry;

typedef struct {
    uint32_t flags;
    uint32_t unknown3;
    uint32_t unknown;
    uint32_t pad;
    uint32_t unknown2;
    uint32_t data_ptr; // This is speculation
    uint32_t pad2;
}resource_entry_0x16;


// The header of an 0x10 ALR chunk, which stores information about textures in
// the file
typedef struct {
    uint32_t surface_count;
    uint32_t texture_count;
    unsigned char alr_name[0x10];
}texture_metadata_header;

typedef struct {
    char name[0x10];
    uint32_t unk1;
    uint32_t unk2;
    uint32_t unk3;
    uint32_t unk4;
}tex_name;

// A chunk with metadata about textures stored in the resource section.
typedef struct {
    uint32_t index;
    char filename[32];
    uint32_t padding[2]; // Can't be a u64 because of struct padding
    float unknown[2]; // This is often 1.0f
    uint32_t width;
    uint32_t height;
}tex_info;

typedef struct {
    uint16_t width;
    uint16_t height;
    uint32_t flags; // Unknown
    uint32_t mipmap_count;
    uint32_t unknown; // Often 4 or 8, sometimes counts up from 13?
    uint32_t pad;
}surface_info;

// This whole structure appears to hold animation data, or MAYBE mesh data.
typedef struct {
    uint32_t id; // 0x5
    uint32_t size;
    float total_time; // This often matches the number of frames(?)
    uint16_t unknown_settings1;
    uint16_t array_width_1; // # of bytes in each element of the second array
    uint32_t translation_key_count; // Derived from 0x000DDFF3 in pdpxb20031024saito_d.xbe (offset 0xCDFF3 in the file)
    uint32_t rotation_key_count;    // Derived from 0x000DE04E in pdpxb20031024saito_d.xbe (offset 0xCE04E in the file)
    uint32_t scale_key_count; // Hasn't been tested yet
    uint16_t unknown_settings2;
    uint16_t translation_key_size;
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
    uint16_t frame;
    uint16_t unk1;
    uint16_t unk2;
    uint16_t unk3;
}anim_rotation_keys;

typedef struct {
    uint32_t id;
    uint32_t chunk_size;
    uint16_t sub_chunk_count; // Each sub-chunk is 0x4C large
    uint16_t unknown;
}chunk_0x1_header;

typedef struct {
    uint32_t id;
    uint32_t size;
}chunk_generic;

