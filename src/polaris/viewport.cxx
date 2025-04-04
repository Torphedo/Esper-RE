#include "viewport.hxx"
#include "polaris/renderlist.hxx"
#include <imgui.h>
#include "imgui_utils.hxx"

#include <common/logging.h>

extern "C" {
    #include <common/gl/shader.h>
    #include <common/gl/input.h>
}

// We ought to split these shaders into other files, but the GLSL code is so
// trivial that it's not really worth it.
const char* vertex_shader = R"(
#version 330 core
layout (location = 0) in vec3 a_pos;
layout (location = 1) in vec2 a_texcoord;

uniform mat4 pvm;
uniform uint uv_divisor;
out vec2 texcoord;

void main() {
    gl_Position = pvm * vec4(a_pos, 1.0);

    // We map the large integer value into the [0, 1] range for texture lookups
    texcoord = a_texcoord / uv_divisor;
}
)";

const char* fragment_shader = R"(
#version 330 core
out vec4 fragment_rgba;

in vec2 texcoord;
uniform sampler2D albedo_texture;

void main() {
    // fragment_rgba = vec4(texcoord, 0.0f, 1.0f);
    fragment_rgba = texture(albedo_texture, texcoord);

    // Solid yellow
    // fragment_rgba = vec4(1.0f, 0.906f, 0.258f, 1.0f);
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
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, width, height, 0, GL_RGBA, GL_UNSIGNED_BYTE, nullptr);
    // We only really care about the downscale filter
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);

    // Setup backing depth texture for framebuffer
    glBindTexture(GL_TEXTURE_2D, depth_tex);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_DEPTH_COMPONENT, width, height, 0, GL_DEPTH_COMPONENT, GL_UNSIGNED_BYTE, nullptr);

    // Actually attach texture to the framebuffer
    glBindFramebuffer(GL_FRAMEBUFFER, fbo);
    glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, color_tex, 0);
    glFramebufferTexture2D(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, GL_TEXTURE_2D, depth_tex, 0);
    // Enable depth testing for this framebuffer since we set up a depth buffer
    glEnable(GL_DEPTH_TEST);

    shader = program_compile_src(vertex_shader, fragment_shader);
    if (!shader_link_check(shader)) {
        LOG_MSG(error, "Shader compilation error!\n");
        return false;
    }
    uniform_pvm = glGetUniformLocation(shader, "pvm");
    uniform_uv_divisor = glGetUniformLocation(shader, "uv_divisor");

    if (glCheckFramebufferStatus(GL_FRAMEBUFFER) != GL_FRAMEBUFFER_COMPLETE) {
        // uh oh...
        LOG_MSG(warning, "Failed to setup a complete framebuffer!\n");
    } else {
        // execute victory dance
        // This should always succeed, we don't bother printing
        initialized = true;
    }

    glClearColor(0.0f, 0.0f, 0.0f, 1.0f);

    if (wireframe) {
        glPolygonMode(GL_FRONT_AND_BACK, GL_LINE);
    }

    // Clean up our state
    glBindTexture(GL_TEXTURE_2D, 0);
    glBindFramebuffer(GL_FRAMEBUFFER, 0);

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

void viewport_t::render_editor(const std::vector<gl_obj>& tex_array) noexcept {
    if (!editor_enabled) {
        return;
    }
    ImGui::Begin("Viewport Editor", &editor_enabled);

    static u16 selected_mesh = 0;
    ImGui::InputU16("Selected Mesh", &selected_mesh);
    selected_mesh %= meshes.size();

    // If you make this loop over all meshes in the future, make sure not to
    // use the for loop style with a colon (or make sure you get a reference),
    // otherwise it'll run the menu on a copy and not modify the data
    mesh_view& mesh = meshes.at(selected_mesh);
    mesh.edit_menu(tex_array);

    ImGui::End();
}

