#pragma once

#include <stdint.h>

// All ALR files begin with this structure.
typedef struct {
    uint32_t id;                // 0x11
    uint32_t block_size;        // Size of this entire structure, including the offset array you'll have to read separately
    uint32_t flags;             // Unknown, may potentially indicate types of data that will be in the file
    uint32_t resource_offset;   // Offset of resource section at the end of the file
    uint32_t offset_array_size; // Number of offsets in the array
    uint32_t last_resource_end; // Relative offset from resource_offset where the last resource ends. This should point to the end of the file when added to resource_offset.
    uint64_t pad;
}block_layout;

// Next, there is an array of uint32_t absolute offsets to various chunks of data throughout the file. It's unknown why
// these are split into these large chunks or if there's any pattern to the grouping. The size listed in the block
// layout struct includes this array, which has a size of (block_layout.offset_array_size * 0x4).

// After the block layout structure, there is a (poorly understood) structure containing several absolute offsets into
// the resource section, which is confirmed to store textures. It most likely stores other types of 3D resources as well.

typedef struct {
    uint32_t id;                // 0x15
    uint32_t block_size;        // Size of this entire structure, including the instances of the next struct
    uint32_t array_size;
}resource_layout_header;

// Next, there are [array_size] instances of this structure:

typedef struct {
    uint32_t flags;    // Unknown, always 01 00 04 00 so far
    uint32_t data_ptr; // An offset to some data in the resource section, relative to block_layout.resource_offset (located at 0xC in the file)
    uint32_t pad;      // Always 0, so far
    uint32_t unknown;
    uint32_t unknown2; // Often 0
    uint32_t ID; // This is the same in all variations of the same model. Easy way to identify specific ALRs / models in memory
    uint32_t unknown3;
}resource_entry;


// The header of an 0x10 ALR block, which stores information about textures in the file
typedef struct {
    uint32_t id; // 0x10000000
    uint32_t size;
    uint32_t DDS_count;
    uint32_t texture_count;
    unsigned char alr_name[0x10];
}texture_metadata_header;

// A block with just enough information to reconstruct a TGA header and attach pixel data to it.
typedef struct {
    uint32_t index;
    char filename[32];
    uint32_t padding[2]; // Making this a uint64_t makes the size incorrect because of struct padding, which could break ALR writing in the future
    float unknown[2]; // This is often 1.0f
    uint32_t width;
    uint32_t height;
}texture_header;

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
    float unknown_float; // This often matches the number of frames
    uint16_t unknown_settings1;
    uint16_t array_width_1; // The number of bytes in each element of the second array
    uint32_t ArraySize1; // The block can have up to 3 arrays of numbered floats
    uint32_t ArraySize2;
    uint32_t ArraySize3; // This might actually be padding, since there's no third settings field.
    uint16_t unknown_settings2;
    uint16_t array_width_2;
}anim_header;

typedef struct {
    float index; // I don't know why they would use floats for indices, but it seems like that's what they did...
    float X;
}anim_array_type1;

typedef struct {
    float index;
    float X;
    float Y;
}anim_array_type2;

typedef struct {
    float index;
    float X;
    float Y;
    float Z;
}anim_array_type3;
