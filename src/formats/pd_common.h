#pragma once
/// @file pd_common.h
/// This file contains structures used across several formats.

typedef struct {
    float x;
    float y;
    float z;
}vec3f;

// Bounding box + sphere
typedef struct {
    vec3f center;
    float sphere_radius;
    vec3f aabb_min;
    vec3f aabb_max;
}pd3d_bounds;
static_assert(sizeof(pd3d_bounds) == 40, "Wrong 3D bounds size!");