bool viewport_t::render_contents(GLFWwindow* window, const std::vector<gl_obj>& tex_array) noexcept {
    if (!enabled || !initialized) {
        return false;
    }

    // Editor window
    this->render_editor(tex_array);

    // Calculate delta time every time we render
    static double prev_time = glfwGetTime();
    const double cur_time = glfwGetTime();
    const double delta_time = cur_time - prev_time;
    prev_time = cur_time;

    bool is_hovered = false;
    ImGui::Begin("Viewport");
    {
        // Get camera transform
        mat4 pvm = {0};
        cam.proj_view(pvm);

        // Wireframe toggle
        const float padding = ImGui::GetStyle().FramePadding.x * 2;
        bool wireframe_changed = ImGui::Checkbox("Wireframe", &wireframe);
        ImGui::SameLine();
        bool cull_changed = ImGui::Checkbox("Back-face culling", &cull_back_faces);

        // Need to do this ridiculous workaround to manually ensure options
        // don't take up like half the horizontal screen space
        const char* options[] = {"Orbit", "Minecraft", "Fly"};
        const char* label = "Camera Mode";
        const float combo_width = ImGui::CalcTextSize(options[1]).x * 1.5f + padding;
        ImGui::SetNextItemWidth(combo_width);
        ImGui::SameLine();

        camera_mode cur_mode = cam.mode;
        ImGui::Combo(label, (int*)&cur_mode, options, CAMERA_MODE_ENUM_MAX);
        if (cur_mode != cam.mode) {
            // We need to use the setter instead of overwriting directly to get
            // correct behaviour.
            cam.set_mode(cur_mode);
        }

        ImGui::SameLine();
        const char* move_speed_label = "Move Speed";
        ImGui::SetNextItemWidth(ImGui::CalcTextSize(move_speed_label).x + 20.0f + padding);
        ImGui::SliderFloat(move_speed_label, &cam.move_speed, 0.1f, 1000.0f);

        // Start rendering to the viewport
        bind();
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

        // Actually apply state toggles now that the framebuffer is bound
        if (wireframe_changed) {
            if (wireframe) {
                glPolygonMode(GL_FRONT_AND_BACK, GL_LINE);
            } else {
                glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);
            }
        }
        if (cull_changed) {
            if (cull_back_faces) {
                glEnable(GL_CULL_FACE);
            } else {
                glDisable(GL_CULL_FACE);
            }
        }

        // Bind shader & upload camera transform
        glUseProgram(shader);
        glUniformMatrix4fv(uniform_pvm, 1, GL_FALSE, (float*)pvm);

        // Render all index buffers of all known meshes
        for (mesh_view mesh : meshes) {
            if (!mesh.active) {
                continue; // This mesh is hidden
            }

            glUniform1ui(uniform_uv_divisor, mesh.uv_divisor);

            glActiveTexture(GL_TEXTURE0);
            glBindTexture(GL_TEXTURE_2D, tex_array.at(mesh.albedo_tex_idx));
            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);

            glBindVertexArray(mesh.vao);
            for (index_buffer idx_buf : mesh.idx_buffers) {
                if (!idx_buf.enabled) {
                    continue; // This index buffer is hidden
                }
                glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, idx_buf.obj);
                glDrawElements(mesh.draw_mode, idx_buf.num, GL_UNSIGNED_SHORT, 0);
            }
            // VAO keeps index buffer binding, so clear it after draw.
            glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, 0);
            glBindTexture(GL_TEXTURE_2D, 0);
            glBindVertexArray(0);
        }

        ImGui::Image(color_tex, ImGui::GetContentRegionAvail());
        const bool mouse_click = ImGui::IsMouseDown(0);
        is_hovered = ImGui::IsItemHovered();
        if (is_hovered && mouse_click) {
            if (!cursor_lock) {
                glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_DISABLED);

                // Get non-accelerated input if possible
                if (glfwRawMouseMotionSupported()) {
                    glfwSetInputMode(window, GLFW_RAW_MOUSE_MOTION, GLFW_TRUE);
                }
                cursor_lock = true;
            }
        }
        else if (!mouse_click && cursor_lock) {
            // Disable when left click is released
            glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_NORMAL);
            glfwSetInputMode(window, GLFW_RAW_MOUSE_MOTION, GLFW_FALSE);
            cursor_lock = false;
        }

        if (is_hovered) {
            // Update the camera state
            cam.update(delta_time);
        }

        glUseProgram(0);
        unbind(); // Reset state
    }
    ImGui::End();

    return is_hovered;
}
