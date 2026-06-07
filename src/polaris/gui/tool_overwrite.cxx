#include "tool_overwrite.hxx"
#include <imgui.h>
#include "util/imgui_utils.hxx"

void overwrite_tool::advanced_submenu(bool showSourceStep) noexcept {
    if (ImGui::CollapsingHeader("Advanced")) {
        ImGui::InputU32("Repeat count", &operation_count);
        ImGui::InputU32("Target step (bytes)", &target_step);
        if (showSourceStep) {
            ImGui::InputU32("Source step (bytes)", &source_step);
        }
    }
}

void overwrite_tool::do_gui(void* data) noexcept {
    if (!enabled) {
        return;
    }

    u8* alr_data = (u8*)data;

    ImGui::Begin("Mass Overwrite Tool", &enabled);
    if (ImGui::BeginTabBar("modes")) {
        if (ImGui::BeginTabItem("memcopy")) {
            ImGui::InputU32("Source offset", &source_offset);
            ImGui::InputU32("Target offset", &target_offset);
            ImGui::InputU32("Copy size", &copy_size);
            advanced_submenu(true);

            if (ImGui::Button("Go!")) {
                u32 target_pos = target_offset;
                u32 source_pos = source_offset;
                for (u32 i = 0; i < operation_count; i++) {
                    memcpy(alr_data + target_pos, alr_data + source_pos, copy_size);
                    target_pos += copy_size + target_step;
                    source_pos += copy_size + source_step;
                }
            }

            ImGui::EndTabItem();
        }
        if (ImGui::BeginTabItem("memset")) {
            ImGui::InputU32("Target offset", &target_offset);
            ImGui::InputU32("Set size", &set_size);
            ImGui::InputU8("Set value", &value);
            advanced_submenu(false);

            if (ImGui::Button("Go!")) {
                for (u32 i = 0, pos = target_offset; i < operation_count; i++, pos += set_size + target_step) {
                    memset(alr_data + pos, value, set_size);
                }
            }

            ImGui::EndTabItem();
        }
        ImGui::EndTabBar();
    }

    ImGui::End();
}
