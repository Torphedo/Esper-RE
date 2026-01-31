#pragma once
#include "mesh_view.hxx"
#include <vector>
#include <GLFW/glfw3.h>

#include <common/int.h>
#include <formats/alr.h>
#include <formats/alr_animations.h>
#include <layer.hxx>

#include "camera.hxx"
#include "gui/framebuffer.hxx"

struct viewport_t : gui_layer {
    // Whether the viewport has been set up and can be rendered to.
    bool initialized = false;
    // Whether the viewport's framebuffer is visible to the user
    bool visible = false;

    // Whether the viewport editor window should render.
    bool editor_enabled = false;
    bool raycast_test = false;
    bool wireframe_selection = false;
    u16 selected_mesh = 0;

    u32 anim_id = BAS01_WAIT0;
    float anim_frame = 0.0f;

    framebuffer fbo;

    alr::file* alr = nullptr;

    // Shader program used to render the scene
    gl_obj shader = 0;

    // Uniform locations (to send data to the shader each frame)

    // Projection-view-model transform matrix
    gl_obj uniform_pvm = 0;

    // A value to divide the UVs by before using them
    gl_obj uniform_uv_divisor = 0;

    // 32-bit shader flag bitfield
    gl_obj uniform_flags = 0;

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

    // Shader settings bitfield
    struct shader_flags_t {
        bool render_texcoords: 1;
        bool render_normals: 1;
        bool has_normal: 1; // Whether this object even has a normal map
        u32: 0; // This pads the bitfield to 32 bits
    }shader_flags = {};

    // All meshes in the scene
    std::vector<mesh_view> meshes;

    // Set up a custom framebuffer. Returns whether it succeeded, you can also
    // check the [initialized] member.
    void init(GLFWwindow* window) noexcept override;

    void update(GLFWwindow* window) noexcept override;
    void render(GLFWwindow* window) noexcept override;
    void render_mesh(const mesh_view& mesh, mat4 pvm, bool allow_semi_transparent);

    // Destroys the underlying OpenGL resources and invalidates all copies of this instance.
    void destroy() noexcept override;
};
