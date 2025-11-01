#include "selector_ray.hxx"

vec3s screen_to_world(vec2s mouse_pos, vec4s viewport, mat4s view_proj_xform) {
    const vec3s window_pos = {mouse_pos.x, mouse_pos.y, 0.0f};
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

bool raycast_aabb(ray_t ray, vec3s min, vec3s max) {
    vec3s box[2] = {min, max};
    if (glms_aabb_point(box, ray.origin)) {
        return true; // Already inside the box
    }

    const vec3s center = glms_aabb_center(box);
    const vec3s dir_to_box = glms_vec3_sub(center, ray.origin);
    if (glms_vec3_dot(dir_to_box, ray.dir) <= 0.0f) {
        // We are facing away from the box, no matter how far we go down the ray
        // we'll never hit it
        return false;
    }

    vec4s sphere = {0};
    glms_aabb_sphere(box, sphere);
    const float radius = sphere.w;
    const float raymarch_distance = radius + glms_vec3_norm(dir_to_box);

    float mag = 0.001f;
    ray.dir = glms_vec3_normalize(ray.dir);
    while (!glms_aabb_point(box, glms_vec3_scale(ray.dir, mag)) && mag < raymarch_distance) {
        mag += 0.1f;
    }

    return mag < raymarch_distance;
}

bool raycast(ray_t ray, vertbuf_entry vert_entry, idxbuf_header idxbuf, u8* resbuf) {
    const vec3s box_min = *(vec3s*)&idxbuf.aabb_min;
    const vec3s box_max = *(vec3s*)&idxbuf.aabb_max;
    if (!raycast_aabb(ray, box_min, box_max)) {
        return false;
    }

    const u8* vertices = resbuf + vert_entry.data_ptr;
    const bool strip = idxbuf.primitive_type == IDX_TYPE_STRIP;
    for (u32 i = 0; i < idxbuf.num_indices; i++) {
        const u8* vert1 = vertices + (i * vert_entry.vertex_size);
        const u8* vert2 = vert1 + vert_entry.vertex_size;
        const u8* vert3 = vert2 + vert_entry.vertex_size;

        const vec3s point1 = *(vec3s*)vert1;
        const vec3s point2 = *(vec3s*)vert2;
        const vec3s point3 = *(vec3s*)vert3;

        bool hit = glms_ray_triangle(ray.origin, ray.dir, point1, point2, point3, nullptr);
        if (hit) {
            return true;
        }

        if (!strip) {
            i += 2;
        }
    }

    return false;
}
