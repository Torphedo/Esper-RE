#include "viewport.hxx"
#include <imgui.h>

#include <common/logging.h>

bool viewport_t::setup(u16 width, u16 height) noexcept {
    // The initialization flag defaults to failure, so we just early return on
    // failure and explicitly set success.

    // Setup the OpenGL objects we'll need
    glGenFramebuffers(1, &fbo);
    if (fbo == 0) {
        return false; // Maybe we should print an error here
    }
    glGenTextures(1, &this->color_tex);
    if (color_tex == 0) {
        glDeleteFramebuffers(1, &fbo); // Clean up
        return false; // Maybe we should print an error here
    }

    // Setup backing texture for framebuffer
    glBindTexture(GL_TEXTURE_2D, color_tex);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, width, height, 0, GL_RGBA, GL_UNSIGNED_BYTE, nullptr);
    // We only really care about the downscale filter
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glBindTexture(GL_TEXTURE_2D, 0);

    // Actually attach texture to the framebuffer
    glBindFramebuffer(GL_FRAMEBUFFER, fbo);
    glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, color_tex, 0);
    glBindFramebuffer(GL_FRAMEBUFFER, 0);


    if (glCheckFramebufferStatus(GL_FRAMEBUFFER) != GL_FRAMEBUFFER_COMPLETE) {
        // uh oh...
        LOG_MSG(warning, "Failed to setup a complete framebuffer!\n");
        initialized = false;
    } else {
        // execute victory dance
        // https://learnopengl.com/Advanced-OpenGL/Framebuffers
        LOG_MSG(info, "Successfully set up framebuffer!\n");
        initialized = true;
    }

    return initialized;
}

void viewport_t::destroy() noexcept {
    if (!initialized) {
        return;
    }
    glDeleteFramebuffers(1, &fbo);
    glDeleteTextures(1, &color_tex);
    initialized = false;
}

void viewport_t::render_imgui() noexcept {
    if (!enabled || !initialized) {
        return;
    }

    if (ImGui::Begin("Viewport", &this->enabled)) {
        bind();
        // Clear the screen as a test
        glClearColor(1.0f, 0, 0, 1.0f); // Set clear color
        glClear(GL_COLOR_BUFFER_BIT);
        ImGui::Image(color_tex, ImGui::GetContentRegionAvail());

        ImGui::End();
        unbind();
    }
}