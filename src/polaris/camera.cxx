#include "camera.hxx"
#include <cglm/struct.h>
#include <glad/glad.h>
#include <common/int.h>

extern "C" {
    #include <common/gl/input.h>
}

// Restrict a number to a certain range
float clampf(float x, float min, float max) {
    if (x > max) {
        return max;
    }
    else if (x < min) {
        return min;
    } else {
        return x;
    }
}

// Get the camera position relative to an orbit center-point based on the
// rotation angles
vec3s orbit_pos_by_angles(camera& cam) {
    // Get combined quaternion of rotation about Y & Z axes
    const versors xrot = glms_quatv(cam.orbit_angles.x, {0, 1, 0});
    const versors yrot = glms_quatv(cam.orbit_angles.y, {0, 0, 1});
    const versors total_rot = glms_quat_mul(xrot, yrot);

    vec3s pos_difference = glms_quat_rotatev(total_rot, {cam.radius,0,0});
    return pos_difference;
}

vec2s camera::get_cursor_delta() {
    const vec2s cursor = vec2s{input.cursor_x, input.cursor_y};

    vec2s cursor_delta = glms_vec2_sub(cursor, last_cursor);
    cursor_delta = glms_vec2_scale(cursor_delta, mouse_sens);

    // Save state so we can find the delta next time we're called
    last_cursor = cursor;

    if (fabsf(input.RS_x) > deadzone || fabsf(input.RS_y) > deadzone) {
        cursor_delta.x = input.RS_x * mouse_sens * 5;
        cursor_delta.y = input.RS_y * mouse_sens * 5;
    }

    // Invert sign as needed.
    if (invert_mouse_x) {
        cursor_delta.x = -cursor_delta.x;
    }
    if (invert_mouse_y) {
        cursor_delta.y = -cursor_delta.y;
    }

    return cursor_delta;
}

void camera::update(double delta_time) noexcept {
    static vec2s last_scroll = {0};

    const vec2s cursor_delta = get_cursor_delta();

    const vec2s scroll_delta = {
        input.scroll_x - last_scroll.x,
        input.scroll_y - last_scroll.y
    };
    // Save state so we can find the delta next time we're called
    last_scroll = vec2s{input.scroll_x, input.scroll_y};


    const vec3s cam_dir = this->facing();
    const float multiplier = (float)delta_time * move_speed;

    // This just sets each axis to zero if it's below the deadzone threshold
    const float LS_x = input.LS_x * (fabsf(input.LS_x) > deadzone);
    const float LS_y = input.LS_y * (fabsf(input.LS_y) > deadzone);

    const float forward  = multiplier * ((input.w - input.s) - LS_y);
    const float side     = multiplier * ((input.a - input.d) - LS_x);
    float vertical = multiplier * ((input.space - input.shift) + (input.RT - input.LT));

    // Exclude vertical view component so it doesn't affect horizontal movement
    vec3s horizontal = glms_normalize({cam_dir.x, 0, cam_dir.z});
    const vec3s cam_side = glms_vec3_rotate(horizontal, glm_rad(90), camera_up);
    // Make forward/back move along camera vector in fly mode
    if (mode == CAMERA_FLY) {
        horizontal = cam_dir;
        vertical = 0; // Ignore the normal vertical movement keys
    }

    vec3s pos_delta = glms_vec3_scale(horizontal, forward); // [Camera dir] * forward movement
    // Add [Camera dir rotated by 90 degrees] * side movement
    pos_delta = glms_vec3_add(pos_delta, glms_vec3_scale(cam_side, side));
    pos_delta.y += vertical;

    // Update angles & zoom from mouse input
    orbit_angles = glms_vec2_add(orbit_angles, cursor_delta);
    radius -= scroll_delta.y;
    radius = MAX(0.05f, radius); // Don't allow <= 0 zoom

    // Update target pos using delta from user input
    target = glms_vec3_add(target, pos_delta);

    // Rendering breaks @ exactly 90 with Euler rotations, and we don't want to
    // be upside-down.
    orbit_angles.y = clampf(orbit_angles.y, glm_rad(-89.999f), glm_rad(89.999f));

    // Add target position to relative orbit position to get final position
    pos = glms_vec3_add(target, orbit_pos_by_angles(*this));
}

vec3s camera::facing() const noexcept {
    // In fly mode, the target & camera are swapped
    const vec3s target = (mode == CAMERA_ORBIT) ? this->target : this->pos;
    const vec3s pos = (mode == CAMERA_ORBIT) ? this->pos : this->target;
    return glms_normalize(glms_vec3_sub(target, pos));
}

void camera::set_mode(camera_mode new_mode) noexcept {
    if (new_mode == mode) {
        return; // Nothing to do.
    }

    switch (new_mode) {
        case CAMERA_ORBIT:
            invert_mouse_x = false;
            invert_mouse_y = false;
            mouse_sens = 0.015f;
            break;
        default:
        case CAMERA_FLY:
            invert_mouse_x = false;
            invert_mouse_y = true;
            mouse_sens = 0.005f;
            break;
    }
    // If entering or leaving orbit mode, the target will be swapped with the
    // camera. We need to face the opposite direction to correct for the change
    bool needs_view_flip = (mode == CAMERA_ORBIT || new_mode == CAMERA_ORBIT);
    if (needs_view_flip) {
        orbit_angles.x = fmodf(orbit_angles.x + glm_rad(180), 360);
        orbit_angles.y = -orbit_angles.y;
    }

    // Set mode
    mode = new_mode;
}

void camera::view_matrix(mat4 view_out) const noexcept {
    if (mode == CAMERA_ORBIT) {
        glm_lookat((float*)&pos, (float*)&target, (float*)&camera_up, view_out);
    } else {
        // In fly mode, the target & camera are swapped
        glm_lookat((float*)&target, (float*)&pos, (float*)&camera_up, view_out);
    }
}

void camera::proj_view(mat4 out) const noexcept {
    // Pre-multiply the projection & view components of the PVM matrix

    // Projection matrix
    mat4 projection = {0};
    int viewportVals[4] = {0};
    glGetIntegerv(GL_VIEWPORT, viewportVals);
    const float width = float(viewportVals[2]);
    const float height = float(viewportVals[3]);

    const float aspect = width / height;
    glm_perspective_rh_no(glm_rad(45), aspect, near_clip_plane, far_clip_plane, projection);

    // Camera matrix
    mat4 view = {0};
    this->view_matrix(view);
    glm_mat4_mul(projection, view, (vec4*)out);
}
