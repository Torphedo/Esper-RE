#include "mapdata.hxx"
#include <cstdlib>

#include <common/file.h>
#include <common/logging.h>

#include "formats/st00.h"
#include "util/imgui_utils.hxx"
#include "gui/polaris.hxx"
#include "mesh_view.hxx"

bool mapdata::offset_is_reasonable(s32 offset) noexcept {
    if (offset <= 0 || offset > size) {
        return false;
    }
    return true;
}

bool mapdata::load_verify() const noexcept {
    if (!data) {
        return false;
    }
    const u32 magic = *(u32*)data;
    const bool is_st00 = (magic == st00_magic);
    const bool is_area = strncmp((char*)data, "AR0", 3) == 0;
    if (!is_st00 && !is_area) {
        LOG_MSG(error, "\"%s\" doesn't seem to be a .dat map file (invalid magic 0x%x)\n", filepath, magic);
        return false;
    }
    return true;
}

mapdata::mapdata(const char* filepath, polaris& pol) : pol(pol), filepath(filepath) {
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

void map_obj_to_viewport(viewport_t& viewport, const al::resource& alr, const ps01_entry* entry) noexcept {
    mesh_view mesh = mesh_at_idx(alr, FIRST_OBJ_IDX + entry->object_id, 0);
    for (index_buffer& idxbuf : mesh.idx_buffers) {
        idxbuf.is_skele_transform = false;
        idxbuf.position = (vec3s*)&entry->pos;
        idxbuf.rotation = (vec3s*)&entry->rotation;
    }
    viewport.meshes.push_back(mesh);
}

void mapdata::edit_ps01_entry(u32 idx, ps01_entry* entry, u32 max_id) noexcept {
    ImGui::ScopedIndent indent(ImGui::CharWidth(2));

    std::string cur_name = name_at_idx(entry->object_id);
    if (cur_name.empty()) {
        str_format_append(cur_name, "missing name [ID 0x%X]", entry->object_id);
    }

    std::string label;
    str_format_append(label, "Object Type##%d", idx);
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

    label = "";
    str_format_append(label, "Send to viewport##%d", idx);
    if (ImGui::Button(label.c_str())) {
        map_obj_to_viewport(pol.viewport, pol.editor.res, entry);
    }
}

void mapdata::edit_ps01_entries(st00_t* header, ps01_entry* entries) noexcept {
    if (header->ps00_count > 0) {
        const bool all_to_viewport = ImGui::Button("Send all to viewport");
        u32 offset = header->chunk_size;
        for (u32 i = 0; i < header->ps00_count; i++) {
            if (all_to_viewport) {
                map_obj_to_viewport(pol.viewport, pol.editor.res, &entries[i]);
            }
            ImGui::Text("@ 0x%X: ", offset);
            edit_ps01_entry(i, &entries[i], header->nm00_count);
            ImGui::Spacing();
            offset += sizeof(*entries);
        }
    }
}

void mapdata::edit_cp00_entries(s32 offset) noexcept {
    if (offset < 0) {
        ImGui::Text("No CP00 entry (offset %d)", offset);
        return;
    }

    cp00_t* header = (cp00_t*)(data + offset);
    ImGui::PushItemWidth(ImGui::CharWidth(30));
    for (u32 i = 0; i < header->num_entries; i++) {
        cp00_entry* entry = &header->entries[i];
        ImGui::Text("Spawn Point %d:", i);
        ImGui::ScopedIndent indent(ImGui::CharWidth(2));

        std::string label;
        str_format_append(label, "Player Spawn Point##%d", i);
        ImGui::InputFloat3(label.c_str(), &entry->player_pos.x);

        for (u32 j = 0; j < ARRAY_SIZE(entry->capsules); j++) {
            label = "";
            str_format_append(label, "Capsule %d##%d", j, i);
            ImGui::InputFloat3(label.c_str(), &entry->capsules[j].x);
        }
        for (u32 j = 0; j < 3; j++) {
            ImGui::Spacing();
        }
    }
    ImGui::PopItemWidth();
}

void mapdata::draw_custom_editor() {
    if (!ImGui::BeginTabBar("")) {
        return;
    }
    st00_t* header = get_header();
    auto* ps00_entries = (ps01_entry*)(data + header->chunk_size);
    auto* ps01_entries = (ps01_entry*)(data + header->ps01_offset);
    if (ImGui::BeginTabItem("PS0/")) {
        edit_ps01_entries(header, ps00_entries);
        ImGui::EndTabItem();
    }

    if (ImGui::BeginTabItem("PS01")) {
        edit_ps01_entries(header, ps01_entries);
        ImGui::EndTabItem();
    }

    if (ImGui::BeginTabItem("NM00")) {
        if (offset_is_reasonable(header->nm00_offset)) {
            char* txt = (char*)(data + header->nm00_offset);
            for (s32 i = 0; i < header->nm00_count; i++) {
                const s32 len = strlen(txt) + 1;
                std::string label = "Object " + std::to_string(i);
                ImGui::InputText(label.c_str(), txt, len);
                txt += len;
            }
        }
        ImGui::EndTabItem();
    }

    if (ImGui::BeginTabItem("CP00 (1)")) {
        if (offset_is_reasonable(header->CP00_offset1)) {
            edit_cp00_entries(header->CP00_offset1);
        }
        ImGui::EndTabItem();
    }

    if (ImGui::BeginTabItem("CP00 (2)")) {
        if (offset_is_reasonable(header->CP00_offset2)) {
            edit_cp00_entries(header->CP00_offset2);
        }
        ImGui::EndTabItem();
    }

    if (ImGui::BeginTabItem("CP00 (3)")) {
        if (offset_is_reasonable(header->CP00_offset3)) {
            edit_cp00_entries(header->CP00_offset3);
        }
        ImGui::EndTabItem();
    }

    if (ImGui::BeginTabItem("CP00 (4)")) {
        if (offset_is_reasonable(header->CP00_offset4)) {
            edit_cp00_entries(header->CP00_offset4);
        }
        ImGui::EndTabItem();
    }

    if (ImGui::BeginTabItem("CP00 (5)")) {
        if (offset_is_reasonable(header->CP00_offset5)) {
            edit_cp00_entries(header->CP00_offset5);
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
