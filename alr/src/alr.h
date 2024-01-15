#pragma once
#include "arguments.h"

#include "int_shorthands.h"

// All ALR files begin with this structure.
// Followed by a u32 array whose size is listed in the header. The u32s are
// offsets to chunks of data throughout the file, and aren't always in order.
// It's unclear what these offsets are used for or if there's any pattern to
// the grouping. They are part of the header chunk, so the size includes this
// array.
typedef struct {
    u32 id;                // 0x11
    u32 chunk_size;        // Size of this chunk (includes ID & size)
    u32 flags;             // Unknown
    u32 resource_offset;   // Offset of resource buffer at end of file
    u32 offset_array_size; // Number of offsets in the array
    u32 resource_size;     // Total size of resource buffer at end of the file
    u64 pad;
}chunk_layout;


// chunk_layout except with the id and size removed, so it can be used on a
// chunk buffer.
typedef struct {
    u32 flags;             // Unknown
    u32 resource_offset;   // Offset of resource buffer at end of file
    u32 offset_array_size; // Number of offsets in the array
    u32 resource_size;     // Total size of resource buffer at end of the file
}alr_header;

// This structure follows the offset array. It has offsets into the resource
// buffer which is always at the end of the file. It also has some metadata
// about textures in the file, which aren't fully understood.
typedef struct {
    u32 id;         // 0x15
    u32 chunk_size; // Size of this entire chunk
    u32 array_size;
}resource_layout_header;

// Next, there are [array_size] instances of this structure:

typedef enum {
    FORMAT_MONO_16 = 0b10000010,
    FORMAT_RGBA8 = 0b00000110,
    FORMAT_DXT5 = 0b00001111,
}alr_pixel_format;

// Bad enum name. I don't know what this value means except for these 2
// constants. Maybe a bitfield??
typedef enum {
    TEXTURE_REGULAR = 0x29,
    TEXTURE_CUBEMAP = 0x2D
}alr_texture_style;

typedef struct {
    u32 flags;    // Unknown, always 01 00 04 00 so far
    u32 data_ptr; // Offset to data in resource section (relative to chunk_layout.resource_offset)
    u32 pad;      // Always 0 (so far)
    u8 unknown;   // Usually 0x29
    u8 pixel_format;
    u8 unknown2;
    // I think this is actually the mip count, but often the textures have the
    // maximum possible mip count, so 1 << [mip count] == height/width.
    u8 resolution_pwr;
    u32 unknown3; // Often 0
    // Same in all variations of the same model. Can be used to identify
    // specific ALRs in memory.
    u32 ID;
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
    u32 translation_key_count; // Name from 0x000DDFF3 in pdpxb20031024saito_d.xbe (offset 0xCDFF3 in the file)
    u32 rotation_key_count;    // Name from 0x000DE04E in pdpxb20031024saito_d.xbe (offset 0xCE04E in the file)
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

// Interfaces for handling the ALR data.
// Chunk size & ID, then the chunk data, then the offset array index this chunk
// is located in. (idx is used for split function)
typedef void (*chunk_handler)(chunk_generic chunk, u8* chunk_buf, u32 idx);
typedef void (*buffer_handler)(u8* buf, u32 size, u32 idx);

typedef enum {
    ALR_MAX_CHUNK_ID = 0x16
}chunkmax;

// ALR processing interface. The chunk loop will call the appropriate function
// for each chunk and for each texture buffer. Data from the resource layout
// header (0x15 chunk) is needed for texture metadata, so that chunk handler
// should store some state for later.
typedef struct {
    chunk_handler chunk_handlers[ALR_MAX_CHUNK_ID + 1];
    buffer_handler tex_handler;
}alr_interface;


// Sends chunk data to callbacks via the provided interface, which can modify
// the input data. The ALR is written to the output file with any modifications
// made by callbacks. This allows callbacks to easily edit individual buffers
// of an ALR without handling the output themselves.
bool alr_edit(char* alr_filename, char* out_filename, flags options, alr_interface handlers);

// Sends chunk data to callbacks via the provided interface.
bool alr_parse(char* alr_filename, flags options, alr_interface handlers);

