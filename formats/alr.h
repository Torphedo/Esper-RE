#pragma once
#include <common/int.h>
#include <assert.h>

// All ALR files begin with this structure.
// Followed by a u32 array whose size is listed in the header. The u32s are
// offsets to chunks of data throughout the file, and aren't always in order.
// It's unclear what these offsets are used for or if there's any pattern to
// the grouping. They are part of the header chunk, so the size includes this
// array.
typedef struct {
    u32 id;                // 0x11
    s32 chunk_size;        // Size of this chunk (includes ID & size)
    u32 flags;             // Unknown
    u32 texbuf_offset;     // Offset of resource buffer at end of file
    u32 offset_array_size; // Number of offsets in the array
    u32 texbuf_size;       // Total size of resource buffer at end of the file
    u64 pad;
}chunk_layout;
static_assert(sizeof(chunk_layout) == 0x20, "Wrong layout chunk header size!");


// This structure follows the offset array. It has offsets into the resource
// buffer which is always at the end of the file. It also has some metadata
// about textures in the file, which aren't fully understood.
typedef struct {
    u32 id;         // 0x15
    u32 chunk_size; // Size of this entire chunk
    u32 array_size;
}resource_layout_header;
static_assert(sizeof(resource_layout_header) == 0xC, "Wrong texture metadata chunk header size!");

// Next, there are [array_size] instances of this structure:

typedef enum {
    FORMAT_RGBA8 = 0b00000110,
    FORMAT_RGBA8_ALT = 0b10010010, // These are both the same(?)
    FORMAT_DXT1 = 0b00001100,
    FORMAT_DXT3 = 0b00001110,
    FORMAT_DXT5 = 0b00001111,
    FORMAT_A8 = 0b10000000,
    FORMAT_MONO_16 = 0b10000010,
}alr_pixel_format;

// Bad enum name. I don't know what this value means except for these 2
// constants. Maybe a bitfield??
typedef enum {
    TEXTURE_REGULAR = 0x29,
    TEXTURE_CUBEMAP = 0x2D
}alr_texture_style;

typedef struct {
    u32 flags;    // Unknown, always 01 00 04 00 so far
    u32 data_ptr; // Offset to data in resource section (relative to chunk_layout.texbuf_offset)
    u32 pad;      // Always 0 (so far)
    u8 unknown;   // Usually 0x29
    u8 pixel_format;
    u8 unknown2;
    // I think this is actually the mip count, but often the textures have the
    // maximum possible mip count, so 1 << [mip count] == height/width.
    u8 resolution_pwr;
    u32 unknown3; // Often 0
    u32 text1;
    u32 text2;
}resource_entry;
static_assert(sizeof(resource_entry) == 0x1C, "Wrong texture metadata size!");

// This is like the 0x15 structure, but for meshes instead of textures
typedef struct {
    u8 unknown_flag;
    u8 vertex_size; // These are always the same (so far?)
    u8 vertex_size2;
    u8 unknown_flag2;
    u32 unknown3;
    u32 vertex_count;
    u32 pad;
    u32 unknown2;
    u32 data_ptr; // This is speculation
    u32 pad2;
}resource_entry_0x16;
static_assert(sizeof(resource_entry_0x16) == 0x1C, "Wrong vertex metadata size!");

// The header of an 0x10 ALR chunk, which stores information about texture
// atlases in the file.
typedef struct {
    u32 atlas_count; // The number of texture atlases
    u32 texture_count; // The total number of textures in all atlases
    unsigned char alr_name[0x10]; // Usually the name of the ALR with no extension
}texture_metadata_header;
static_assert(sizeof(texture_metadata_header) == 0x18, "Wrong atlas chunk header size!");

// The header is followed by [atlas_count] instances of this structure:
typedef struct {
    unsigned char name[0x10];
    u32 unk1;
    u32 unk2;
    u32 unk3;
    u32 unk4;
}atlas_name;
static_assert(sizeof(atlas_name) == 0x20, "Wrong texture atlas name size!");

// The above structure is followed by [atlas_count] instances of this structure:
typedef struct {
    u16 width;
    u16 height;
    u32 flags; // Unknown
    u32 mipmap_count;
    u32 unknown; // Often 4 or 8, sometimes counts up from 13?
    u32 pad;
}atlas_info;
static_assert(sizeof(atlas_info) == 0x14, "Wrong texture atlas metadata size!");

// The above structure is followed by [texture_count] instances of this structure:
typedef struct {
    u32 index; // The atlas index this texture belongs to
    unsigned char filename[32];
    u32 padding[2]; // Can't be a u64 because of struct padding
    float atlas_texcoords[2]; // This is often 1.0f
    u32 width;
    u32 height;
}tex_info;
static_assert(sizeof(tex_info) == 0x3C, "Wrong texture metadata size!");

// Animation data
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
static_assert(sizeof(anim_header) == 0x20, "Wrong animation header size!");

// The floating-point values at first looked like indices, but are actually
// keyframe values (which would be terrible and unprecise as integers).
typedef struct {
    float frame;
    float x;
}keyframe_1;
static_assert(sizeof(keyframe_1) == 0x8, "Wrong 1-component keyframe size!");

typedef struct {
    float frame;
    float x;
    float y;
}keyframe_2;
static_assert(sizeof(keyframe_2) == 0xC, "Wrong 2-component keyframe size!");

// X, Y, and Z may be labelled in the wrong order, depending on which axis the
// game uses for "up" (but this is an arbitrary naming decision).
typedef struct {
    float frame;
    float x;
    float y;
    float z;
}keyframe_3;
static_assert(sizeof(keyframe_3) == 0x10, "Wrong 3-component keyframe size!");

typedef struct {
    u16 frame;
    u16 unk1;
    u16 unk2;
    u16 unk3;
}anim_rotation_keys;
static_assert(sizeof(anim_rotation_keys) == 0x8, "Wrong rotation key size!");

typedef struct {
    u16 joint_count;
    u16 unknown; // Usually 1
    u32 pad;
}chunk_armature;
static_assert(sizeof(chunk_armature) == 0x8, "Wrong armature chunk header size!");

typedef struct {
    float mat[3][3];
    u16 unk1;
    u16 unk2;
    u16 unk3;
    s16 parent_idx;
    u32 name;
    u8 pad[0x10];
}joint_t;
static_assert(sizeof(joint_t) == 0x40, "Wrong joint size!");

typedef struct {
    u32 id;
    u32 chunk_size;
    u16 sub_chunk_count; // Each sub-chunk is 0x4C large
    u16 unknown;
}chunk_0x1_header;
static_assert(sizeof(chunk_0x1_header) == 0xC, "Wrong 0x1 chunk header size!");

// For 0x2 chunks
typedef struct {
    float unk_float[10];
    u32 pad[3]; // Always 0, so far
    u32 unk1; // Definitely a u32, unknown purpose
    u16 vertex_buf; // Index of vertex buffer in the 0x16 chunk
    u16 unk2;
    u16 vertex_buf2; // Same as above, so far?
    u16 unk3; // Usually 5?
    // Seems to be the first index of the first triangle. Maybe used to help order index buffers in optimal order
    u32 first_idx;
    u32 num_tris;
    u32 unk4;
    u32 pad2[5];
}idx_buf_header;
static_assert(sizeof(idx_buf_header) == 0x60, "Wrong index buffer header size!");

typedef struct {
    u32 id;
    s32 size;
}chunk_generic;
static_assert(sizeof(chunk_generic) == 0x8, "Wrong generic chunk header size!");
