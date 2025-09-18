// Need this define to use operators on ImGui vector types
#define IMGUI_DEFINE_MATH_OPERATORS
#include <imgui.h>
#include <imgui_internal.h>
#include "imgui_utils.hxx"
#include <nfd.h> // Cross-platform native file dialog
#include <cglm/struct.h>

#include <common/int.h>

#include "polaris.hxx"
#include "scope_timer.hxx"

void polaris::handle_input_suppression() noexcept {
    if (ImGui::GetIO().WantCaptureMouse) {
        // ImGui wants control of the mouse (it's probably over a window),
        // so we'll suppress the real mouse state this frame.
        input.cursor = prev_input.cursor;
        input.scroll = prev_input.scroll;
        input.click_left = prev_input.click_left;
        input.click_right = prev_input.click_right;
        input.click_middle = prev_input.click_middle;
        input.mouse_button_4 = prev_input.mouse_button_4;
        input.mouse_button_5 = prev_input.mouse_button_5;
    }

    if (ImGui::GetIO().WantCaptureKeyboard) {
        // Save non-keyboard input
        const vec2s cursor = input.cursor;
        const vec2s scroll = input.scroll;
        const bool click_left = input.click_left;
        const bool click_right = input.click_right;
        const bool click_middle = input.click_middle;
        const bool mouse_4 = input.mouse_button_4;
        const bool mouse_5 = input.mouse_button_5;

        const vec2s LS = input.LS;
        const vec2s RS = input.RS;
        const float LT = input.LT;
        const float RT = input.RT;
        const gamepad_t gp = input.gp;

        // Copy over all keyboard input
        input = prev_input;

        // Restore non-keyboard input
        input.cursor = cursor;
        input.scroll = scroll;
        input.LS = LS;
        input.RS = RS;
        input.LT = LT;
        input.RT = RT;
        input.gp = gp;
        input.click_left = click_left;
        input.click_right = click_right;
        input.click_middle = click_middle;
        input.mouse_button_4 = mouse_4;
        input.mouse_button_5 = mouse_5;
    }
}

void polaris::do_menu_bar() noexcept {
    ImGuiViewport* viewport = ImGui::GetMainViewport();
    const float height = ImGui::GetFrameHeight();
    const ImGuiWindowFlags flags = ImGuiWindowFlags_NoScrollbar | ImGuiWindowFlags_MenuBar;

    const bool ctrl_pressed = ImGui::IsKeyDown(ImGuiKey_LeftCtrl) || ImGui::IsKeyDown(ImGuiKey_RightCtrl);
    bool load_alr = ctrl_pressed && ImGui::IsKeyPressed(ImGuiKey_L, false);
    bool save_alr = ctrl_pressed && ImGui::IsKeyPressed(ImGuiKey_S, false);

    bool load_layout = false;

    if (ImGui::BeginViewportSideBar("MainMenu", viewport, ImGuiDir_Up, height, flags)) {
        if (ImGui::BeginMenuBar()) {
            if (ImGui::BeginMenu("File")) {
                load_alr |= ImGui::MenuItem("Load ALR", "Ctrl-L");
                save_alr |= ImGui::MenuItem("Save ALR", "Ctrl-S");
                load_layout |= ImGui::MenuItem("Load .dat");
                ImGui::EndMenu();
            }

            if (ImGui::BeginMenu("View")) {
                ImGuiIO& io = ImGui::GetIO();
                ImGui::InputFloat("Font Size", &io.FontGlobalScale, 0.1f);
                ImGui::EndMenu();
            }

            if (ImGui::BeginMenu("Windows")) {
                ImGui::MenuItem("Viewport", nullptr, &this->viewport.enabled);
                ImGui::MenuItem("Viewport Editor", nullptr, &this->viewport.editor_enabled);
                ImGui::MenuItem("Performance Timers", nullptr, &this->show_timers);
                ImGui::MenuItem("ImGui Demo Window", nullptr, &this->show_demo);
                ImGui::EndMenu();
            }

            ImGui::EndMenuBar();
        }
        ImGui::End();
    }

    if (load_alr) {
        // Display the file picker and load the ALR if a file is picked
        char* path = nullptr;
        const nfdu8filteritem_t filters[] = { { "AL Resource", "alr"} };
        nfdresult_t result = NFD_OpenDialogU8(&path, filters, ARRAY_SIZE(filters), nullptr);
        if (result == NFD_OKAY && path != nullptr) {
            this->alr.load(path);
        }
        free(path);
    }

    if (save_alr) {
        // Display the file picker and save the ALR if a file is picked
        nfdu8filteritem_t filters[] = { { "AL Resource", "alr"} };
        char* path = nullptr;
        nfdresult_t result = NFD_SaveDialogU8(&path, filters, ARRAY_SIZE(filters), nullptr, nullptr);
        if (result == NFD_OKAY && path != nullptr) {
            this->alr.save(path);
        }
        free(path);
    }

    if (load_layout) {
        // Display the file picker and load if a file is picked
        nfdu8filteritem_t filters[] = { { "AL Layout", "dat"} };
        char* path = nullptr;
        nfdresult_t result = NFD_OpenDialogU8(&path, filters, ARRAY_SIZE(filters), nullptr);
        if (result == NFD_OKAY && path != nullptr) {
            this->map = mapdata(path);
        }
        free(path);
    }
}

void polaris::do_gui(GLFWwindow* window) noexcept {
    const scope_timer draw_timer(timer_map, "main_draw");

    // Make the entire window a giant docking space
    ImGui::DockSpaceOverViewport();

    // We have to wait until we know the graphics context has been created to do
    // graphics-related initialization (since the program may run in headless
    // mode with no graphics context).
    if (!viewport.initialized) {
        // Have the viewport render in full resolution, it'll be downscale when
        // rendered as a texture by ImGui::Image
        int width = 0;
        int height = 0;
        glfwGetFramebufferSize(window, &width, &height);
        viewport.setup(width, height);
    } else {
        // TODO: Is there a good way to get a const& to ourselves?
        if (!viewport.render_contents(window, alr)) {
            // We don't want to supress input if the viewport needs it
            this->handle_input_suppression();
        }
    }

    this->do_menu_bar();

    if (this->show_demo) {
        ImGui::ShowDemoWindow(&this->show_demo);
    }

    if (this->show_timers) {
        ImGui::Begin("Performance Timers", &show_timers);
        for (std::pair<const char*, double> entry : timer_map) {
            ImGui::Text("%s: %.2lfms", entry.first, entry.second * 1000);
        }
        ImGui::End();
    }

    this->alr.draw(viewport);
    this->map.do_gui();

    // It's the end of the frame for us, save the current input
    prev_input = input;
}
