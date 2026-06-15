#define IMGUI_DEFINE_MATH_OPERATORS
#include <imgui.h>

#include <common/gl/shader.h>
#include <common/gl/input.h>

#include <common/logging.h>
#include <formats/alr.h>

#include <util/scope_timer.hxx>
#include <util/imgui_utils.hxx>
#include "render_context.hxx"
#include "selector_ray.hxx"
#include "shaders/vkblink.frag.h"
#include "shaders/box.vert.hxx"
#include "shaders/solid.frag.h"

// GLSL shaders
#include <shaders/generic.vert.h>
#include <shaders/skinning.vert.h>
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

    vkblink_shader = program_compile_src(generic_vert, vkblink_frag);
    if (!shader_link_check(vkblink_shader)) {
        LOG_MSG(error, "Failed to compile diffuse shader!\n");
        return;
    }

    uv_shader = program_compile_src(generic_vert, show_uv_frag);
    if (!shader_link_check(uv_shader)) {
        LOG_MSG(error, "Failed to compile UV shader!\n");
        return;
    }

    skinned_shader = program_compile_src(skinning_vert, diffuse_frag);
    if (!shader_link_check(skinned_shader)) {
        LOG_MSG(error, "Failed to compile skinned shader!\n");
        return;
    }

    cube_shader = program_compile_src(box_vert, solid_frag);
    if (!shader_link_check(cube_shader)) {
        LOG_MSG(error, "Failed to compile cube shader!\n");
        return;
    }
    glGenVertexArrays(1, &blank_vao);


    set_shader(diffuse_shader);

    if (wireframe) {
        glPolygonMode(GL_FRONT_AND_BACK, GL_LINE);
    }
    fbo.unbind();
}

void render_context::set_shader(gl_obj shader, bool force) noexcept {
    if (active_shader == shader && !force) {
        return; // Don't set up multiple times if we don't need to
    }

    active_shader = shader;

    glUseProgram(active_shader);
    uniform_skin_xforms = glGetUniformLocation(active_shader, "skin_xforms");
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
    click_ray.reset();

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
    ivec2s screenSize = {};
    vec2s mouse_pos = {};

    ImVec2 image_size = ImVec2(fbo.width, fbo.height);
    const float fboToPresentedScale = ImGui::ImageScaleForWindow(fbo.width, fbo.height);
    image_size *= fboToPresentedScale;

    {
        double x, y;
        glfwGetCursorPos(window, &x, &y);
        mouse_pos = {float(x), float(y)};

        glfwGetFramebufferSize(window, &screenSize.x, &screenSize.y);
        const vec2s sizeDiff = {(float)screenSize.x - image_size.x, (float)screenSize.y - image_size.y };
        mouse_pos.y = screenSize.y - mouse_pos.y - 1;
        mouse_pos.y += sizeDiff.y;
    };

    ImGui::Image(fbo.color_tex, image_size, ImVec2(0, 1), ImVec2(1, 0));
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

        if (ImGui::IsKeyDown(ImGuiKey_LeftAlt)) {
            const vec4s fb_viewport = {
                    .x = fb_start.x, .y = fb_start.y,
                    .z = image_size.x, .w = image_size.y,
            };
            fbo.bind();
            click_ray = screen_to_ray(mouse_pos, cam, fb_viewport);
            fbo.unbind();
        } else {
            cam.update(delta_time);
        }
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
