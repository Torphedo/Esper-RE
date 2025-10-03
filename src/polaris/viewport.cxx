#define IMGUI_DEFINE_MATH_OPERATORS
#include "viewport.hxx"
#include <imgui.h>
#include "imgui_utils.hxx"

#include "mesh_view.hxx"
#include "polaris.hxx"

#include <common/vfile.h>
#include <common/logging.h>

extern "C" {
    #include <common/gl/shader.h>
    #include <common/gl/input.h>
}

// We ought to split these shaders into other files, but the code is so
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
uniform sampler2D normal_texture;
uniform vec3 cam_dir;
uniform int flags = 0;

void main() {
    bool render_uv_colors = (flags & 1) != 0;
    bool render_normal_colors = (flags & 2) != 0;
    bool has_normal = (flags & 4) != 0;

    vec4 color = vec4(texcoord, 0.0, 1.0);
    // I figure avoiding a texture sample is worth an if statement. - torph
    if (!render_uv_colors) {
        color = texture(albedo_texture, texcoord);
    }

    // TODO: Do alpha blending here. This is low-priority since most textures have BC1 1-bit alpha (except for a few normal maps).
    if (color.a < 0.1) {
        discard;
    }

    vec3 normal_vec = cam_dir;
    if (has_normal) {
        normal_vec = texture(normal_texture, texcoord).rgb;
        normal_vec = (normal_vec * 2.0) - 1.0;
    }
    const float ambient = 0.3f;
    float diffuse_factor = abs(dot(cam_dir, normal_vec)) + ambient;

    fragment_rgba = color * diffuse_factor;
    fragment_rgba.a = 1.0;

    if (render_normal_colors) {
        fragment_rgba = vec4(normal_vec, 1.0);
    }
}
)";

bool viewport_t::setup(u16 new_width, u16 new_height) noexcept {
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

    shader = program_compile_src(vertex_shader, fragment_shader);
    if (!shader_link_check(shader)) {
        LOG_MSG(error, "Shader compilation error!\n");
        return false;
    }
    uniform_pvm = glGetUniformLocation(shader, "pvm");
    uniform_uv_divisor = glGetUniformLocation(shader, "uv_divisor");
    uniform_flags = glGetUniformLocation(shader, "flags");
    uniform_cam_dir = glGetUniformLocation(shader, "cam_dir");

    uniform_sampler_albedo = glGetUniformLocation(shader, "albedo_texture");
    uniform_sampler_normal = glGetUniformLocation(shader, "normal_texture");

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

    if (initialized) {
        width = new_width;
        height = new_height;
    }

    // This defaults to false
    return initialized;
}

viewport_t::~viewport_t() noexcept {
    if (initialized) {
        glDeleteFramebuffers(1, &fbo);
        glDeleteTextures(1, &color_tex);
        glDeleteTextures(1, &depth_tex);
        glDeleteProgram(shader);
        for (mesh_view mesh : meshes) {
            mesh.destroy();
        }

        initialized = false;
    }
}

void viewport_t::render_editor(al::resource& alr) noexcept {
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
    mesh.edit_menu(alr);

    ImGui::End();
}

