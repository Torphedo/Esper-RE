#pragma once
#include <optional>
#include <common/int.h>
#include <layer.hxx>
#include <formats/alr_animations.h>

#include "camera.hxx"
#include "framebuffer.hxx"
#include "selector_ray.hxx"

struct render_context : gui_layer {
    // Whether the viewport has been set up and can be rendered to.
    bool initialized = false;
    // Whether the viewport's framebuffer is visible to the user
    bool visible = false;

    // Whether the viewport editor window should render.
    bool editor_enabled = false;
    bool wireframe_selection = false;
    bool show_bounding_boxes = false;
    u16 selected_mesh = 0;

    u32 anim_id = BAS01_WAIT0;
    float anim_frame = 0.0f;

    std::optional<ray_t> click_ray;

    framebuffer fbo;

    // Shader program used to render the scene
    gl_obj active_shader = 0;

    gl_obj diffuse_shader = 0;
    gl_obj vkblink_shader = 0;
    gl_obj uv_shader = 0;
    gl_obj skinned_shader = 0;
    gl_obj cube_shader = 0;

    gl_obj blank_vao = 0;

    // Uniform locations (to send data to the shader each frame)

    // Skinning matrix array
    gl_obj uniform_skin_xforms = 0;

    // Projection-view-model transform matrix
    gl_obj uniform_pvm = 0;

    // A value to divide the UVs by before using them
    gl_obj uniform_uv_divisor = 0;

    // Camera view direction vector
    gl_obj uniform_cam_dir = 0;

    // Color texture sampler. We call it albedo since it's the same number of
    // characters as "normal" and means the same thing.
    gl_obj uniform_sampler_albedo = 0;
    gl_obj uniform_sampler_normal = 0;
    gl_obj uniform_sampler_lightmap = 0;

    // Camera, misc. rendering state
    camera cam;
    bool cursor_lock = false; // For infinite camera panning

    // Wireframe mode toggle
    bool wireframe = false;
    // Backface culling toggle
    bool backface_cull = true;

    bool render_texcoords = false;
    bool render_skinning = false;

    // Set up a custom framebuffer.
    // Check the [initialized] member to see if it succeeded.
    void init(GLFWwindow* window) noexcept override;

    void update(GLFWwindow* window) noexcept override;
    void bind() noexcept;
    void unbind() noexcept;

    // Destroys the underlying OpenGL resources and invalidates all copies of this instance.
    void destroy() noexcept override;

    void set_shader(gl_obj shader, bool force = false) noexcept;
};
