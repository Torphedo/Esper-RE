// Need this define to use operators on ImGui vector types
#define IMGUI_DEFINE_MATH_OPERATORS
#include <imgui.h>
#include <imgui_internal.h>
#include "imgui_utils.hxx"
#include <nfd.h> // Cross-platform native file dialog

#include <common/int.h>

#include "polaris.hxx"
#include "scope_timer.hxx"
#include "mkak.hxx"

void polaris::do_menu_bar() noexcept {
    ImGuiViewport* viewport = ImGui::GetMainViewport();
    const float height = ImGui::GetFrameHeight();
    const ImGuiWindowFlags flags = ImGuiWindowFlags_NoScrollbar | ImGuiWindowFlags_MenuBar;

    const bool ctrl_pressed = ImGui::IsKeyDown(ImGuiKey_LeftCtrl) || ImGui::IsKeyDown(ImGuiKey_RightCtrl);
    bool load_alr = ctrl_pressed && ImGui::IsKeyPressed(ImGuiKey_L, false);
    bool save_alr = ctrl_pressed && ImGui::IsKeyPressed(ImGuiKey_S, false);

    bool load_layout = false;
    bool save_layout = false;
    bool extract_mkak = false;

    if (ImGui::BeginViewportSideBar("MainMenu", viewport, ImGuiDir_Up, height, flags)) {
        if (ImGui::BeginMenuBar()) {
            if (ImGui::BeginMenu("File")) {
                load_alr |= ImGui::MenuItem("Load ALR", "Ctrl-L");
                save_alr |= ImGui::MenuItem("Save ALR", "Ctrl-S");
                load_layout |= ImGui::MenuItem("Load .dat");
                save_layout |= ImGui::MenuItem("Save .dat");
                extract_mkak |= ImGui::MenuItem("Extract .mk / .ak");
                ImGui::EndMenu();
            }

            if (ImGui::BeginMenu("View")) {
                ImGuiIO& io = ImGui::GetIO();
                ImGui::InputFloat("Font Size", &io.FontGlobalScale, 0.1f);
                ImGui::EndMenu();
            }

            if (ImGui::BeginMenu("Windows")) {
                ImGui::MenuItem("Viewport", nullptr, &this->viewport.active);
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

    if (save_layout) {
        // Display the file picker and load if a file is picked
        nfdu8filteritem_t filters[] = { { "AL Layout", "dat"} };
        char* path = nullptr;
        nfdresult_t result = NFD_SaveDialogU8(&path, filters, ARRAY_SIZE(filters), nullptr, nullptr);
        if (result == NFD_OKAY && path != nullptr) {
            map.save(path);
        }
        free(path);
    }

    if (extract_mkak) {
        nfdu8filteritem_t filters[] = { { "Phantom Dust MK archive", "mk"}, { "Phantom Dust AK archive", "ak"} };
        char* path = nullptr;
        nfdresult_t result_in = NFD_OpenDialogU8(&path, filters, ARRAY_SIZE(filters), nullptr);
        char* out_dir = nullptr;
        nfdresult_t result_out = NFD_PickFolderU8(&out_dir, nullptr);
        if (result_in == NFD_OKAY && result_out == NFD_OKAY && path && out_dir) {
            mkak::dump_to_folder(path, out_dir);
        }
        free(path);
        free(out_dir);
    }
}

void polaris::init(GLFWwindow* window) noexcept {
    NFD_Init();
    viewport.init(window);
    viewport.alr = &alr;
}

void polaris::update(GLFWwindow* window) noexcept {
    const scope_timer draw_timer(timer_map, "main_draw");

    viewport.update(window);
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
}

void polaris::render(GLFWwindow* window) noexcept {
    viewport.render(window);
}

void polaris::destroy() noexcept {
    viewport.destroy();
}
