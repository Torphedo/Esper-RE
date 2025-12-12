#define IMGUI_DEFINE_MATH_OPERATORS
#include "viewport.hxx"
#include <imgui.h>
#include "imgui_utils.hxx"

#include <common/vfile.h>
#include <common/logging.h>

#include "mesh_view.hxx"
#include "polaris.hxx"
#include "selector_ray.hxx"

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

void viewport_t::init(GLFWwindow* window) noexcept {
    // Have the viewport render in full resolution, it'll be downscale when
    // rendered as a texture by ImGui::Image
    int width = 0;
    int height = 0;
    glfwGetFramebufferSize(window, &width, &height);
    initialized = fbo.setup(width, height);
    if (!initialized) {
        return;
    }

    fbo.bind();
    shader = program_compile_src(vertex_shader, fragment_shader);
    if (!shader_link_check(shader)) {
        LOG_MSG(error, "Shader compilation error!\n");
        return;
    }
    uniform_pvm = glGetUniformLocation(shader, "pvm");
    uniform_uv_divisor = glGetUniformLocation(shader, "uv_divisor");
    uniform_flags = glGetUniformLocation(shader, "flags");
    uniform_cam_dir = glGetUniformLocation(shader, "cam_dir");

    uniform_sampler_albedo = glGetUniformLocation(shader, "albedo_texture");
    uniform_sampler_normal = glGetUniformLocation(shader, "normal_texture");

    if (wireframe) {
        glPolygonMode(GL_FRONT_AND_BACK, GL_LINE);
    }
    fbo.unbind();
}

void viewport_t::destroy() noexcept {
    if (initialized) {
        fbo.destroy();
        glDeleteProgram(shader);
        for (mesh_view mesh : meshes) {
            mesh.destroy();
        }

        initialized = false;
    }
}

