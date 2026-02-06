#define IMGUI_DEFINE_MATH_OPERATORS
#include <imgui.h>

#include <common/gl/shader.h>
#include <common/gl/input.h>

#include <common/logging.h>
#include <formats/alr.h>

#include <util/scope_timer.hxx>
#include <util/imgui_utils.hxx>
#include "render_context.hxx"

// GLSL shaders
#include <shaders/generic.vert.h>
#include <shaders/diffuse.frag.h>
#include <shaders/show_uv.frag.h>

void render_context::init(GLFWwindow* window) noexcept {
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
    diffuse_shader = program_compile_src(generic_vert, diffuse_frag);
    if (!shader_link_check(diffuse_shader)) {
        LOG_MSG(error, "Failed to compile diffuse shader!\n");
        return;
    }
    uv_shader = program_compile_src(generic_vert, show_uv_frag);
    if (!shader_link_check(uv_shader)) {
        LOG_MSG(error, "Failed to compile UV shader!\n");
        return;
    }

    set_shader(diffuse_shader);

    if (wireframe) {
        glPolygonMode(GL_FRONT_AND_BACK, GL_LINE);
    }
    fbo.unbind();
}

void render_context::set_shader(gl_obj shader) noexcept {
    active_shader = shader;

    uniform_pvm = glGetUniformLocation(active_shader, "pvm");
    uniform_uv_divisor = glGetUniformLocation(active_shader, "uv_divisor");
    uniform_cam_dir = glGetUniformLocation(active_shader, "cam_dir");

    uniform_sampler_albedo = glGetUniformLocation(active_shader, "albedo_texture");
    uniform_sampler_normal = glGetUniformLocation(active_shader, "normal_texture");
    uniform_sampler_lightmap = glGetUniformLocation(active_shader, "lightmap_texture");

}

void render_context::destroy() noexcept {
    if (initialized) {
        fbo.destroy();
        glDeleteProgram(diffuse_shader);
        glDeleteProgram(uv_shader);
        initialized = false;
    }
}

void render_context::update(GLFWwindow* window) noexcept {
    const scope_timer draw_timer("viewportUpdate");
    if (!active || !initialized) {
        return;
    }
    const double delta_time = ImGui::GetIO().DeltaTime;
    anim_frame += (delta_time / FRAMETIME_24FPS);

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
        cam.update(delta_time);
    }

    ImGui::End();
}

void render_context::bind() noexcept {
    // Start rendering to the viewport
    fbo.bind();
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

    // Bind shader & upload camera transform
    glUseProgram(active_shader);
    glUniform3fv(uniform_cam_dir, 1, cam.facing().raw);

    glUniform1i(uniform_sampler_albedo, 0);
    glUniform1i(uniform_sampler_normal, 1);
    glUniform1i(uniform_sampler_lightmap, 2);
}

void render_context::unbind() noexcept {
    glUseProgram(0);
    fbo.unbind();
}
