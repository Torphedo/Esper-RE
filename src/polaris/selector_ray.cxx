#include "selector_ray.hxx"

vec3s screen_to_world(vec2s mouse_pos, vec4s viewport, mat4s view_proj_xform) {
    const vec3s window_pos = {mouse_pos.x, mouse_pos.y, 0.0f};
    return glms_unproject(window_pos, view_proj_xform, viewport);
}

ray_t screen_to_ray(vec2s mouse_pos, const camera& cam, vec4s viewport) {
    mat4 proj_view = GLM_MAT4_IDENTITY_INIT;
    cam.proj_view(proj_view);
    const vec3s world_pos = screen_to_world(mouse_pos, viewport, *(mat4s*)proj_view);

    ray_t out = {
        .origin = cam.target,
        .dir = glms_normalize(glms_vec3_sub(world_pos, cam.target)),
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

bool raycast(ray_t ray, const u8* vertbuf, u32 vertex_size, mat4s transform, const idxbuf_header* idxbuf) {
    const vec3s box_min = *(vec3s*)&idxbuf->aabb_min;
    const vec3s box_max = *(vec3s*)&idxbuf->aabb_max;
    if (!raycast_aabb(ray, box_min, box_max)) {
        // TODO: Make AABB test work properly
        // return false;
    }

    const u16* indices = (u16*)&idxbuf[1]; // Indices begin when header ends
    const bool strip = idxbuf->primitive_type == IDX_TYPE_STRIP;
    // Copied from alr_dump.cxx!dump_idx_buf()
    for (s32 i = 2; i < idxbuf->num_indices; i++) {
        const u16 idx1 = indices[i - 2];
        const u16 idx2 = indices[i - 1];
        const u16 idx3 = indices[i];
        if (idx1 == idx2 || idx1 == idx3 || idx2 == idx3) {
            // Triangle strips will repeat 1 index to create a triangle with an
            // area of 0, which is used to end a strip and start another.
            // We skip these since they're not part of the geometry.
            continue;
        }

        const u8* vert1 = vertbuf + (idx1 * vertex_size);
        const u8* vert2 = vertbuf + (idx2 * vertex_size);
        const u8* vert3 = vertbuf + (idx3 * vertex_size);

        vec3s point1 = *(vec3s*)vert1;
        vec3s point2 = *(vec3s*)vert2;
        vec3s point3 = *(vec3s*)vert3;
        point1 = vec3_transform(point1, transform);
        point2 = vec3_transform(point2, transform);
        point3 = vec3_transform(point3, transform);

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