void viewport_t::update(GLFWwindow* window) noexcept {
    if (!active || !initialized) {
        return;
    }

    if (editor_enabled) {
        if (ImGui::Begin("Viewport Editor", &this->editor_enabled)) {

            ImGui::InputU16("Selected Mesh", &selected_mesh);
            selected_mesh %= meshes.size();

            // If you make this loop over all meshes in the future, make sure not to
            // use the for loop style with a colon (or make sure you get a reference),
            // otherwise it'll run the menu on a copy and not modify the data
            mesh_view &mesh = meshes.at(selected_mesh);
            mesh.edit_menu(*alr);

        }
        ImGui::End();
    }

    ImGui::Begin("Viewport");
    // Calculate delta time every time we render
    static double prev_time = glfwGetTime();
    const double cur_time = glfwGetTime();
    const double delta_time = cur_time - prev_time;
    prev_time = cur_time;
    const float padding = ImGui::GetStyle().FramePadding.x * 2;

    // Wireframe toggle
    fbo.bind();
    if (ImGui::Checkbox("Wireframe", &wireframe)) {
        fbo.set_wireframe(wireframe);
    }

    ImGui::SameLine();
    if (ImGui::Checkbox("Back-face culling", &backface_cull)) {
        fbo.set_backface_cull(backface_cull);
    }
    fbo.unbind();

    ImGui::SameLine();
    bool temp_render_texcoords = shader_flags.render_texcoords;
    ImGui::Checkbox("Visualize UVs", &temp_render_texcoords);
    shader_flags.render_texcoords = temp_render_texcoords;

    ImGui::SameLine();
    bool temp_render_normals = shader_flags.render_normals;
    ImGui::Checkbox("Visualize normals", &temp_render_normals);
    shader_flags.render_normals = temp_render_normals;

    ImGui::SameLine();
    bool temp_force_disable_normals = shader_flags.has_normal;
    ImGui::Checkbox("Use normals", &temp_force_disable_normals);
    shader_flags.has_normal = temp_force_disable_normals;

    // Need this ridiculous workaround to make sure options don't take up
    // like half the horizontal screen space
    const char *options[] = {"Orbit", "Minecraft", "Fly"};
    const char *label = "Camera Mode";
    const float combo_width = ImGui::CalcTextSize(options[1]).x * 1.5f + padding;
    ImGui::SetNextItemWidth(combo_width);
    ImGui::SameLine();

    camera_mode cur_mode = cam.mode;
    ImGui::Combo(label, (int *) &cur_mode, options, CAMERA_MODE_ENUM_MAX);
    if (cur_mode != cam.mode) {
        // We need to use the setter instead of overwriting directly to get
        // correct behaviour.
        cam.set_mode(cur_mode);
    }

    ImGui::SameLine();
    const char *move_speed_label = "Move Speed";
    ImGui::SetNextItemWidth(ImGui::CalcTextSize(move_speed_label).x + 20.0f + padding);
    ImGui::SliderFloat(move_speed_label, &cam.move_speed, 0.1f, 1000.0f);


    ImGui::SameLine();
    ImGui::Checkbox("Enable raycast test", &raycast_test);

    ImVec2 fb_start = ImGui::GetCursorScreenPos();
    vec2s mouse_pos = {};
    {
        double x, y;
        glfwGetCursorPos(window, &x, &y);
        mouse_pos = {float(x), float(y)};

        const vec2s fb_pos = {fb_start.x, fb_start.y};
        mouse_pos = glms_vec2_sub(mouse_pos, fb_pos);
    };

    ImVec2 image_size = ImVec2(fbo.width, fbo.height);
    const float scale = ImGui::ImageScaleForWindow(fbo.width, fbo.height);
    image_size *= scale;
    glms_vec2_scale(mouse_pos, scale);

    ImGui::Image(fbo.color_tex, image_size);
    if (ImGui::IsMouseClicked(0)) {
        if (ImGui::IsItemHovered()) {
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
        if (raycast_test) {
            const vec4s fb_viewport = {
                // .x = fb_start.x, .y = fb_start.y,
                .z = image_size.x, .w = image_size.y,
            };
            ray_t ray = screen_to_ray(mouse_pos, cam, fb_viewport);

            for (u32 i = 0; i < meshes.size(); i++) {
                const mesh_view& mesh = meshes[i];
                if (!mesh.active) {
                    continue;
                }
                bool got_selected = false;
                for (const index_buffer& idxbuf : mesh.idx_buffers) {
                    if (!idxbuf.enabled) {
                        continue;
                    }
                    vfile vf = vfile_open(alr->data, alr->alr_size);
                    vf.pos = idxbuf.idx_chunk_offset;
                    const auto generic_0x2 = VFILE_READ(chunk_generic, &vf);
                    assert(generic_0x2.id == 2);
                    const auto* alr_idxbuf = (idxbuf_header*)vfile_cur(vf);
                    if (raycast(ray, mesh.vertices, mesh.vertex_size, idxbuf.get_transform(*alr), alr_idxbuf)) {
                        got_selected = true;
                        break;
                    }
                }
                if (got_selected) {
                    selected_mesh = i;
                    break;
                }
            }

            // ray.dir = glms_vec3_scale(ray.dir, 0.1f);
            // cam.target = glms_vec3_add(cam.target, ray.dir);
            // cam.pos = glms_vec3_add(cam.pos, ray.dir);
        } else {
            cam.update(delta_time);
        }
    }

    ImGui::End();
}

void viewport_t::render(GLFWwindow* window) noexcept {
    if (!active || !initialized) {
        return;
    }

    // Start rendering to the viewport
    fbo.bind();
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

    // Get camera transform
    mat4 pvm = {0};
    cam.proj_view(pvm);

    // Bind shader & upload camera transform
    glUseProgram(shader);
    glUniformMatrix4fv(uniform_pvm, 1, GL_FALSE, (float*)pvm);
    glUniform3fv(uniform_cam_dir, 1, cam.facing().raw);
    glUniform1i(uniform_flags, *((u32*)&shader_flags));

    glUniform1i(uniform_sampler_albedo, 0);
    glUniform1i(uniform_sampler_normal, 1);

    // Render all index buffers of all known meshes
    for (u32 i = 0; i < meshes.size(); i++) {
        const mesh_view& mesh = meshes[i];
        if (!mesh.active) {
            continue; // This mesh is hidden
        }
        fbo.set_wireframe((i == selected_mesh) || wireframe);

        glUniform1ui(uniform_uv_divisor, mesh.uv_divisor);

        glBindVertexArray(mesh.vao);
        for (index_buffer idx_buf : mesh.idx_buffers) {
            if (!idx_buf.enabled) {
                continue; // This index buffer is hidden
            }

            mat4s obj_pvm = glms_mul(*(mat4s*)pvm, idx_buf.get_transform(*alr));
            glUniformMatrix4fv(uniform_pvm, 1, GL_FALSE, (float*)obj_pvm.raw);

            // We cast away const here but don't write to the buffer
            vfile vf = vfile_open(alr->data, alr->alr_size);
            vf.pos = idx_buf.idx_chunk_offset;
            vfile_seek(&vf, sizeof(chunk_generic));
            const auto header = VFILE_READ(idxbuf_header, &vf);
            const auto mat_chunk = alr->prev_chunk_by_id(0x1, idx_buf.idx_chunk_offset);
            chunk_0x1_entry tex_entry = {};
            alr->tex_manager.get_material(*alr, mat_chunk.offset, header.texture_idx, &tex_entry);

            glActiveTexture(GL_TEXTURE0);
            glBindTexture(GL_TEXTURE_2D, alr->tex_manager.get(*alr, tex_entry.texture_idx));
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
            glBindTexture(GL_TEXTURE_2D, alr->tex_manager.get(*alr, normal_idx));
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

    glUseProgram(0);
    fbo.unbind(); // Reset state
}
