#include <common/gl/input.h>
#include "camera.h"

// Up axis for our camera
vec3s camera_up = {0.0f, 1.0f, 0.0f};

vec3s camera_target = {0};
vec3s camera_pos = {.z = 10.0f};

const float mouse_sens = 0.005f;
const float scroll_sens = 0.05f;

bool invert_mouse_x = true;
bool invert_mouse_y = false;

float clampf(float x, float min, float max) {
    if (x > max) {
        return max;
    }
    else if (x < min) {
        return min;
    }
    else {
        return x;
    }
}

vec2s get_cursor_delta(vec2s cursor_pos) {
    static vec2s last_cursor = {0};

    // Nullify movement unless click is held
    if (!input.click_left) {
        last_cursor = input.cursor;
    }

    vec2s cursor_delta = {
        .x = (cursor_pos.x - last_cursor.x) * mouse_sens,
        .y = (cursor_pos.y - last_cursor.y) * mouse_sens
    };

    // Save state so we can find the delta next time we're called
    last_cursor = cursor_pos;

    // Invert sign as needed.
    if (invert_mouse_x) {
        cursor_delta.x = -cursor_delta.x;
    }
    if (invert_mouse_y) {
        cursor_delta.y = -cursor_delta.y;
    }

    return cursor_delta;
}

void camera_update(mat4* view, float aspect_ratio) {
    static vec2s last_scroll = {0};

    vec2s cursor_delta = get_cursor_delta(input.cursor);

    // Adjust speed based on zoom and account for aspect ratio
    cursor_delta = glms_vec2_scale(cursor_delta, camera_pos.z);
    cursor_delta.x /= aspect_ratio;

    const vec2s scroll_delta = {
        .x = scroll_sens * (last_scroll.x - input.scroll.x),
        .y = scroll_sens * (last_scroll.y - input.scroll.y),
    };

    // Save state so we can find the delta next time we're called
    last_scroll = input.scroll;

    // Update target pos using delta from user input
    camera_target.x += cursor_delta.x;
    camera_target.y += cursor_delta.y;
    camera_pos.x += cursor_delta.x;
    camera_pos.y += cursor_delta.y;

    // Adjust depth w/ zoom
    camera_pos.z += scroll_delta.y;
    camera_pos.z = clampf(camera_pos.z, 0.1f, 100.0f);
    
    mat4 temp = {0};
    if (view == NULL) {
        view = &temp;
    }
    glm_lookat((float*)&camera_pos, (float*)&camera_target, (float*)&camera_up, *view);
}

void camera_view_matrix(mat4 out) {
    glm_lookat((float*)&camera_pos, (float*)&camera_target, (float*)&camera_up, out);
}

void camera_proj_view(mat4 out) {
    // Pre-multiply the projection & view components of the PVM matrix

    // Projection matrix
    mat4 projection = {0};
    const GLFWvidmode* mode = glfwGetVideoMode(glfwGetPrimaryMonitor());
    float aspect = (float)mode->width / (float)mode->height;
    glm_perspective_rh_no(glm_rad(45), aspect, 0.1f, 1000.0f, projection);

    // Camera matrix
    mat4 view = {0};
    camera_view_matrix(view);
    glm_mat4_mul(projection, view, (vec4*)out);
}
