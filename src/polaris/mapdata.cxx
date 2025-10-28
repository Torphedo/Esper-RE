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

void edit_ps01_entry(u32 idx, ps01_entry* entry) {
    ImGui::ScopedIndent indent(ImGui::CharWidth(2));
    std::string label;
    str_format_append(label, "Object ID##%d", idx);

    ImGui::SetNextItemWidth(ImGui::CharWidth(10));
    ImGui::InputU32(label.c_str(), &entry->object_id);

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
            edit_ps01_entry(i, &ps00_entries[i]);
            ImGui::Spacing();
            offset += sizeof(*ps00_entries);
        }
        ImGui::EndTabItem();
    }

    if (ImGui::BeginTabItem("PS01")) {
        u32 offset = header->ps01_offset;
        for (u32 i = 0; i < header->ps01_count; i++) {
            ImGui::Text("@ 0x%X: ", offset);
            edit_ps01_entry(i, &ps01_entries[i]);
            ImGui::Spacing();
            offset += sizeof(*ps01_entries);
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
