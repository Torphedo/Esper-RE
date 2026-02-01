#pragma once
#include "shut_up_msvc.h"
#include <stdbool.h>
#include <assert.h>
#include <common/int.h>
#include "data_types.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef struct {
    float x;
    float y;
    float z;
}vec3f;

// ALRs are structured with groups of chunks, separated by empty 0x0 chunks.

// For each animation, chunks are laid out like this:
// - [one or more 0x5 chunk(s)
// - 0x0 (null terminator)

// For each model, chunks are laid out like this:
// - 0x1
// - 0x3
// - 0x16
// - 0x13 [optional]
// - [one or more 0x2 chunk(s)]
// - 0xD (empty)
// - 0x0 (null terminator)

// The whole ALR is laid out like this:
// - 0x11
// - 0x15
// - [animations, if present]
// - [models, if present]
// - 0x10 and 0x0, if texture atlas is used

enum {
    ALR_ID_MATERIAL = 0x1,
    ALR_ID_INDICES = 0x2,
    ALR_ID_SKELETON = 0x3,
    ALR_ID_ANIMATION = 0x5,
    ALR_ID_CAM_ANIM = 0x7,
    ALR_ID_END_INDICES = 0xD,
    ALR_ID_TEXATLAS = 0x10,
    ALR_ID_HEADER = 0x11,
    ALR_ID_TEXTURE = 0x15,
    ALR_ID_MODEL = 0x16,
};

// Header (0x11) chunk
// =============================================================================
// All ALR files begin with this chunk, followed by an array of file offsets.
// The order of offsets follows the structure above. This means the game can use
// the animation ID (see alr_animations.h) as an index to look up animation data.
// Sometimes the offsets are negative (maybe to indicate it doesn't have a
// specific animation), and are usually (but not always) sorted.
typedef struct {
    u32 id;                // 0x11
    s32 chunk_size;        // Size of this chunk (includes ID & size)
    // In development builds (and maybe modern ones), the game throws an error
    // about the ALR being too old to load unless this is 0x52.
    u16 version; // This is usually (maybe always?) 7
    u16 unk2;
    u32 texbuf_offset;     // Offset of resource buffer at end of file
    u32 offset_array_size; // Number of offsets in the array
    u32 texbuf_size;       // Total size of resource buffer at end of the file
    u64 pad;
    s32 offsets[]; // This takes up 0 bytes in C
}chunk_layout;
static_assert(sizeof(chunk_layout) == 0x20, "Wrong layout chunk header size!");

// Texture (0x15) chunk
// =============================================================================
// This describes the format/dimensions/etc. of textures, and always comes after the 0x11 chunk.
// At the end of the file is a large buffer with vertex and texture data (the resource buffer).
// Together with 0x16 chunks, it maps out the resource buffer.
typedef enum {
    // 8-bit red channel only
    FORMAT_R8 = 0,
    FORMAT_R8_2 = 0x13,

    // 8-bit alpha only
    FORMAT_A8 = 0x1F,

    FORMAT_BGRA_5551 = 0x02, // 5 bits per channel, 1 bit alpha
    FORMAT_BGRA_4444 = 0x04, // 4 bits per channel
    FORMAT_BGR_565 = 0x05,   // 5 bits for blue/red, 6 bits for green

    // 8-bit RGBA
    FORMAT_RGBA8 =   0x06,
    FORMAT_RGBA8_2 = 0x12,

    // 0x28 is a case in the game code's switch statement, but it should never
    // be hit since it doesn't fit in 5 bits
    // FORMAT_RGBA8_3 = 0x28,

    // Block-compressed formats
    FORMAT_DXT1 = 0x0C,
    FORMAT_DXT3 = 0x0E,
    FORMAT_DXT5 = 0x0F,
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
    u32 unused;   // The game combines this with [data_ptr] to store a pointer
    u8 unknown;   // Usually 0x29

    // Values match the alr_pixel_format enum. We can't use it directly, because
    // MSVC will make it "int" (4 bytes) by default even in a bitfield.
    u8 pixel_format: 5;
    u8 unk_pixel_format: 3; // The top bit is sometimes set, unclear meaning.
    u8 unknown2: 4;

    // For power-of-2 textures.
    // 1 << n = height/width
    u8 width_pwr: 4;
    u8 height_pwr;

    // For non-power-of-2 textures.
    // These store the actual height/width values, in some rectangular textures.
    u32 width_direct: 12;
    u32 height_direct: 12;
    u32: 0; // Pad out the rest of the 32 bits
    u32 text1;
    u32 text2;
}texture_entry;
static_assert(sizeof(texture_entry) == 0x1C, "Wrong texture metadata size!");

typedef struct {
    u32 id; // 0x15
    u32 size;
    u32 num_entries;
    texture_entry entries[];
}texture_header;
static_assert(sizeof(texture_header) == 0xC, "Wrong texture metadata chunk header size!");