bool viewport_t::render_contents(GLFWwindow* window, al::resource& alr) noexcept {
    if (!enabled || !initialized) {
        return false;
    }

    // Editor window
    this->render_editor(alr);

    // Calculate delta time every time we render
    static double prev_time = glfwGetTime();
    const double cur_time = glfwGetTime();
    const double delta_time = cur_time - prev_time;
    prev_time = cur_time;

    bool is_hovered = false;
    ImGui::Begin("Viewport");
    {
        // Start rendering to the viewport
        bind();
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

        // Get camera transform
        mat4 pvm = {0};
        cam.proj_view(pvm);

        // Wireframe toggle
        const float padding = ImGui::GetStyle().FramePadding.x * 2;
        bool wireframe_changed = ImGui::Checkbox("Wireframe", &wireframe);
        ImGui::SameLine();
        bool cull_changed = ImGui::Checkbox("Back-face culling", &backface_cull);

        ImGui::SameLine();
        bool temp_render_texcoords = shader_flags.render_texcoords;
        ImGui::Checkbox("Visualize UVs", &temp_render_texcoords);
        shader_flags.render_texcoords = temp_render_texcoords;

        ImGui::SameLine();
        bool temp_render_normals = shader_flags.render_normals;
        ImGui::Checkbox("Visualize normals", &temp_render_normals);
        shader_flags.render_normals = temp_render_normals;

        ImGui::SameLine();
        static bool no_transforms = false;
        ImGui::Checkbox("Disable object transforms", &no_transforms);

        ImGui::SameLine();
        bool temp_force_disable_normals = shader_flags.has_normal;
        ImGui::Checkbox("Use normals", &temp_force_disable_normals);
        shader_flags.has_normal = temp_force_disable_normals;

        // Need this ridiculous workaround to make sure options don't take up
        // like half the horizontal screen space
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

        // Actually apply state toggles now that the framebuffer is bound
        if (wireframe_changed) {
            if (wireframe) {
                glPolygonMode(GL_FRONT_AND_BACK, GL_LINE);
            } else {
                glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);
            }
        }
        if (cull_changed) {
            if (backface_cull) {
                glEnable(GL_CULL_FACE);
            } else {
                glDisable(GL_CULL_FACE);
            }
        }

        // Bind shader & upload camera transform
        glUseProgram(shader);
        glUniformMatrix4fv(uniform_pvm, 1, GL_FALSE, (float*)pvm);
        glUniform3fv(uniform_cam_dir, 1, cam.facing().raw);
        glUniform1i(uniform_flags, *((u32*)&shader_flags));

        glUniform1i(uniform_sampler_albedo, 0);
        glUniform1i(uniform_sampler_normal, 1);

        // Render all index buffers of all known meshes
        for (const mesh_view& mesh : meshes) {
            if (!mesh.active) {
                continue; // This mesh is hidden
            }

            glUniform1ui(uniform_uv_divisor, mesh.uv_divisor);


            glBindVertexArray(mesh.vao);
            for (index_buffer idx_buf : mesh.idx_buffers) {
                if (!idx_buf.enabled) {
                    continue; // This index buffer is hidden
                }
                mat4s obj_pvm = *(mat4s*)pvm;
                if (!no_transforms) {
                    obj_pvm = glms_mul(obj_pvm, idx_buf.transform);
                }
                glUniformMatrix4fv(uniform_pvm, 1, GL_FALSE, (float*)obj_pvm.raw);

                // We cast away const here but don't write to the buffer
                vfile vf = vfile_open(alr.data, alr.alr_size);
                vf.pos = idx_buf.idx_chunk_offset;
                vfile_seek(&vf, sizeof(chunk_generic));
                const auto header = VFILE_READ(idxbuf_header, &vf);
                const auto mat_chunk = alr.prev_chunk_by_id(0x1, idx_buf.idx_chunk_offset);
                chunk_0x1_entry tex_entry = {};
                alr.tex_manager.get_material(alr, mat_chunk.offset, header.texture_idx, &tex_entry);

                glActiveTexture(GL_TEXTURE0);
                glBindTexture(GL_TEXTURE_2D, alr.tex_manager.get(alr, tex_entry.texture_idx));
                glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
                glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);

                gl_obj normal_idx = tex_entry.normal_idx;
                if (tex_entry.vertbuf_format == 0x1F) {
                    normal_idx = tex_entry.normal_backup_idx;
                }

                shader_flags_t flags = this->shader_flags;
                if (flags.has_normal) {
                    flags.has_normal = (normal_idx != 0);
                }
                glUniform1i(uniform_flags, *((u32*)&flags));

                glActiveTexture(GL_TEXTURE1);
                glBindTexture(GL_TEXTURE_2D, alr.tex_manager.get(alr, normal_idx));
                glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
                glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);

                const u16 draw_mode = (header.primitive_type == IDX_TYPE_STRIP) ? GL_TRIANGLE_STRIP : GL_TRIANGLES;
                glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, idx_buf.obj);
                glDrawElements(draw_mode, header.num_indices, GL_UNSIGNED_SHORT, 0);
            }
            // VAO keeps index buffer binding, so clear it after draw.
            glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, 0);
            glBindTexture(GL_TEXTURE_2D, 0);
            glBindVertexArray(0);
        }

        ImVec2 image_size = ImVec2(width, height);
        const float scale = ImGui::ImageScaleForWindow(width, height);
        image_size *= scale;

        ImGui::Image(color_tex, image_size);
        is_hovered = ImGui::IsItemHovered();
        if (ImGui::IsMouseClicked(0)) {
            if (is_hovered) {
                glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_DISABLED);

                // Get non-accelerated input if possible
                if (glfwRawMouseMotionSupported()) {
                    glfwSetInputMode(window, GLFW_RAW_MOUSE_MOTION, GLFW_TRUE);
                }
                cursor_lock = true;
            }
        }
        if (ImGui::IsMouseReleased(0)) {
            ImGui::GetIO().WantCaptureMouse = true;
            ImGui::GetIO().WantCaptureKeyboard = true;
            glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_NORMAL);
            glfwSetInputMode(window, GLFW_RAW_MOUSE_MOTION, GLFW_FALSE);
            cursor_lock = false;
        }

        if (cursor_lock) {
            ImGui::GetIO().WantCaptureMouse = false;
            ImGui::GetIO().WantCaptureKeyboard = false;
            cam.update(delta_time);
        }

        glUseProgram(0);
        unbind(); // Reset state
    }
    ImGui::End();

    return is_hovered;
}
