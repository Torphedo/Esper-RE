// ALR file format for use in ImHex hex editor.
// By Torphedo, Nov. 2 2025.
#include <std/io.pat>
#include <std/string.pat>
#include <std/sys.pat>

struct vec3f {
    float x, y, z;
};

struct String {
    char txt[while($[$] != 0)]; // Null terminated string
    char nullterm [[color("000000")]]; // The null terminator
} [[format_read("stringtostring")]];

fn stringtostring(String s) {
    return s.txt;
};

struct ssb_text {
    u32 text1;
    u32 text2;
} [[format_read("formatSSBText")]];

fn decode_ssb_text(u32 encoded_val) {
    if (encoded_val == 0xFFFFFFFF) {
        return "[empty]";
    }

    const str char_lookup = "\x000123456789abcdefghijklmnopqrstuvwxyz_";
    const u32 ENCODING_BASE = 40;
    const u32 ENCODED_CHAR_COUNT = 6;

    str output = "000000";
    for (s32 i = 0, i < ENCODED_CHAR_COUNT, i+= 1) {
        // Each character is added to the final value, which is then multiplied
        // by the encoding base. To decode, we find the remainder at each power
        // of the base, and use that as an index into a lookup table.
        // In the end this stores 6 characters in 4 bytes.
        const u8 remainder = encoded_val % ENCODING_BASE;
        encoded_val /= ENCODING_BASE;

        // Out-of-bounds values are clamped to the index of the null terminator
        u8 idx = sizeof(char_lookup) - 1;
        if (remainder < idx) {
            idx = remainder;
        }

        // Characters are decoded in reverse order, so we write them back-to-front
        output[ENCODED_CHAR_COUNT - i - 1] = char_lookup[idx];
    }

    return std::format("{}", output);
};

fn formatSSBText(ssb_text txt) {
    str text1 = decode_ssb_text(txt.text1);
    str text2 = decode_ssb_text(txt.text2);

    return std::format("{}{}", text1, text2);
};

struct chunk_header {
    u32 id;
    u32 size;
};

// ID 0x11
struct chunk_layout {
    // In development builds (and maybe modern ones), the game throws an error
    // about the ALR being too old to load unless this is 0x52.
    u16 version; // This is usually (maybe always?) 7
    u16 unk2;
    u32 resbuf_offset;     // Offset of resource buffer at end of file
    u32 offset_array_size; // Number of offsets in the array
    u32 resbuf_size;       // Total size of resource buffer at end of the file
    padding[8];
    u32 offsets[offset_array_size];
};

// Just allows our pointer rebase function to know the resource buffer offset
chunk_header layout_header @ 0 [[hidden]];
chunk_layout layout @ $ [[hidden]];

// Rebase a pointer to be within the resource buffer
fn resbuf_ptr(u32 ptr) {
    return ptr + layout.resbuf_offset;
};

struct material_entry {
    u8 unk1[4];
    u32 unk2;
    u32 unk3; // Usually 0?
    u16 unk4;
    u8 vertbuf_format;
    u8 vert_size;
    u8 entry_idx;
    u8 unk6;
    u16 unk7;
    u16 unk8;
    u16 unk9;
    u16 texture_idx;
    // With specific texture formats (just 0x1F so far), this becomes the index
    // of the lightmap texture, and the next value is the index of the normal map.
    u16 normal_idx;
    u16 normal_backup_idx;
    padding[18];
    u8 unk10[4];
    u8 unk11[4];
    padding[4];
    u32 unk12;
    u16 unk13[2];
    padding[8];
};

// ID 0x1
struct chunk_material {
    u16 num_entries;
    u16 unknown;
    material_entry materials[num_entries];
};

enum alr_primitive_type : u16 {
    // Every 3 vertices are a new triangle (GL_TRIANGLES)
    IDX_TYPE_NORMAL = 5,
    // Each vertex combines with the last 2 to form a triangle strip.
    // Strips are separated by repeating the same vertex to create an invisible
    // triangle. (GL_TRIANGLE_STRIP)
    IDX_TYPE_STRIP = 6,
};

// ID 0x2
struct chunk_index {
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
    alr_primitive_type primitive_type; // alr_primitive_type enum
    // Seems to be the first index of the first triangle. Maybe used to help order index buffers in optimal order
    u32 first_idx;
    // The number of triangles formed by the indices
    u32 num_tris;
    // The number of indices stored in the buffer (might not match the available space)
    u32 num_indices;
    padding[20];
    u16 indices[num_indices];
};