/// @brief Get the height/width of a texture in pixels
///
/// This function accounts for power-of-2 and non-power-of-2 texture sizes.
/// @param entry The texture to read the dimensions of
/// @param height_out Location to receive texture height
/// @param width_out Location to receive texture width
void alr_texture_get_dimensions(texture_entry entry, u16* height_out, u16* width_out);

/// @brief Overwrite the dimensions of a texture
///
/// This function accounts for power-of-2 and non-power-of-2 texture sizes.
/// @param entry Texture to edit
/// @param height The new height (in pixels)
/// @param width The new width (in pixels)
void alr_texture_set_dimensions(texture_entry* entry, u16 height, u16 width);

// Model (0x16) chunk
// =============================================================================
// This describes the format, size, etc. of vertex buffers.
// Together with 0x15 chunks, it maps out the resource buffer.
typedef struct {
    u8 format; // Determines the structure of the vertex data
    u8 vertex_size; // These are always the same (so far?)
    u8 vertex_size2;
    u8 unknown1;
    u32 unknown2;
    u32 vertex_count;
    u32 unused1;
    u32 unknown3;
    u32 data_ptr;
    u32 unused2; // The game uses this to store a pointer in [data_ptr]
}vertbuf_entry;
static_assert(sizeof(vertbuf_entry) == 0x1C, "Wrong vertex metadata size!");

// Same as texture header, but the array is a different type
typedef struct {
    u32 id; // 0x16
    u32 size;
    u32 num_entries;
    vertbuf_entry entries[];
}vertbuf_header;
static_assert(sizeof(vertbuf_header) == 0xC, "Wrong vertex buffer header size!");


// 0x13 chunk
// =============================================================================
// Not much is known about these. They're found often in ALRs from /Effect, and
// sometimes in stage ALRs from /Map.
typedef struct {
    float unk1[12];
    // The last 8 bytes of this may be padding
    u8 unk2[16];
    // This is probably a 4x4 transform matrix
    float unk3[16];
}chunk_0x13;
static_assert(sizeof(chunk_0x13) == 0x80);

// 0x14 chunk
// =============================================================================
// Not much is known about these, they're found in ALRs from /Effect.
typedef struct {
    u8 pad1[8];
    u32 text1;
    u32 text2;
    u16 unk1;
    u16 unk2;
    u8 pad2[16];
}chunk_0x14;
static_assert(sizeof(chunk_0x14) == 0x24);

// ======= BEGIN CUSTOM STRUCTURES =======
// These aren't part of any ALR file, they just let us describe vertex formats
// using data instead of code.

// All supported vertex attribute slots
typedef enum {
    ATTRIBUTE_POSITION,
    ATTRIBUTE_TEXCOORD,
    ATTRIBUTE_LIGHTMAP_TEXCOORD,
    ATTRIBUTE_NORMAL,
    ATTRIBUTE_WEIGHT,
    ATTRIBUTE_ENUM_MAX,
}attribute_idx;

static const char* attribute_names[] = {
    "Position",
    "Texture Coordinates",
    "Lighting Texture Coordinates",
    "Vertex Normal",
    "Weight",
    "[Invalid]",
};

// Vertex attribute data for glVertexAttribPointer()
typedef struct {
    data_type type;
    u16 offset;
    // A value to divide each component by before using it. Unused if 0
    u16 divisor;
    u8 components; // This can only be between 1 and 4

    // Whether this entry is used (the poor man's std::optional).
    bool exists;
}vertex_attribute;

enum {
    ALR_MAX_FORMAT = 0x26,
};

typedef struct {
    // Format value from vertex entry structure. I was going to make an array
    // with this as the index, but sparse arrays are annoying to declare
    // without the [index] = {}, syntax, which g++ doesn't implement even in
    // extern "C" mode. - torph
    u8 id;
    u8 size;
    vertex_attribute attributes[ATTRIBUTE_ENUM_MAX];
}vertex_format_t;

// Position is the same for all formats so far
#define ALR_STD_POS   {      \
    .type = DATA_TYPE_FLOAT, \
    .offset = 0,             \
    .components = 3,         \
    .exists = true,          \
}                            \

#define ALR_STD_UV_DEF(custom_offset, custom_divisor) { \
    .type = DATA_TYPE_S16,               \
    .offset = custom_offset,             \
    .divisor = custom_divisor,           \
    .components = 2,                     \
    .exists = true,                      \
}                                        \

#define ALR_POS_ONLY .attributes = { ALR_STD_POS }

