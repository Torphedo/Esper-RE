#include "tool_quest.hxx"
#include <string>
#include <stdlib.h>

#include <nfd.h>

#include "util/imgui_utils.hxx"
#include "util/utils.hxx"

static const nfdu8filteritem_t quest_filter[] = { { "Phantom Dust Quest File", "qdt"} };

#define INPUT_UNK_FIELD(obj, idx, type) ImGui::Input##type("Unknown " #idx, &obj.unk##idx);

void edit_quest_entry(quest_entry& entry) {
    const int word_wrap = ImGuiInputTextFlags_WordWrap;

    ImGui::InputText("Name", entry.name, sizeof(entry.name));
    ImGui::InputTextMultiline("Description", entry.desc, sizeof(entry.desc), ImVec2(), word_wrap);
    ImGui::InputText("Objective", entry.objective, sizeof(entry.objective));
    ImGui::InputText("Unknown Text", entry.unk_text, sizeof(entry.unk_text));

    ImGui::ScopedWidth scopeWidth(16);
    ImGui::InputU16("Quest ID", &entry.id);
    ImGui::InputU16("# Players", &entry.num_players);
    ImGui::InputU16("# Enemies", &entry.num_enemies);
    ImGui::InputU16("# Player Slots", &entry.player_slots);
    ImGui::InputU16("Thumbnail ID", &entry.thumbnail_id);

    if (ImGui::CollapsingHeader("Unknown Fields")) {
        INPUT_UNK_FIELD(entry, 1, U16);
        INPUT_UNK_FIELD(entry, 2, U32);
        INPUT_UNK_FIELD(entry, 3, U16);
        INPUT_UNK_FIELD(entry, 4, U16);
        INPUT_UNK_FIELD(entry, 5, U16);
        INPUT_UNK_FIELD(entry, 6, U16);
    }
}

void quest_tool::do_gui() noexcept {
    if (!enabled) {
        return;
    }

    ImGui::Begin("Quest Editor", &enabled);
    if (data) {
        if (ImGui::Button("Unload quest file")) {
            this->unload();
        }
        ImGui::SameLine();
        if (ImGui::Button("Save")) {
            char* path = nullptr;
            nfdresult_t result_in = NFD_SaveDialogU8(&path, quest_filter, ARRAY_SIZE(quest_filter), nullptr, "questdata.qdt");
            if (result_in == NFD_OKAY && path) {
                this->save(path);
            }
            free(path);
        }
    } else {
        ImGui::Text("No quest file loaded.");

        if (ImGui::Button("Load Quest File")) {
            char* path = nullptr;
            nfdresult_t result_in = NFD_OpenDialogU8(&path, quest_filter, ARRAY_SIZE(quest_filter), nullptr);
            if (result_in == NFD_OKAY && path) {
                this->load(path);
            }
            free(path);
        }
    }

    if (!data) {
        ImGui::End();
        return;
    }

    // We only reach here if the file is loaded
    auto* entries = (quest_entry*)data;

    const ImGuiChildFlags flags = ImGuiChildFlags_AutoResizeX;
    ImGui::BeginChild("quest_select", ImVec2(), flags);
    for (auto i = 0; i < PD_MAX_QUESTS; i++) {
        quest_entry& entry = entries[i];

        std::string label;
        str_format_append(label, "%s##%d", entry.name, i);
        if (ImGui::Selectable(label.c_str())) {
            selected_quest = i;
        }
    }
    ImGui::EndChild();

    ImGui::SameLine();
    ImGui::BeginChild("quest_edit");
    edit_quest_entry(entries[selected_quest]);
    ImGui::EndChild();

    ImGui::End();
}