struct joint_t {
    vec3f position;
    // X/Y/Z Euler rotation
    vec3f rotation;
    vec3f scale;
    u16 unk1;
    u16 unk2;
    u16 unk3;
    s16 parent_idx;
    u32 name;
    padding[0x10];
} [[format_read("format_joint_t")]];

fn format_joint_t(joint_t joint) {
    return std::format("{}", decode_ssb_text(joint.name));
};

// ID 0x3
struct chunk_joints {
    u16 num_joints;
    u16 unknown; // Usually 1
    padding[4];
    joint_t joints[num_joints];
};

struct animation_key<auto size> {
    match (size) {
        (3):  {u8 frame; u16 x;}
        (5):  {u8 frame; u16 x; u16 y;}
        (7):  {u8 frame; u16 x; u16 y; u16 z;}
        (8):  {float frame; float x;}
        (12): {float frame; float x; float y;}
        (16): {float frame; float x; float y; float z;}
    }
};

// ID 0x5
struct chunk_animation {
    float length;
    u16 joint_idx;
    u16 rotation_key_size;
    u32 translation_key_count;
    u32 rotation_key_count;
    u32 scale_key_count;
    u8 unk1;
    u8 unk2;
    u16 translation_key_size;

    animation_key<translation_key_size> translation_keys[translation_key_count];
    animation_key<rotation_key_size> rotation_keys[rotation_key_count];
    // animation_key<translation_key_size> scale_keys[scale_key_count];
};

enum pixel_fmt_t : u8 {
    // These are all the same(?)
    FORMAT_RGBA8 =   0b00000110,
    FORMAT_RGBA8_2 = 0b10010010,
    FORMAT_RGBA8_3 = 0b10000110,
    FORMAT_DXT1 = 0b00001100,
    FORMAT_DXT3 = 0b00001110,
    FORMAT_DXT5 = 0b00001111,
    FORMAT_A8 = 0b10000000,
    FORMAT_A8_2 = 0b10011111,
    FORMAT_A8_3 = 0b10010011,
    FORMAT_RG8 = 0b10000100,
    FORMAT_MONO_16 = 0b10000010,
    FORMAT_MONO_16_2 = 0b10000101,
};

// Bad enum name. I don't know what this value means except for these 2
// constants. Maybe a bitfield??
enum texture_type : u8 {
    TEXTURE_REGULAR = 0x29,
    TEXTURE_CUBEMAP = 0x2D
};

// After the header, there are [array_size] instances of this structure:
struct texture_entry {
    u32 flags;          // Unknown, always 01 00 04 00 so far
    u8* data_ptr : u32 [[pointer_base("resbuf_ptr")]]; // Offset to data in resource section (relative to chunk_layout.resbuf_offset)
    padding[4];         // Always 0 (so far)
    texture_type type;  // Usually 0x29
    pixel_fmt_t pixel_format;
    u8 unknown2;
    // I think this is actually the mip count, but often the textures have the
    // maximum possible mip count, so 1 << [mip count] == height/width.
    u8 resolution_pwr;
    u32 unknown3; // Often 0
    u32 text1;
    u32 text2;
} [[format_read("format_texture_entry")]];

fn format_texture_entry(texture_entry e) {
    return std::format("{}{}", decode_ssb_text(e.text1), decode_ssb_text(e.text2));
};

// ID 0x15
struct chunk_texture {
    u32 num_textures;
    texture_entry textures[num_textures];
};

struct vertbuf_entry {
    u8 format; // Determines structure of the vertex data
    u8 vertex_size; // These are always the same (so far?)
    u8 vertex_size2;
    u8 unknown1;
    u32 unknown2;
    u32 vertex_count;
    padding[4];
    u32 unknown3;
    u8* data_ptr: u32 [[pointer_base("resbuf_ptr")]];
    padding[4];
};

// ID 0x16
struct chunk_mesh {
    u32 num_meshes;
    vertbuf_entry meshes[num_meshes];
};

struct chunk_generic {
    chunk_header header;
    match (header.id) {
        (0x11): chunk_layout layout;
        (0x15): chunk_texture textures;
        (0x16): chunk_mesh meshes;
        (0x1): chunk_material materials;
        (0x2): chunk_index idx_buffer;
        (0x3): chunk_joints joints;
        (0x5): chunk_animation animations;
        (0x00 | 0xD): padding[0];
        // Do nothing
        (_): padding[header.size] [[color("FF0000")]];
    }

    if (header.size == 0) {
        break;
    }

    $ = addressof(header) + header.size;
};

chunk_generic chunks[while (!std::mem::eof())] @ 0x0;

const u32 resbuf_size = chunks[0].layout.resbuf_size;
u8 resbuf[resbuf_size] @ chunks[0].layout.resbuf_offset [[color("FF0000")]];