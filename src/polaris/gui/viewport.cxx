#define IMGUI_DEFINE_MATH_OPERATORS
#include "viewport.hxx"
#include <imgui.h>

#include <common/vfile.h>
#include <common/logging.h>

#include "alr_opengl.hxx"
#include "polaris.hxx"
#include "selector_ray.hxx"
#include <util/scope_timer.hxx>
#include <util/imgui_utils.hxx>

extern "C" {
    #include <common/gl/shader.h>
    #include <common/gl/input.h>
}

// GLSL shaders
#include "generic.vert.h"
#include "diffuse.frag.h"

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

    glEnable(GL_BLEND);
    shader = program_compile_src(generic_vert, diffuse_frag);
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
    uniform_sampler_lightmap = glGetUniformLocation(shader, "lightmap_texture");

    if (wireframe) {
        glPolygonMode(GL_FRONT_AND_BACK, GL_LINE);
    }
    fbo.unbind();
}

void viewport_t::destroy() noexcept {
    if (initialized) {
        fbo.destroy();
        glDeleteProgram(shader);
        for (alr::mesh& mesh : meshes) {
            mesh.destroy();
        }

        initialized = false;
    }
}

void viewport_t::update(GLFWwindow* window) noexcept {
    const scope_timer draw_timer("viewportUpdate");
    if (!active || !initialized) {
        return;
    }
    // Calculate delta time every time we render
    static double prev_time = glfwGetTime();
    const double cur_time = glfwGetTime();
    const double delta_time = cur_time - prev_time;
    prev_time = cur_time;
    anim_frame += (delta_time / FRAMETIME_24FPS);

    if (editor_enabled) {
        ImGui::Begin("Render Settings", &this->editor_enabled);
        ImGui::SetNextItemWidth(ImGui::CharWidth(20));
        ImGui::InputU16("Selected Model", &selected_mesh);

        fbo.bind();
        if (ImGui::Checkbox("Wireframe", &wireframe)) {
            fbo.set_wireframe(wireframe);
        }

        if (ImGui::Checkbox("Back-face culling", &backface_cull)) {
            fbo.set_backface_cull(backface_cull);
        }
        fbo.unbind();

        bool temp_render_texcoords = shader_flags.render_texcoords;
        ImGui::Checkbox("Visualize UVs", &temp_render_texcoords);
        shader_flags.render_texcoords = temp_render_texcoords;

        bool temp_render_normals = shader_flags.render_normals;
        ImGui::Checkbox("Visualize normals", &temp_render_normals);
        shader_flags.render_normals = temp_render_normals;

        bool temp_force_disable_normals = shader_flags.has_normal;
        ImGui::Checkbox("Use normal maps", &temp_force_disable_normals);
        shader_flags.has_normal = temp_force_disable_normals;

        ImGui::Checkbox("Enable raycast test", &raycast_test);
        ImGui::Checkbox("Render selection in wireframe", &wireframe_selection);


        if (ImGui::CollapsingHeader("Model properties")) {
            selected_mesh = CLAMP(0, selected_mesh, meshes.size());

            if (selected_mesh < meshes.size()) {
                // If you make this loop over all meshes in the future, make sure not to
                // use the for loop style with a colon (or make sure you get a reference),
                // otherwise it'll run the menu on a copy and not modify the data
                alr::mesh& mesh = meshes.at(selected_mesh);
                // TODO: Restore edit menu
                // mesh.edit_menu(*alr);
            }

        }
        ImGui::End();
    }

    visible = ImGui::Begin("Viewport");
    const float padding = ImGui::GetStyle().FramePadding.x * 2;

    // Need this ridiculous workaround to make sure options don't take up
    // like half the horizontal screen space
    const char *options[] = {"Orbit", "Minecraft", "Fly"};
    const char *label = "Camera Mode";
    const float combo_width = ImGui::CalcTextSize(options[1]).x * 1.5f + padding;
    ImGui::SetNextItemWidth(combo_width);

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
    ImGui::SetNextItemWidth(ImGui::CharWidth(16) + padding);
    ImGui::InputU32("Animation ID", &anim_id);
    anim_id %= ALR_NUM_PLAYER_ANIMATIONS;
    ImGui::SameLine();
    ImGui::Text(" = %s", animation_names[anim_id]);

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

        } else {
            cam.update(delta_time);
        }
    }

    ImGui::End();
}

void viewport_t::render(GLFWwindow* window) noexcept {
    const scope_timer draw_timer("viewportRender");
    if (!active || !initialized || !visible) {
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
    glUniform1i(uniform_sampler_lightmap, 2);

    // Render all opaque meshes
    for (u32 i = 0; i < meshes.size(); i++) {
        const alr::mesh& mesh = meshes[i];
        const bool do_wireframe = wireframe || (wireframe_selection && (i == selected_mesh));
        fbo.set_wireframe(do_wireframe);
        mesh.render(*alr, anim_id, anim_frame, *(mat4s*)pvm, uniform_pvm, uniform_uv_divisor);
    }

    // Render semi-transparent meshes

    glUseProgram(0);
    fbo.unbind(); // Reset state
}
