#pragma once
#include <cglm/struct.h>

#include <formats/alr.h>
#include "camera.hxx"

struct ray_t {
    vec3s origin;
    vec3s dir;
};

/// Convert screen-space position to world space
/// @param mouse_pos Screen space position
/// @param viewport Bounds of the viewport in screen space
/// @param view_proj_xform View-projection transformation matrix
/// @return World space position
vec3s screen_to_world(vec2s mouse_pos, vec4s viewport, mat4s view_proj_xform);

/// Convert mouse position to a ray pointing out from the camera in world space
/// @param mouse_pos Screen space mouse position
/// @param cam Camera state
/// @param viewport Viewport bounds
/// @return World space ray
ray_t screen_to_ray(vec2s mouse_pos, const camera& cam, vec4s viewport);

bool raycast(ray_t ray, const u8* vertbuf, u32 vertex_size, mat4s transform, const idxbuf_header* idxbuf);
