#pragma once
#include <common/int.h>

/// @brief Wrapper class for rendering to a texture
///
/// The basic functionality is pretty simple, mostly coming from here:
/// https://learnopengl.com/Advanced-OpenGL/Framebuffers
struct framebuffer {
    bool initialized = false;
    u16 width = 0;
    u16 height = 0;

    // Custom framebuffer that can be rendered to
    gl_obj fbo = 0;

    // Color texture that backs the framebuffer. Render this to see the current
    // contents of the framebuffer
    gl_obj color_tex = 0;

    // Depth buffer to back the framebuffer's depth test
    gl_obj depth_tex = 0;

    // Set up a custom framebuffer. Returns whether it succeeded, you can also
    // check the [initialized] member.
    bool setup(u16 new_width, u16 new_height) noexcept;

    void destroy() noexcept;

    // Simple wrapper methods for those who like them
    void bind() const noexcept;
    void unbind() const noexcept;
};