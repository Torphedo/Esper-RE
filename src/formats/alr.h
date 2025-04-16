#pragma once
#include <common/int.h>
#include <assert.h>
#ifdef __cplusplus
extern "C" {
#endif

// 0x11 chunk
// =====================================================================================================================
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


// 0x15 chunk
// =====================================================================================================================
// This describes the format/dimensions/etc. of textures, and always comes after the 0x11 chunk.
// At the end of the file is a large buffer with vertex and texture data (the resource buffer).
// Together with 0x16 chunks, it maps out the resource buffer.
typedef struct {
    u32 id;         // 0x15
    u32 chunk_size; // Size of this entire chunk
    u32 array_size;
}texture_header;
static_assert(sizeof(texture_header) == 0xC, "Wrong texture metadata chunk header size!");

typedef enum {
    // These are all the same(?)
    // TODO: Why are so many images BRGA?
    FORMAT_RGBA8 =   0b00000110,
    FORMAT_RGBA8_2 = 0b10010010,
    FORMAT_RGBA8_3 = 0b10000110,
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

// After the header, there are [array_size] instances of this structure:
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
}texture_entry;
static_assert(sizeof(texture_entry) == 0x1C, "Wrong texture metadata size!");

// 0x16 chunk
// =====================================================================================================================
// This describes the format, size, etc. of vertex buffers.
// Together with 0x15 chunks, it maps out the resource buffer.
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
}vertbuf_entry;
static_assert(sizeof(vertbuf_entry) == 0x1C, "Wrong vertex metadata size!");

// There are lots of different vertex formats used for different purposes. The
// known vertex sizes (in bytes) are:
// - 0xC (st06.alr)
// - 0x10 (pc00a.alr for low LOD character)
// - 0x14 (st06.alr)
// - 0x18 (st06.alr, pc00a.alr for high LOD character)
// - 0x20 (st06.alr)

// 0x10 chunk
// =====================================================================================================================
// This chunk is for texture atlases and their sub-textures.
typedef struct {
    u32 atlas_count; // The number of texture atlases
    u32 texture_count; // The total number of textures in all atlases
    unsigned char alr_name[0x10]; // Usually the name of the ALR without the ".alr" part
}atlas_header;
static_assert(sizeof(atlas_header) == 0x18, "Wrong atlas chunk header size!");

// After the header are [atlas_count] instances of this structure:
typedef struct {
    char name[0x10];
    u32 unk1;
    u32 unk2;
    u32 unk3;
    u32 unk4;
}atlas_name;
static_assert(sizeof(atlas_name) == 0x20, "Wrong texture atlas name size!");

// After that are [atlas_count] instances of this structure:
// Represents a single texture atlas
typedef struct {
    u16 width;
    u16 height;
    u32 flags; // Unknown
    u32 tex_count;
    u32 unknown; // Often 4 or 8, sometimes counts up from 13?
    u32 pad;
}atlas_entry;
static_assert(sizeof(atlas_entry) == 0x14, "Wrong texture atlas metadata size!");

// After that are [texture_count] instances of this structure:
// Represents a texture in an atlas
typedef struct {
    u32 index; // The atlas index this texture belongs to
    char filename[32];
    u32 padding[2]; // Can't be a u64 because of struct padding
    float atlas_texcoords[2]; // This is often 1.0f
    u32 width;
    u32 height;
}atlas_tex_entry;
static_assert(sizeof(atlas_tex_entry) == 0x3C, "Wrong texture metadata size!");

// 0x5 chunk
// =====================================================================================================================
// This stores keyframes for a single animation.
typedef struct {
    u32 id; // 0x5
    u32 size;
    float length; // How many frames the animation lasts
    u16 unknown_settings1;
    u16 rotation_key_size;
    u32 translation_key_count; // Name from 0x000DDFF3 in pdpxb20031024saito_d.xbe (offset 0xCDFF3 in the file)
    u32 rotation_key_count;    // Name from 0x000DE04E in pdpxb20031024saito_d.xbe (offset 0xCE04E in the file)
    u32 scale_key_count; // Hasn't been tested yet
    u16 unknown_settings2;
    u16 translation_key_size;
}anim_header;
static_assert(sizeof(anim_header) == 0x20, "Wrong animation header size!");

