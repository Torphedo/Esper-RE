#include "selector_ray.hxx"

vec3s screen_to_world(vec2s mouse_pos, vec4s viewport, mat4s view_proj_xform) {
    const vec3s window_pos = {mouse_pos.x, viewport.w - mouse_pos.y, 0.0f};
    return glms_unproject(window_pos, view_proj_xform, viewport);
}

ray_t screen_to_ray(vec2s mouse_pos, const camera& cam, vec4s viewport) {
    mat4s proj_view = GLMS_MAT4_IDENTITY_INIT;
    cam.proj_view((vec4*)&proj_view);
    const vec3s world_pos = screen_to_world(mouse_pos, viewport, proj_view);

    ray_t out = {
        .origin = cam.target,
        .dir = glms_vec3_sub(world_pos, cam.target),
    };

    return out;
}
