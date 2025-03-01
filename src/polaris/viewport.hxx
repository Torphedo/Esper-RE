#pragma once
#include <imgui.h>
#include "renderlist.hxx"
#include <GLFW/glfw3.h>
#include <common/int.h>
#include <vector>
#include "camera.hxx"

/// @brief Wrapper class for a custom viewport renderable in ImGui
/// 
/// The basic viewport functionality is pretty simple, mostly coming from here:
/// https://learnopengl.com/Advanced-OpenGL/Framebuffers
struct viewport_t {
    // To avoid accidentally destroying OpenGL objects by creating & copying an
    // instance inline, the "real" ctor/dtor are .setup() and .destroy().
    // All methods will be no-ops until/unless [initialized] is set by .setup().
    viewport_t() = default;
    // TODO: Look into copy/move constructors?

    // Whether the viewport has been set up and can be rendered to.
    bool initialized = false;

    // Whether the Dear ImGui window is enabled
    bool enabled = false;

    // Custom framebuffer that can be rendered to
    gl_obj fbo = 0;

    // Color texture that backs the framebuffer. Render this to see the current
    // contents of the framebuffer
    gl_obj color_tex = 0;

    // Shader program
    gl_obj shader = 0;

    // Transformation matrix uniform for moving the view around.
    gl_obj uniform_pvm = 0;
    camera cam;
    bool cursor_lock = false;
    bool wireframe = true;

    std::vector<mesh_view> meshes;

    // Set up a custom framebuffer. Returns whether it succeeded, you can also
    // check the [initialized] member.
    bool setup(u16 width, u16 height) noexcept;

    // Basically a destructor, call if you *definitely* want to destroy the
    // underlying OpenGL resources and invalidate all copies of this instance.
    void destroy() noexcept;

    // Render a Dear ImGui window showing the viewport contents
    bool render_imgui(GLFWwindow* window) noexcept;

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
