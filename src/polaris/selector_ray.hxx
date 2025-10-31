#pragma once
#include <cglm/struct.h>

#include <formats/alr.h>
#include "camera.hxx"

struct ray_t {
    vec3s origin;
    vec3s dir;
};

vec3s screen_to_world(vec2s mouse_pos, vec4s viewport, mat4s view_proj_xform);

ray_t screen_to_ray(vec2s mouse_pos, const camera& cam, vec4s viewport);


bool raycast(ray_t ray, vertbuf_entry vert_entry, idxbuf_header idxbuf, u8* resbuf);