// Animation key with 1 component
typedef struct {
    float frame;
    float x;
}anim_key_1;
static_assert(sizeof(anim_key_1) == 0x8, "Wrong 1-component keyframe size!");

// Animation key with 2 components
typedef struct {
    float frame;
    float x;
    float y;
}anim_key2;
static_assert(sizeof(anim_key2) == 0xC, "Wrong 2-component keyframe size!");

// Animation key with 3 components
// X, Y, and Z may be labelled in the wrong order, depending on which axis the
// game uses as "up" (but this is an arbitrary naming decision).
typedef struct {
    float frame;
    float x;
    float y;
    float z;
}anim_key3;
static_assert(sizeof(anim_key3) == 0x10, "Wrong 3-component keyframe size!");

// Force struct packing off just for this struct, otherwise we can't read the
// data in the ALR file
#pragma pack(push, r1, 1)

// Animation key for rotation.
// This comes from decompiling the 2003 build, but I don't remember seeing this in a real file.
typedef struct {
    u8 frame;
    u16 unk1;
    u16 unk2;
    u16 unk3;
}anim_rotation_key;
#pragma pack(pop, r1)
static_assert(sizeof(anim_rotation_key) == 0x7, "Wrong rotation key size!");

// 0x3 chunk
// =====================================================================================================================
// This stores all the joints in the skeleton/armature and their relationships to each other.
typedef struct {
    u16 joint_count;
    u16 unknown; // Usually 1
    u32 pad;
}chunk_armature;
static_assert(sizeof(chunk_armature) == 0x8, "Wrong armature chunk header size!");

// After the header are [joint_count] instances of this structure, holding information about each joint/bone.
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

// 0x2 chunk
// =====================================================================================================================
// Information about an index buffer.

enum {
    IDX_TYPE_NORMAL = 5,
    IDX_TYPE_STRIP = 6,
};

typedef struct {
    // Centerpoint of the object represented by the index buffer
    float center[3];
    float unk_float;
    // AABB min of the object represented by the index buffer
    float aabb_min[3];
    // AABB max of the object represented by the index buffer
    float aabb_max[3];
    u16 unk4[6];
    u32 unk1; // Definitely a u32, unknown purpose
    u16 texture_idx; // Index of vertex buffer in the 0x16 chunk
    u16 unk2;
    u16 vertex_buf; // Same as above, so far?
    u16 unk3; // Usually IDX_TYPE_NORMAL. When it's IDX_TYPE_STRIP, the indices are for a triangle strip. Maybe a bitfield?
    // Seems to be the first index of the first triangle. Maybe used to help order index buffers in optimal order
    u32 first_idx;
    // The number of triangles formed by the indices
    u32 num_tris;
    // The number of indices stored in the buffer (might not match the available space)
    u32 num_indices;
    u32 pad[5];
}idxbuf_header;
static_assert(sizeof(idxbuf_header) == 0x60, "Wrong index buffer header size!");

// 0x1 chunk
// =====================================================================================================================
// Not researched yet.
typedef struct {
    u32 id;
    u32 chunk_size;
    u16 num_entries; // Each entry is 0x4C bytes
    u16 unknown;
}chunk_0x1_header;
static_assert(sizeof(chunk_0x1_header) == 0xC, "Wrong 0x1 chunk header size!");

typedef struct {
    u8 unk1[4]; // 4
    u32 unk2; // 8
    u32 unk3; // Usually 0? // C
    u16 unk4; // E
    u16 unk5; // 10
    u16 unk6; // 12
    u16 unk7; // 14
    u16 unk8; // 16
    u16 unk9; // 18
    u16 texture_idx; // 1C
    u16 normal_idx;
    u16 reflect_idx;
    u32 pad[4];
    u8 unk10[4];
    u8 unk11[4];
    u32 pad2;
    u32 unk12;
    u16 unk13[2];
    u32 pad3[2];
}chunk_0x1_entry;
static_assert(sizeof(chunk_0x1_entry) == 0x4C, "Wrong 0x1 chunk entry size!");

// The common ID and size that come at the start of any chunk.
typedef struct {
    u32 id;
    s32 size;
}chunk_generic;
static_assert(sizeof(chunk_generic) == 0x8, "Wrong generic chunk header size!");

#ifdef __cplusplus
}
#endif
