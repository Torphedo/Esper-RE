// Need this define to use operators on ImGui vector types
#define IMGUI_DEFINE_MATH_OPERATORS
#include "polaris.hxx"
#include <imgui_internal.h>
#include "util/imgui_utils.hxx"
#include "util/nfde_wrapper.hxx"

#include <common/int.h>
#include <formats/miniaudio_stx.h>

#include "alr/mkak.hxx"
#include "util/scope_timer.hxx"
#include "version.h"

bool extract_mkak_menu() {
    nfdu8filteritem_t filters[] = { { "Phantom Dust MK archive", "mk"}, { "Phantom Dust AK archive", "ak"} };
    bool result = false;

    char* path = nullptr;
    nfdresult_t result_in = NFD_OpenDialogU8(&path, filters, ARRAY_SIZE(filters), nullptr);
    char* out_dir = nullptr;
    if (result_in == NFD_OKAY && path) {
        nfdresult_t result_out = NFD_PickFolderU8(&out_dir, nullptr);
        if (result_out == NFD_OKAY && out_dir) {
            result = mkak::dump_to_folder(path, out_dir);
        }
    }
    free(path);
    free(out_dir);

    return result;
}

bool create_mkak_menu() {
    const nfdu8filteritem_t filters[] = { { "Phantom Dust MK archive", "mk"}, { "Phantom Dust AK archive", "ak"} };
    bool result = false;

    std::vector<std::string> input_paths;
    nfdresult_t result_in = NFD_OpenDialogMultipleAutoFree(input_paths, nullptr, 0, nullptr);
    if (result_in == NFD_OKAY) {
        char* outpath = nullptr;
        nfdresult_t result_out = NFD_SaveDialogU8(&outpath, filters, ARRAY_SIZE(filters), nullptr, nullptr);

        if (result_out == NFD_OKAY && outpath) {
            result = mkak::create_pack(input_paths, outpath);
        }
        free(outpath);
    }

    return result;
}

bool create_stx_menu() {
    const nfdu8filteritem_t infilters[] = {
        { "Audio File", "wav,mp3,mod,xm,s3m"},
        {"Raw Audio", "wav"},
        {"MPEG-3", "mp3"},
        {"ProTracker Module", "mod"},
        {"FastTracker II Module", "xm"},
        {"ScreamTracker 3 Module", "s3m"},
    };
    const nfdu8filteritem_t outfilters[] = { { "Phantom Dust Music", "stx"} };
    bool result = false;

    // Prompt for an input and output file
    char* path = nullptr;
    nfdresult_t nfdRes = NFD_OpenDialogU8(&path, infilters, ARRAY_SIZE(infilters), nullptr);
    if (nfdRes == NFD_OKAY && path) {
        char* outpath = nullptr;
        nfdresult_t result_out = NFD_SaveDialogU8(&outpath, outfilters, ARRAY_SIZE(outfilters), nullptr, nullptr);
        if (result_out == NFD_OKAY && outpath) {
            result = ma_generate_stx(path, outpath);
        }
        free(outpath);
    }
    free(path);

    return result;
}

