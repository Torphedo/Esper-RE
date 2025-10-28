#include "mapdata.hxx"
#include <cstdlib>

#include <common/file.h>
#include <common/vfile.h>
#include <common/logging.h>

#include "formats/st00.h"
#include "imgui_utils.hxx"

bool mapdata::load_verify() const noexcept {
    const u32 magic = *(u32*)data;
    if (magic != st00_magic) {
        LOG_MSG(error, "\"%s\" doesn't seem to be a .dat map file (invalid magic 0x%x)\n", filepath, magic);
        return false;
    }
    return true;
}

mapdata::mapdata(const char* filepath) : filepath(filepath) {
    initialized = load(filepath);
}

const char* mapdata::name_at_idx(u32 idx) const noexcept {
    const auto* header = (st00_t*)data;
    if (idx >= header->nm00_count) {
        return "";
    }

    u32 i = 0;
    const char* txt = (const char*)(data + header->nm00_offset);
    while (idx > i) {
        txt += strlen(txt) + 1;
        i++;
    }

    return txt;
}

// Move assignment operator (when assigning with a temp value)
mapdata& mapdata::operator=(mapdata&& other) {
    if (this != &other) {
        free(data);
        memcpy(this, &other, sizeof(other)); // Copy state from temporary
        // Wipe temporary so it doesn't free our pointer on destroy
        memset(&other, 0, sizeof(other));
    }

    return *this;
}

mapdata::~mapdata() {
    initialized = false;
}

void mapdata::edit_ps01_entry(u32 idx, ps01_entry* entry, u32 max_id) noexcept {
    ImGui::ScopedIndent indent(ImGui::CharWidth(2));
    std::string label;
    str_format_append(label, "Object Type##%d", idx);

    std::string cur_name = name_at_idx(entry->object_id);
    if (cur_name.empty()) {
        str_format_append(cur_name, "missing name [ID 0x%X]", entry->object_id);
    }

    if (ImGui::BeginCombo(label.c_str(), cur_name.c_str())) {
        st00_t* header = get_header();
        for (s32 i = 0; i < header->nm00_count; i++) {
            if (ImGui::Selectable(name_at_idx(i))) {
                entry->object_id = i;
            }
        }
        ImGui::EndCombo();
    }

    // Don't allow invalid IDs, wrap around both ways
    if (entry->object_id < 0) {
        entry->object_id += max_id;
    }
    entry->object_id %= max_id;

    label = "";
    str_format_append(label, "Position##%d", idx);

    ImGui::SetNextItemWidth(ImGui::CharWidth(30));
    ImGui::InputFloat3(label.c_str(), &entry->pos.x);

    label = "";
    str_format_append(label, "Rotation##%d", idx);
    ImGui::SetNextItemWidth(ImGui::CharWidth(30));
    ImGui::InputFloat3(label.c_str(), &entry->rotation.x);
}

void mapdata::draw_custom_editor() {
    if (!ImGui::BeginTabBar("")) {
        return;
    }
    st00_t* header = get_header();
    auto* ps00_entries = (ps01_entry*)(data + header->chunk_size);
    auto* ps01_entries = (ps01_entry*)(data + header->ps01_offset);
    if (ImGui::BeginTabItem("PS0/")) {
        u32 offset = header->chunk_size;
        for (u32 i = 0; i < header->ps00_count; i++) {
            ImGui::Text("@ 0x%X: ", offset);
            edit_ps01_entry(i, &ps00_entries[i], header->nm00_count);
            ImGui::Spacing();
            offset += sizeof(*ps00_entries);
        }
        ImGui::EndTabItem();
    }

    if (ImGui::BeginTabItem("PS01")) {
        u32 offset = header->ps01_offset;
        for (u32 i = 0; i < header->ps01_count; i++) {
            ImGui::Text("@ 0x%X: ", offset);
            edit_ps01_entry(i, &ps01_entries[i], header->nm00_count);
            ImGui::Spacing();
            offset += sizeof(*ps01_entries);
        }
        ImGui::EndTabItem();
    }

    if (ImGui::BeginTabItem("NM01") && header->nm00_offset > 0) {
        char* txt = (char*)(data + header->nm00_offset);
        for (s32 i = 0; i < header->nm00_count; i++) {
            const s32 len = strlen(txt) + 1;
            std::string label = "Object " + std::to_string(i);
            ImGui::InputText(label.c_str(), txt, len);
            txt += len;
        }
        ImGui::EndTabItem();
    }

    ImGui::EndTabBar();
}

void mapdata::do_gui() noexcept {
    if (!this->initialized) {
        return; // Ignore if not initialized
    }

    ImGui::Begin(".dat Data");

    if (ImGui::BeginTabBar("Chunk Tabs")) {
        if (ImGui::BeginTabItem("Raw file data")) {
            hex_edit.DrawContents((void *) this->data, this->size);
            ImGui::EndTabItem();
        }
        if (ImGui::BeginTabItem("Chunks")) {
            draw_custom_editor();
            ImGui::EndTabItem();
        }

        ImGui::EndTabBar();
    }

    ImGui::End();
}
