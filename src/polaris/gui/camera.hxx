#pragma once
#include <cglm/struct.h>

// Up axis for our camera
static vec3s camera_up = {0.0f, -1.0f, 0.0f};

typedef enum {
    CAMERA_ORBIT, // 3rd-person dual-stick style
    CAMERA_POV, // POV Minecraft-style
    CAMERA_FLY, // Flying (freecam style, like Source Engine spectator)
    CAMERA_MODE_ENUM_MAX,
}camera_mode;

struct camera {
    vec3s target = {0}; // Position the camera looks towards
    vec3s pos = {0}; // Position of the viewer
    vec2s orbit_angles = {0};
    float radius = 30.0f;
    float move_speed = 15.0f;
    float mouse_sens = 0.005f;
    float near_clip_plane = 0.1f;
    float far_clip_plane = 10000.0f;

    bool invert_mouse_x = false;
    bool invert_mouse_y = true;
    camera_mode mode = CAMERA_POV;

    /// @brief Updates the camera mode.
    ///
    /// Use this instead of accessing the field directly, otherwise it may break.
    /// @param cam The camera to modify
    /// @param new_mode The new mode to use
    void set_mode(camera_mode new_mode) noexcept;

    /// @brief Update the camera state (should be called each frame)
    /// @param The camera to modify
    /// @param delta_time Time elapsed since the last call
    void update(double delta_time) noexcept;

    /// @brief Gets the unit direction vector the camera is looking
    vec3s facing() const noexcept;

    // Get just the camera transform
    void view_matrix(mat4 view) const noexcept;

    // Get combined projection & view matrix for the current camera position
    void proj_view(mat4 out) const noexcept;

    vec2s get_cursor_delta();
};
