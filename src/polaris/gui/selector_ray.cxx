#include "selector_ray.hxx"
#include <common/vfile.h>

vec3s screen_to_world(vec2s mouse_pos, vec4s viewport, mat4s view_proj_xform, float near_plane) {
    const vec3s window_pos = {mouse_pos.x, mouse_pos.y, near_plane};
    return glms_unproject(window_pos, view_proj_xform, viewport);
}

ray_t screen_to_ray(vec2s mouse_pos, const camera& cam, vec4s viewport) {
    mat4s proj_view = GLMS_MAT4_IDENTITY_INIT;
    cam.proj_view((vec4*)proj_view.raw);

    const vec3s near_screen = {mouse_pos.x, mouse_pos.y, cam.near_clip_plane};
    const vec3s far_screen = {mouse_pos.x, mouse_pos.y, cam.far_clip_plane};
    const vec3s near_world = glms_unproject(near_screen, proj_view, viewport);
    const vec3s far_world = glms_unproject(far_screen, proj_view, viewport);

    ray_t out = {
        .origin = near_world,
        .dir = glms_normalize(glms_vec3_sub(far_world, near_world)),
    };

    return out;
}

bool raycast_aabb(ray_t ray, vec3s min, vec3s max) {
    vec3s box[2] = {min, max};
    if (glms_aabb_point(box, ray.origin)) {
        return true; // Already inside the box
    }

    const vec3s center = glms_aabb_center(box);
    const vec3s dir_to_box = glms_vec3_sub(center, ray.origin);

    vec4s sphere = {0};
    glms_aabb_sphere(box, sphere);
    const float radius = sphere.w;
    const float raymarch_distance = radius + glms_vec3_norm(dir_to_box);

    float mag = 0.001f;
    while (!glms_aabb_point(box, glms_vec3_scale(ray.dir, mag)) && mag < raymarch_distance) {
        mag += 0.1f;
    }

    return mag < raymarch_distance;
}

vec3s vec3_transform(vec3s input, mat4s xform) {
    const vec4s v = glms_mat4_mulv(xform, glms_vec4(input, 1.0f));
    return glms_vec3(v);
}

bool raycast(ray_t ray, const void* vertbuf, u32 vertex_size, mat4s transform, const idxbuf_header* idxbuf) {
    const vec3s box_min = *(vec3s*)&idxbuf->aabb_min;
    const vec3s box_max = *(vec3s*)&idxbuf->aabb_max;
    if (!raycast_aabb(ray, box_min, box_max)) {
        // TODO: Make AABB test work properly
        // return false;
    }

    const bool strip = idxbuf->primitive_type == IDX_TYPE_STRIP;
    vfile vf = vfile_open((void*)vertbuf, vertex_size * idxbuf->num_tris);

    // Copied from alr_dump.cxx!dump_idx_buf()
    for (s32 i = 2; i < idxbuf->num_indices; i++) {
        const u16* indices = &idxbuf->indices[i - 2];
        if (indices[0] == indices[1] || indices[0] == indices[2] || indices[1] == indices[2]) {
            // Triangle strips will repeat 1 index to create a triangle with an
            // area of 0, which is used to end a strip and start another.
            // We skip these since they're not part of the geometry.
            continue;
        }

        vec3s points[3] = {};
        for (u32 j = 0; j < 3; j++) {
            vf.pos = indices[j] * vertex_size;
            points[j] = VFILE_READ(vec3s, &vf);
            points[j] = vec3_transform(points[j], transform);
        }

        float distance = 0.0f;
        bool hit = glms_ray_triangle(ray.origin, ray.dir, points[0], points[1], points[2], &distance);
        if (hit) {
            return true;
        }

        if (!strip) {
            i += 2;
        }
    }

    return false;
}
