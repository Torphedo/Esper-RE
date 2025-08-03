#pragma once
#include "mesh_view.hxx"
#include <vector>
#include <GLFW/glfw3.h>

#include <common/int.h>
#include "camera.hxx"

// We need a forward declaration instead of a header include, since a class
// in this file is a member of polaris.
class polaris;

enum {
    POL_TEXSLOT_ALBEDO,
    POL_TEXSLOT_NORMAL,
};

/// @brief Wrapper class for a custom viewport renderable in ImGui
/// 
/// The basic viewport functionality is pretty simple, mostly coming from here:
/// https://learnopengl.com/Advanced-OpenGL/Framebuffers
struct viewport_t {
    // Custom framebuffer that can be rendered to
    gl_obj fbo = 0;

    // Color texture that backs the framebuffer. Render this to see the current
    // contents of the framebuffer
    gl_obj color_tex = 0;

    // Depth buffer to back the framebuffer's depth test
    gl_obj depth_tex = 0;

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

    // Whether the viewport has been set up and can be rendered to.
    bool initialized = false;

    // Whether the viewport window should render
    bool enabled = false;

    // Whether the viewport editor window should render.
    bool editor_enabled = false;

    // Wireframe mode toggle
    bool wireframe = false;

    // Backface culling toggle
    bool backface_cull = true;

    // Shader settings bitfield
    struct shader_flags_t {
        bool render_texcoords: 1;
        bool render_normals: 1;
        bool has_normal: 1; // Whether this object has a normal map
        bool has_albedo: 1; // Whether this object has an albedo texture
        u32: 0; // This pads the bitfield to 32 bits

        // Implicit conversion
        operator u32() {
            return *(u32*)this;
        }
    }shader_flags = {};

    // All meshes in the scene
    std::vector<mesh_view> meshes;

    // Setup requires an active OpenGL context, so the "real" ctor is .setup().
    // All methods will be no-ops until [initialized] is set by .setup().
    viewport_t() = default;

    // Set up a custom framebuffer. Returns whether it succeeded, you can also
    // check the [initialized] member.
    bool setup(u16 width, u16 height) noexcept;

    // Destroys the underlying OpenGL resources and invalidates all copies of this instance.
    ~viewport_t() noexcept;

    // Render a Dear ImGui editor for the viewport contents
    void render_editor(const polaris* pol) noexcept;

    // Render a Dear ImGui window showing the viewport contents
    bool render_contents(GLFWwindow* window, const polaris* pol) noexcept;

    // Simple wrapper methods for those who like them
    void bind() const noexcept {
        if (initialized) {
            glBindFramebuffer(GL_FRAMEBUFFER, this->fbo);
        }
    }

    void unbind() const noexcept {
        if (initialized) {
            glBindFramebuffer(GL_FRAMEBUFFER, 0);
        }
    }
};
