#pragma once
#include "mesh_view.hxx"
#include <vector>
#include <GLFW/glfw3.h>

#include <common/int.h>
#include <layer.hxx>

#include "camera.hxx"
#include "framebuffer.hxx"

// We need a forward declaration instead of a header include, since a class
// in this file is a member of polaris.
class polaris;

struct viewport_t : gui_layer {
    // Whether the viewport has been set up and can be rendered to.
    bool initialized = false;

    // Whether the viewport editor window should render.
    bool editor_enabled = false;

    framebuffer fbo;

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
    
    // Normal map sampler
    gl_obj uniform_sampler_normal = 0;

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

    // Destroys the underlying OpenGL resources and invalidates all copies of this instance.
    void destroy() noexcept override;

    // Render a Dear ImGui editor for the viewport contents
    void render_editor(al::resource& alr) noexcept;

    // Render a Dear ImGui window showing the viewport contents
    bool render_contents(GLFWwindow* window, al::resource& pol) noexcept;
};
