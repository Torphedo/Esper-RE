#include "tool_quest.hxx"
#include <nfd.h>

#include <formats/questdata.h>
#include <util/imgui_utils.hxx>
#include <util/utils.hxx>

static const nfdu8filteritem_t quest_filter[] = { { "Phantom Dust Quest File", "qdt"} };

#define INPUT_UNK_FIELD(obj, idx, type) ImGui::Input##type("Unknown " #idx, &obj.unk##idx);

const char* get_reward_name(s16 t) {
    switch (t) {
        case PD_REWARD_NONE: return "NONE";
        case PD_REWARD_RANDOM_SKILL: return "RANDOM SKILL";
        case PD_REWARD_SPECIFIC_SKILL: return "SPECIFIC SKILL";
        case PD_REWARD_ARSENAL1:
        case PD_REWARD_ARSENAL2:
            return "ARSENAL CASE";
        case PD_REWARD_CREDITS: return "CREDITS";
        case PD_REWARD_SKILL_RARITY: return "SKILL BY RARITY";
    }
    return "[INVALID TYPE]";
}

const pd_reward_type reward_types[] = {
    PD_REWARD_NONE,
    PD_REWARD_RANDOM_SKILL,
    PD_REWARD_SPECIFIC_SKILL,
    PD_REWARD_ARSENAL1,
    PD_REWARD_ARSENAL2,
    PD_REWARD_CREDITS,
    PD_REWARD_SKILL_RARITY,
};

bool reward_has_value(s16 t) {
    return t == PD_REWARD_ARSENAL1 || t == PD_REWARD_ARSENAL2 || t == PD_REWARD_SPECIFIC_SKILL;
}

void edit_quest_entry(quest_entry& quest) {
    const int word_wrap = ImGuiInputTextFlags_WordWrap;

    ImGui::InputText("Name", quest.name, sizeof(quest.name));
    ImGui::InputTextMultiline("Description", quest.desc, sizeof(quest.desc), ImVec2(), word_wrap);
    ImGui::InputText("Objective", quest.main_objective, sizeof(quest.main_objective));
    for (int i = 0; i < ARRAY_SIZE(quest.shown_objectives); i++) {
        if (quest.shown_objectives[i]) {
            std::string label;
            str_format_append(label, "Bonus objective %d", i + 1);
            ImGui::InputText(label.c_str(), quest.bonus_objectives[i], sizeof(quest.bonus_objectives[i]));
        }
    }

    ImGui::InputText("Unknown Text", quest.unk_text, sizeof(quest.unk_text));

    ImGui::ScopedWidth scopeWidth(20);
    ImGui::InputS16("Quest ID", &quest.id);
    ImGui::InputU16("# Players", &quest.num_players);
    ImGui::InputU16("# Enemies", &quest.num_enemies);
    ImGui::InputU16("# Player Slots", &quest.player_slots);
    ImGui::InputU16("Mission Photo", &quest.mission_photo);
    ImGui::InputU16("Stage #", &quest.stage);
    ImGui::InputU16("Stage Variant", &quest.stage_variant);

    if (ImGui::BeginTabBar("deckTabs")) {
        for (int i = 0; i < ARRAY_SIZE(quest.entries); i++) {
            player_entry& e = quest.entries[i];
            if (strlen(e.name) == 0) {
                break;
            }

            std::string label;
            str_format_append(label, "%s ##%d", e.name, i);
            if (ImGui::BeginTabItem(label.c_str())) {
                ImGui::InputU16("Character ID", &e.character);
                ImGui::InputU16("Costume ID", &e.costume);
                ImGui::InputText("Deck Name", e.name, sizeof(e.name));
                ImGui::InputText("AI Name", e.aiprog, sizeof(e.aiprog));
                ImGui::InputU16("Team", &e.team);
                ImGui::InputU16("Start Point Index", &e.start_idx);
                ImGui::InputU16("AI Param Index", &e.ai_param_idx);
                ImGui::InputU16("Health", &e.health);
                ImGui::InputU16("Unknown 5", &e.unk5);
                ImGui::EndTabItem();
            }
        }
        ImGui::EndTabBar();
    }

    if (ImGui::CollapsingHeader("Rewards")) {
        for (int i = 0; i < ARRAY_SIZE(quest.rewards); i++) {
            pd_reward &reward = quest.rewards[i];

            std::string label;
            str_format_append(label, "Reward %d", i);
            if (ImGui::BeginCombo(label.c_str(), get_reward_name(reward.type))) {
                for (const auto t: reward_types) {
                    if (ImGui::Selectable(get_reward_name(t))) {
                        reward.type = t;
                    }
                }
                ImGui::EndCombo();
            }

            if (reward_has_value(reward.type)) {
                label = "";
                str_format_append(label, "Reward value %d", i);
                ImGui::InputS16(label.c_str(), &reward.value);
            }
        }
    }

    if (ImGui::CollapsingHeader("Unknown Fields")) {
        INPUT_UNK_FIELD(quest, 1, U16);
        INPUT_UNK_FIELD(quest, 3, U16);
        INPUT_UNK_FIELD(quest, 4, U16);
        INPUT_UNK_FIELD(quest, 5, U16);
        INPUT_UNK_FIELD(quest, 6, U16);
        INPUT_UNK_FIELD(quest, 7, U16);
        INPUT_UNK_FIELD(quest, 8, U16);
        INPUT_UNK_FIELD(quest, 9, U16);
    }

    if (ImGui::CollapsingHeader("Partners")) {
        const char* names[8] = {"Meister", "Chunky", "Cuff Button", "pH", "Edgar", "Know", "Tsubutaki", "Sammah"};

        for (u32 i = 0; i < ARRAY_SIZE(names); i++) {
            bool val = quest.header.flags_byte & (u8(1) << i);
            if (ImGui::Checkbox(names[i], &val)) {
                const u8 mask = ~(u8(1 << i));
                const u8 byte = u8(val) << i;
                quest.header.flags_byte &= mask; // Clear the bit
                quest.header.flags_byte |= byte;  // Set the bit if needed
            }
        }
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

    if (ImGui::BeginTabBar("Quest Entry Tabs")) {
        quest_entry& entry = entries[selected_quest];
        if (ImGui::BeginTabItem("Custom Editor")) {
            edit_quest_entry(entry);
            ImGui::EndTabItem();
        }
        if (ImGui::BeginTabItem("Hex Editor")) {
            hex_edit.DrawContents(&entry, sizeof(entry), selected_quest * sizeof(entry));
            ImGui::EndTabItem();
        }
        ImGui::EndTabBar();
    }
    ImGui::EndChild();

    ImGui::End();
}