static const vertex_format_t alr_vert_formats[ALR_MAX_FORMAT] = {
    {   .id = 0x01,
        .size = 0x18,
        .attributes = {
            ALR_STD_POS,
            ALR_STD_UV_DEF(16, 4096),
        },
    },
    {   .id = 0x03,
        .size = 0x20,
        .attributes = {
            ALR_STD_POS,
            ALR_STD_UV_DEF(16, 4096),
        },
    },
    {   .id = 0x05,
        .size = 0x1C,
        .attributes = {
            ALR_STD_POS,
            ALR_STD_UV_DEF(16, INT16_MAX),
        },
    },
    {   .id = 0x07,
        .size = 0x10,
        ALR_POS_ONLY,
    },
    {   .id = 0x08, // Suspected to be collision
        .size = 0x14,
        ALR_POS_ONLY,
    },
    {   .id = 0x09,
        .size = 0x14,
        .attributes = {
            ALR_STD_POS,
            ALR_STD_UV_DEF(12, 4096),
        },
    },
    {   .id = 0x0B,
        .size = 0x10,
        ALR_POS_ONLY,
    },
    {   .id = 0x0D,
        .size = 0x00,
    },
    {   .id = 0x10,
        .size = 0x0C,
        ALR_POS_ONLY,
    },
    {   .id = 0x11,
        .size = 0x18,
        .attributes = {
            ALR_STD_POS,
            ALR_STD_UV_DEF(12, INT16_MAX),
            // 0x10 - 0x14 are 16-bit ints, probably unsigned.

            // In pc00a.alr:
            // The first value is mostly 0, but higher around the joints (knees, elbows, neck, etc.)
            // The second value is more or less constant, but notably zero at
            // the bottom of the shoes and under the coat.

            // 0x14 - 0x18 are 16-bit ints, definitely unsigned.
            // Values range from 90 - 170 on both. This is within the array size
            // of 203 on the joint chunk for this file, but there are only 103
            // bones (the rest are identity transforms).
        },
    },
    {   .id = 0x15,
        .size = 0x18,
        .attributes = {
            ALR_STD_POS,
            ALR_STD_UV_DEF(12, INT16_MAX),
        },
    },
    {   .id = 0x16,
        .size = 0x1C,
        ALR_POS_ONLY,
    },
    {   .id = 0x17,
        .size = 0x20,
        ALR_POS_ONLY,
    },
    {   .id = 0x18,
        .size = 0x10,
        ALR_POS_ONLY,
    },
    {   .id = 0x1A,
        .size = 0x28,
        ALR_POS_ONLY,
    },
    {   .id = 0x1D,
        .size = 0x24,
        ALR_POS_ONLY,
    },
    {   .id = 0x1E,
        .size = 0x1C,
        ALR_POS_ONLY,
    },
    {   .id = 0x1F,
        .size = 0x20,
        .attributes = {
            ALR_STD_POS,
            ALR_STD_UV_DEF(16, 4096),
            ALR_STD_UV_DEF(28, 4096),
        },
    },
    {   .id = 0x21,
        .size = 0x20,
        .attributes = {
            ALR_STD_POS,
            ALR_STD_UV_DEF(16, 4096),
        },
    },
    {   .id = 0x25,
        .size = 0x1C,
        .attributes = {
            ALR_STD_POS,
            ALR_STD_UV_DEF(16, 4096),
        },
    },
};

static vertex_format_t format_by_id(u8 id) {
    for (u32 i = 0; i < ARRAY_SIZE(alr_vert_formats); i++) {
        if (alr_vert_formats[i].id == id) {
            return alr_vert_formats[i];
        }
    }

    vertex_format_t out = {
        .size = 12,
        ALR_POS_ONLY,
    };
    return out;
}
// ======= END CUSTOM STRUCTURES =======


// Atlas (0x10) chunk
// =============================================================================
// This chunk is for texture atlases and their sub-textures.
typedef struct {
    u32 id;
    u32 size;
    u32 atlas_count; // The number of texture atlases
    u32 texture_count; // The total number of textures in all atlases
    unsigned char alr_name[0x10]; // Usually the name of the ALR without the ".alr" part
}atlas_header;
static_assert(sizeof(atlas_header) == 0x20, "Wrong atlas chunk header size!");

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
    // Texture coordinate of the bottom right corner of this texture within the
    // atlas. The size in texture coordinates is calculated by dividing the
    // texture size by the size of the whole atlas. Then, that's subtracted from
    // the bottom right corner to find the top left corner.
    float atlas_texcoords[2];
    u32 width;
    u32 height;
}atlas_tex_entry;
static_assert(sizeof(atlas_tex_entry) == 0x3C, "Wrong texture metadata size!");

