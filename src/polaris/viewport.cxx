#include "viewport.hxx"
#include <imgui.h>

#include <common/logging.h>

extern "C" {
    #include <common/gl/shader.h>
}

const char* vertex_shader = R"(
#version 330 core
layout (location = 0) in vec3 a_pos;
// layout (location = 1) in vec4 a_color;

void main() {
    gl_Position = vec4(a_pos, 1.0);
}
)";

const char* fragment_shader = R"(
#version 330 core
out vec4 fragment_rgba;

void main() {
    fragment_rgba = vec4(1.0f, 1.0f, 1.0f, 1.0f);
}
)";

bool viewport_t::setup(u16 width, u16 height) noexcept {
    // Setup the OpenGL objects we'll need
    glGenFramebuffers(1, &fbo);
    if (fbo == 0) {
        LOG_MSG(error, "Failed to setup framebuffer object for viewport!\n");
        return false;
    }
    glGenTextures(1, &this->color_tex);
    if (color_tex == 0) {
        glDeleteFramebuffers(1, &fbo); // Clean up
        LOG_MSG(error, "Failed to setup framebuffer backing texture for viewport!\n");
        return false;
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

    shader = program_compile_src(vertex_shader, fragment_shader);
    if (!shader_link_check(shader)) {
        LOG_MSG(error, "Shader compilation error!\n");
        return false;
    }

    if (glCheckFramebufferStatus(GL_FRAMEBUFFER) != GL_FRAMEBUFFER_COMPLETE) {
        // uh oh...
        LOG_MSG(warning, "Failed to setup a complete framebuffer!\n");
    } else {
        // execute victory dance
        // This should always succeed, we don't bother printing
        initialized = true;
    }

    // This defaults to false
    return initialized;
}

void viewport_t::destroy() noexcept {
    if (initialized) {
        glDeleteFramebuffers(1, &fbo);
        glDeleteTextures(1, &color_tex);
        for (mesh_view mesh : meshes) {
            mesh.destroy();
        }

        initialized = false;
    }
}

void viewport_t::render_imgui() noexcept {
    if (!enabled || !initialized) {
        return;
    }

    if (ImGui::Begin("Viewport", &this->enabled)) {
        bind();
        glUseProgram(shader);
        // Clear as a test
        glClear(GL_COLOR_BUFFER_BIT);

        for (mesh_view mesh : meshes) {
            glBindVertexArray(mesh.vao);
            for (index_buffer idx_buf : mesh.idx_buffers) {
                LOG_MSG(debug, "Drawing %d elements\n", idx_buf.num);
                glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, idx_buf.obj);
                glDrawElements(mesh.draw_mode, idx_buf.num, idx_buf.indices_type, 0);
            }
            glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, 0);
            glBindVertexArray(0);
        }

        ImGui::Image(color_tex, ImGui::GetContentRegionAvail());

        ImGui::End();
        glUseProgram(0);
        unbind();
    }
}
