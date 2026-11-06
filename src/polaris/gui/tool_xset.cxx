#include "tool_xset.hxx"
#include <cstring>
#include <common/path.h>
#include <nfd.h>

#include <formats/charaset.h>
#include <formats/stageset.h>
#include <util/imgui_utils.hxx>
#include <util/utils.hxx>

static const nfdu8filteritem_t quest_filter[] = { { "Phantom Dust Character/Stage Set File", "dat"} };

#define INPUT_UNK_FIELD(obj, idx, type) ImGui::Input##type("Unknown " #idx, &obj.unk##idx);
#define SAFE_TEXT(buf, offset, max, fallback) ((offset) >= (max) ? (fallback) : ((buf) + (offset)))

void edit_charaset_entry(charaset_entry& entry, char* textbuf, u32 bufsize) {
    char fallback[] = "[invalid offset]";
    char* fname = SAFE_TEXT(textbuf, entry.filename_offset, bufsize, fallback);
    char* name = SAFE_TEXT(textbuf, entry.name_offset, bufsize, fallback);
    char* name2 = SAFE_TEXT(textbuf, entry.name_offset2, bufsize, fallback);
    char* sfx_fname = SAFE_TEXT(textbuf, entry.sfx_filename_offset, bufsize, fallback);

    ImGui::InputText("Filename", fname, strlen(fname));
    ImGui::InputText("Name", name, strlen(name));
    ImGui::InputText("Name 2", name2, strlen(name2));
    ImGui::InputText("SFX Filename", sfx_fname, strlen(sfx_fname));

    ImGui::ScopedWidth scopeWidth(16);
    ImGui::InputU16("Jump Skill ID", &entry.jump_skill_id);

    if (ImGui::CollapsingHeader("Unknown Fields")) {
        INPUT_UNK_FIELD(entry, 1, U16);
        INPUT_UNK_FIELD(entry, 2, U16);
        INPUT_UNK_FIELD(entry, 3, U16);
        INPUT_UNK_FIELD(entry, 4, U16);
        INPUT_UNK_FIELD(entry, 5, U16);
        INPUT_UNK_FIELD(entry, 6, U16);

        // unk6 is an array
        ImGui::InputScalarN("Unknown 7", ImGuiDataType_U16, entry.unk7, ARRAY_SIZE(entry.unk7));

        INPUT_UNK_FIELD(entry, 8, U32);
        INPUT_UNK_FIELD(entry, 9, U16);
        INPUT_UNK_FIELD(entry, 10, U16);

        // unk10, 10, and 11 are arrays
        ImGui::InputScalarN("Unknown 11", ImGuiDataType_U16, entry.unk11, ARRAY_SIZE(entry.unk11));
        ImGui::InputScalarN("Unknown 12", ImGuiDataType_U16, entry.unk12, ARRAY_SIZE(entry.unk12));
        ImGui::InputScalarN("Unknown 13", ImGuiDataType_U16, entry.unk13, ARRAY_SIZE(entry.unk13));
    }
}

void edit_stageset_entry(stageset_entry& entry, char* textbuf, u32 bufsize) {
    char* name = textbuf + entry.name_offset;
    char* short_name = textbuf + entry.short_name_offset;
    char* internal_name = textbuf + entry.internal_name_offset;
    ImGui::InputText("Name", name, strlen(name));
    ImGui::InputText("Short Name", short_name, strlen(short_name));
    ImGui::InputText("Internal Name", internal_name, strlen(internal_name));

    ImGui::ScopedWidth scopeWidth(16);
    ImGui::InputU16("Stage ID", &entry.stage_id);
    ImGui::InputU16("Variant ID", &entry.variant_id);

    bool visible = entry.visibility;
    if (ImGui::Checkbox("Visible In-Game", &visible)) {
        entry.visibility = visible;
    }
}

void xset_tool::do_gui() noexcept {
    if (!enabled) {
        return;
    }

    ImGui::Begin("Character/Stage Set Editor", &enabled);
    if (data) {
        if (ImGui::Button("Unload file")) {
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
        ImGui::Text("No file loaded.");

        if (ImGui::Button("Load File")) {
            char* path = nullptr;
            nfdresult_t result_in = NFD_OpenDialogU8(&path, quest_filter, ARRAY_SIZE(quest_filter), nullptr);
            if (result_in == NFD_OKAY && path) {
                this->load(path);
                char fnameBuf[4096] = {0};
                path_get_filename(path, fnameBuf);
                if (strncmp(fnameBuf, "chara", 5) == 0) {
                    is_charaset = true;
                }
            }
            free(path);
        }
    }

    if (!data) {
        ImGui::End();
        return;
    }

    // We only reach here if the file is loaded
    auto* charaHeader = (charaset_header *)data;
    auto* stageHeader = (stageset_header *)data;

    const ImGuiChildFlags flags = ImGuiChildFlags_AutoResizeX;
    ImGui::BeginChild("entry_select", ImVec2(), flags);
    for (auto i = 0; i < charaHeader->entry_count; i++) {
        std::string label;
        str_format_append(label, "%s##%d", name_for_entry(i), i);
        if (ImGui::Selectable(label.c_str())) {
            selected_entry = i;
        }
    }
    ImGui::EndChild();

    ImGui::SameLine();
    ImGui::BeginChild("entry_edit");

    if (ImGui::BeginTabBar("Entry Tabs")) {
        if (ImGui::BeginTabItem("Custom Editor")) {
            char* textbuf = get_textbuf();
            const u32 bufsize = size - charaHeader->string_pool_offset;
            if (is_charaset) {
                edit_charaset_entry(charaHeader->entries[selected_entry], textbuf, bufsize);
            } else {
                edit_stageset_entry(stageHeader->entries[selected_entry], textbuf, bufsize);
            }
            ImGui::EndTabItem();
        }
        if (ImGui::BeginTabItem("Hex Editor")) {
            const u32 size = is_charaset ? sizeof(*charaHeader->entries) : sizeof(*stageHeader->entries);
            void* entry = nullptr;
            if (is_charaset) {
                entry = &charaHeader->entries[selected_entry];
            } else {
                entry = &stageHeader->entries[selected_entry];
            }

            hex_edit.DrawContents(entry, size, selected_entry * size);
            ImGui::EndTabItem();
        }
        ImGui::EndTabBar();
    }
    ImGui::EndChild();

    ImGui::End();
}

char* xset_tool::get_textbuf() noexcept {
    // Header is actually the same in both formats.
    static_assert(offsetof(charaset_header, string_pool_offset) == offsetof(stageset_header, string_pool_offset), "");
    static_assert(sizeof(charaset_header) == sizeof(stageset_header), "");
    const charaset_header* header = (charaset_header*)data;

    return (char*)(data + header->string_pool_offset);
}

const char* xset_tool::name_for_entry(u32 idx) noexcept {
    const auto* charaHeader = (charaset_header *)data;
    const auto* stageHeader = (stageset_header *)data;
    const char* textbuf = get_textbuf();
    if (is_charaset) {
        return textbuf + charaHeader->entries[idx].name_offset;
    } else {
        return textbuf + stageHeader->entries[idx].name_offset;
    }
}