// Animation (0x5) chunk
// =============================================================================
// This stores keyframes for a single animation.
typedef struct {
    u32 id; // 0x5
    u32 size;
    float length; // How many frames the animation lasts
    u16 joint_idx;
    u16 rotation_key_size;
    u32 translation_key_count; // Name from 0x000DDFF3 in pdpxb20031024saito_d.xbe (offset 0xCDFF3 in the file)
    u32 rotation_key_count;    // Name from 0x000DE04E in pdpxb20031024saito_d.xbe (offset 0xCE04E in the file)
    u32 scale_key_count; // Hasn't been tested yet
    u8 unknown_settings2;
    u8 unknown_settings3;
    u16 translation_key_size;
}anim_header;
static_assert(sizeof(anim_header) == 0x20, "Wrong animation header size!");

// Time per animation frame in seconds
static const double FRAMETIME_24FPS = 1.0 / 24.0;

// Skeleton (0x3) chunk
// =============================================================================
// This stores all the joints in the skeleton/armature and their relationships to each other.

// Definition of a joint in the skeleton
typedef struct {
    vec3f position;
    // X/Y/Z Euler rotation
    vec3f rotation;
    vec3f scale;
    u8 unk1;
    u8 unk2;
    u16 idx;
    u16 pad1;
    s16 parent_idx;
    u32 name;
    u8 pad2[0x10];
}joint_t;
static_assert(sizeof(joint_t) == 0x40, "Wrong joint size!");

typedef struct {
    u32 id;
    u32 size;
    u16 joint_count;
    u16 unknown; // Usually 1
    u32 pad;
    joint_t joints[];
}chunk_armature;
static_assert(sizeof(chunk_armature) == 0x10, "Wrong armature chunk header size!");


// Index buffer (0x2) chunk
// =============================================================================
// Information about an index buffer.

// The primitive type determines how the vertices are translated into triangles.
typedef enum {
    // Every 3 vertices are a new triangle (GL_TRIANGLES)
    IDX_TYPE_NORMAL = 5,
    // Each vertex combines with the last 2 to form a triangle strip.
    // Strips are separated by repeating the same vertex to create an invisible
    // triangle. (GL_TRIANGLE_STRIP)
    IDX_TYPE_STRIP = 6,
}alr_primitive_type;

typedef struct {
    u32 id;
    u32 size;
    // Centerpoint of the object represented by the index buffer
    float center[3];
    float unk_float;
    // AABB min of the object represented by the index buffer
    float aabb_min[3];
    // AABB max of the object represented by the index buffer
    float aabb_max[3];
    u32 unk5;
    u8 unk6;
    u8 unk7;
    u16 unk4[3];
    u32 unk1; // Definitely a u32, unknown purpose
    u16 texture_idx; // 0x1 texture entry to apply to this mesh
    u16 transform_idx;
    // Index of 0x3 entry that has this object's transform. When applying that
    // transform, remember to apply the parent transforms
    u16 vertex_buf; // Index of vertex buffer in the 0x16 chunk
    u16 primitive_type; // alr_primitive_type enum
    // Seems to be the first index of the first triangle. Maybe used to help order index buffers in optimal order
    u32 first_idx;
    // The number of triangles formed by the indices
    u32 num_tris;
    // The number of indices stored in the buffer (might not match the available space)
    u32 num_indices;
    u32 pad[5];

    u16 indices[];
}idxbuf_header;
static_assert(sizeof(idxbuf_header) == 0x68, "Wrong index buffer header size!");

// Material (0x1) chunk
// =============================================================================
typedef struct {
    u8 unk1[4];
    u32 unk2;
    u32 unk3; // Usually 0?
    u16 unk4;
    u16 unk5;
    u8 vertbuf_format;
    u8 vert_size;
    u8 entry_idx;
    u8 unk6;
    u8 unk8;
    u8 shadow_map_flag; // Set to 0x54 on shadow maps
    u16 unk9;
    u16 texture_idx;
    // With specific vertex formats (just 0x1F so far), this becomes the index
    // of the lightmap texture, and the next value is the index of the normal map.
    u16 normal_idx;
    u16 normal_backup_idx;
    u16 pad1;
    u32 pad2[4];
    u8 unk10[4];
    u8 unk11[4];
    u32 pad3;
    u32 unk12;
    u16 unk13[2];
    u32 pad4[2];
}chunk_0x1_entry;
static_assert(sizeof(chunk_0x1_entry) == 0x4C, "Wrong 0x1 chunk entry size!");

typedef struct {
    u32 id;
    u32 size;
    u16 num_entries;
    u16 unknown;
    chunk_0x1_entry entries[];
}chunk_0x1_header;
static_assert(sizeof(chunk_0x1_header) == 0xC, "Wrong 0x1 chunk header size!");

// The common ID and size that come at the start of any chunk.
typedef struct {
    u32 id;
    s32 size;
}chunk_generic;
static_assert(sizeof(chunk_generic) == 0x8, "Wrong generic chunk header size!");

#ifdef __cplusplus
}
#endif
