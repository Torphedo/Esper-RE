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
    u8 unknown[8];
}cad_path;
static_assert(sizeof(cad_path) == 0x2C, "CAD path struct is the wrong size");

enum {
    CAD_VERTEX_ARRAY_SIZE = 1000,
    CAD_QUAD_ARRAY_SIZE = 0x80,
    CAD_PATH_ARRAY_SIZE = 0x200,
};

typedef struct {
    u32 vertex_count;
    vec3f vertices[CAD_VERTEX_ARRAY_SIZE];
    u32 quad_count;
    cad_quad quads[CAD_QUAD_ARRAY_SIZE];
    u32 path_count;
    cad_path paths[CAD_PATH_ARRAY_SIZE];
}cad_file;

#ifdef __cplusplus
}
#endif
