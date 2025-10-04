#include "framebuffer.hxx"
#include <glad/glad.h>
#include <GLFW/glfw3.h>

#include <common/logging.h>

void framebuffer::bind() const noexcept {
    if (initialized) {
        glViewport(0, 0, width, height);
        glBindFramebuffer(GL_FRAMEBUFFER, this->fbo);
    }
}

void framebuffer::unbind() const noexcept {
    if (initialized) {
        const GLFWvidmode* mode = glfwGetVideoMode(glfwGetPrimaryMonitor());
        glViewport(0, 0, mode->width, mode->height);
        glBindFramebuffer(GL_FRAMEBUFFER, 0);
    }
}

bool framebuffer::setup(u16 new_width, u16 new_height) noexcept {
    // Setup the OpenGL objects we'll need
    glGenFramebuffers(1, &fbo);
    if (fbo == 0) {
        LOG_MSG(error, "Failed to setup framebuffer object for viewport!\n");
        return false;
    }
    glGenTextures(1, &this->color_tex);
    glGenTextures(1, &this->depth_tex);
    if (color_tex == 0 || depth_tex == 0) {
        if (color_tex != 0) {
            glDeleteTextures(1, &this->color_tex);
        }
        if (depth_tex != 0) {
            glDeleteTextures(1, &this->depth_tex);
        }
        glDeleteFramebuffers(1, &fbo); // Clean up
        LOG_MSG(error, "Failed to setup framebuffer backing texture for viewport!\n");
        return false;
    }

    // Setup backing color texture for framebuffer
    glBindTexture(GL_TEXTURE_2D, color_tex);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, new_width, new_height, 0, GL_RGBA, GL_UNSIGNED_BYTE, nullptr);
    // We only really care about the downscale filter
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);

    // Setup backing depth texture for framebuffer
    glBindTexture(GL_TEXTURE_2D, depth_tex);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_DEPTH_COMPONENT, new_width, new_height, 0, GL_DEPTH_COMPONENT, GL_UNSIGNED_BYTE, nullptr);

    // Actually attach texture to the framebuffer
    glBindFramebuffer(GL_FRAMEBUFFER, fbo);
    glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, color_tex, 0);
    glFramebufferTexture2D(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, GL_TEXTURE_2D, depth_tex, 0);
    // Enable depth testing for this framebuffer since we set up a depth buffer
    glEnable(GL_DEPTH_TEST);

    if (glCheckFramebufferStatus(GL_FRAMEBUFFER) != GL_FRAMEBUFFER_COMPLETE) {
        // uh oh...
        LOG_MSG(warning, "Failed to setup a complete framebuffer!\n");
    } else {
        // execute victory dance
        // This should always succeed, we don't bother printing
        initialized = true;
    }

    glClearColor(0.0f, 0.0f, 0.0f, 1.0f);

    // Clean up our state
    glBindTexture(GL_TEXTURE_2D, 0);
    glBindFramebuffer(GL_FRAMEBUFFER, 0);

    if (initialized) {
        width = new_width;
        height = new_height;
    }

    return initialized;
}

void framebuffer::destroy() noexcept {
    glDeleteFramebuffers(1, &fbo);
    glDeleteTextures(1, &color_tex);
    glDeleteTextures(1, &depth_tex);
}
