#pragma once
// Structures for .cad files, for the AI's pathfinding data

#ifdef __cplusplus
extern "C" {
#endif
#include <common/int.h>
#include <assert.h>
#include "alr.h" // For vec3f structure

typedef struct {
    u8 flags;
    u8 unknown1[3];
    s32 unknown2[8];
    // We think this is the index into the vertex buffer. Why are they 32-bit
    // when the vertex buffer has a fixed size of 1000?
    s32 vertices[4];
    s16 unknown3[4];
    s8 unknown4[24];
}cad_quad;
static_assert(sizeof(cad_quad) == 0x54, "CAD quad struct is the wrong size");

typedef struct {
    vec3f start_point;
    vec3f end_point;
    u32 connected_areas[2]; // These are indices. (To what? quads?)
    u32 flags; // Unknown meaning
    float unknown1;
    u8 unknown2[4];
}cad_path;
static_assert(sizeof(cad_path) == 0x2C, "CAD path struct is the wrong size");

typedef struct {
    vec3f unknown_pos;
    u32 unknown1;
    u32 unknown2;
    float unknown3;
}cad_unknown_struct1;
static_assert(sizeof(cad_unknown_struct1) == 0x18);

typedef struct {
    u32 unknown;
    vec3f unknown_pos;
}cad_unknown_struct2;
static_assert(sizeof(cad_unknown_struct2) == 0x10);

typedef struct {
    s16 unknown1[2];
    s32 unknown2[127];
}cad_unknown_struct3;
static_assert(sizeof(cad_unknown_struct3) == 0x200);

enum {
    CAD_VERTEX_ARRAY_SIZE = 1000,
    CAD_QUAD_ARRAY_SIZE = 0x80,
    CAD_PATH_ARRAY_SIZE = 0x200,
    CAD_UNKNOWN1_ARRAY_SIZE = 0x1E,
    CAD_UNKNOWN2_ARRAY_SIZE = 5,
    CAD_UNKNOWN3_ARRAY_SIZE = 0x280,
};

typedef struct {
    u32 vertex_count;
    vec3f vertices[CAD_VERTEX_ARRAY_SIZE];
    u32 quad_count;
    cad_quad quads[CAD_QUAD_ARRAY_SIZE];
    u32 path_count;
    cad_path paths[CAD_PATH_ARRAY_SIZE];
    u32 unknown_count1;
    cad_unknown_struct1 unknown1[CAD_UNKNOWN1_ARRAY_SIZE];
    cad_unknown_struct2 unknown2[CAD_UNKNOWN2_ARRAY_SIZE];
    // This is only the 2nd unknown count value, but "...count2" implies count
    // for "unknown2". If this comment doesn't make sense, it's probably
    // outdated and you can ignore it - torph
    u32 unknown_count3;
    u8 padding[400];
    cad_unknown_struct2 unknown3[CAD_UNKNOWN3_ARRAY_SIZE];

}cad_file;
static_assert(offsetof(cad_file, quad_count) == 0x2EE4);
static_assert(offsetof(cad_file, path_count) == 0x58E8);
static_assert(offsetof(cad_file, unknown_count1) == 0xB0EC);
static_assert(offsetof(cad_file, unknown1) == 0xB0F0);
static_assert(offsetof(cad_file, unknown2) == 0xB3C0);
static_assert(offsetof(cad_file, unknown_count3) == 0xB410);
static_assert(offsetof(cad_file, unknown3) == 0xB5A4);

#ifdef __cplusplus
}
#endif