bool about_menu() {
    bool open = true;
    ImGui::Begin("About Polaris", &open);

    ImGui::TextCentered("Polaris v" POLARIS_VERSION "\n");
    ImGui::TextCentered("Open-source @ " POLARIS_URL "\n");
    ImGui::TextCentered("Written by Torphedo\n\n\n");
    ImGui::TextCentered("=== Special Thanks ===\n");
    for (const char* txt : polaris_special_thanks) {
        ImGui::Text("\t%s\n", txt);
    }
    ImGui::End();

    return open;
}

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
    bool create_mkak = false;
    bool create_stx = false;

    if (ImGui::BeginViewportSideBar("MainMenu", viewport, ImGuiDir_Up, height, flags)) {
        if (ImGui::BeginMenuBar()) {
            if (ImGui::BeginMenu("File")) {
                load_alr |= ImGui::MenuItem("Load ALR", "Ctrl-L");
                save_alr |= ImGui::MenuItem("Save ALR", "Ctrl-S");
                load_layout |= ImGui::MenuItem("Load .dat");
                save_layout |= ImGui::MenuItem("Save .dat");
                ImGui::EndMenu();
            }

            if (ImGui::BeginMenu("View")) {
                ImGuiIO& io = ImGui::GetIO();
                ImGui::InputFloat("Font Size", &io.FontGlobalScale, 0.1f);
                ImGui::EndMenu();
            }

            if (ImGui::BeginMenu("Windows")) {
                ImGui::MenuItem("Viewport", nullptr, &this->renderCtx.active);
                ImGui::MenuItem("Render Settings", nullptr, &this->renderCtx.editor_enabled);
                ImGui::MenuItem("Performance Timers", nullptr, &this->show_timers);
                ImGui::MenuItem("ImGui Demo Window", nullptr, &this->show_demo);
                ImGui::EndMenu();
            }

            if (ImGui::BeginMenu("Tools")) {
                ImGui::MenuItem("Audio Analyzer (.bin / STX)", nullptr, &audioTool.enabled);
                ImGui::MenuItem("Quest Editor (.qdt)", nullptr, &questTool.enabled);
                ImGui::MenuItem("Generate STX", nullptr, &create_stx);
                ImGui::MenuItem("Extract .mk / .ak", nullptr, &extract_mkak);
                ImGui::MenuItem("Create .mk / .ak", nullptr, &create_mkak);
                ImGui::MenuItem("Shader (.cso) Viewer", nullptr, &csoTool.enabled);
                ImGui::EndMenu();
            }

            if (ImGui::BeginMenu("Help")) {
                ImGui::MenuItem("About", nullptr, &show_about);

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
            // Wipe data that references the ALR before loading a new one
            editor.clear_meshes();
            editor.load(path);
        }
        free(path);
    }

    if (save_alr) {
        // Display the file picker and save the ALR if a file is picked
        nfdu8filteritem_t filters[] = { { "AL Resource", "alr"} };
        char* path = nullptr;
        nfdresult_t result = NFD_SaveDialogU8(&path, filters, ARRAY_SIZE(filters), nullptr, nullptr);
        if (result == NFD_OKAY && path != nullptr) {
            editor.alr.save(path);
        }
        free(path);
    }

    if (load_layout) {
        // Display the file picker and load if a file is picked
        nfdu8filteritem_t filters[] = { { "AL Layout", "dat"} };
        char* path = nullptr;
        nfdresult_t result = NFD_OpenDialogU8(&path, filters, ARRAY_SIZE(filters), nullptr);
        if (result == NFD_OKAY && path != nullptr) {
            this->mapEdit = mapdata_editor(path, *this);
        }
        free(path);
    }

    if (save_layout) {
        // Display the file picker and load if a file is picked
        nfdu8filteritem_t filters[] = { { "AL Layout", "dat"} };
        char* path = nullptr;
        nfdresult_t result = NFD_SaveDialogU8(&path, filters, ARRAY_SIZE(filters), nullptr, nullptr);
        if (result == NFD_OKAY && path != nullptr) {
            mapEdit.map.save(path);
        }
        free(path);
    }

    if (extract_mkak) {
        extract_mkak_menu();
    }
    if (create_mkak) {
        create_mkak_menu();
    }
    if (create_stx) {
        create_stx_menu();
    }
}

void polaris::init(GLFWwindow* window) noexcept {
    NFD_Init();
    renderCtx.init(window);
    editor.load_all_meshes();
}

void polaris::update(GLFWwindow* window) noexcept {
    const scope_timer draw_timer("mainUpdate");

    renderCtx.update(window);
    this->do_menu_bar();

    if (this->show_demo) {
        ImGui::ShowDemoWindow(&this->show_demo);
    }

    if (this->show_timers) {
        ImGui::Begin("Performance Timers", &show_timers);
        for (std::pair<const char*, double> entry : global_timers) {
            ImGui::Text("%s: %.2lfms", entry.first, entry.second * 1000);
        }
        global_timers.clear();
        ImGui::End();
    }

    if (show_about) {
        show_about = about_menu();
    }

    editor.update(renderCtx);
    this->mapEdit.do_gui();
    this->audioTool.do_gui();
    this->questTool.do_gui();
    this->csoTool.do_gui();
}

void polaris::render(GLFWwindow* window) noexcept {
    const scope_timer draw_timer("mainRender");
    editor.render(renderCtx);
}

void polaris::destroy() noexcept {
    renderCtx.destroy();
}
