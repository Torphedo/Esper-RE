#include "mapdata_editor.hxx"

#include "polaris.hxx"
#include <util/imgui_utils.hxx>
#include <util/utils.hxx>

mapdata_editor::mapdata_editor(const char* filepath, polaris& pol) : pol(pol) {
    map.filepath = filepath;
    initialized = map.load(filepath);
}

// Move assignment operator
mapdata_editor& mapdata_editor::operator=(mapdata_editor&& other) {
    if (this != &other) {
        map.unload();
        memcpy(this, &other, sizeof(other)); // Copy state from temporary
        // Wipe temporary so it doesn't free our pointer on destroy
        memset(&other, 0, sizeof(other));
    }

    return *this;
}

mapdata_editor::~mapdata_editor() {
    initialized = false;
}

void map_obj_to_viewport(alr::editor& ed, const ps01_entry* entry) noexcept {
    const u32 idx = FIRST_OBJ_IDX + entry->object_id;
    ed.instances.emplace_back(ed.meshes[idx], (vec3s*)&entry->pos, (vec3s*)&entry->rotation);
}

void mapdata_editor::edit_ps01_entry(u32 idx, ps01_entry* entry, u32 max_id) noexcept {
    ImGui::ScopedIndent indent(ImGui::CharWidth(2));

    std::string cur_name = map.name_at_idx(entry->object_id);
    if (cur_name.empty()) {
        str_format_append(cur_name, "missing name [ID 0x%X]", entry->object_id);
    }

    std::string label;
    str_format_append(label, "Object Type##%d", idx);
    if (ImGui::BeginCombo(label.c_str(), cur_name.c_str())) {
        st00_t* header = map.get_header();
        for (s32 i = 0; i < header->nm00_count; i++) {
            if (ImGui::Selectable(map.name_at_idx(i))) {
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
        map_obj_to_viewport(pol.editor, entry);
    }
}

void mapdata_editor::edit_ps01_entries(st00_t* header, ps01_entry* entries) noexcept {
    if (header->ps00_count > 0) {
        const bool all_to_viewport = ImGui::Button("Send all to viewport");
        u32 offset = header->chunk_size;
        for (u32 i = 0; i < header->ps00_count; i++) {
            if (all_to_viewport) {
                map_obj_to_viewport(pol.editor, &entries[i]);
            }
            ImGui::Text("@ 0x%X: ", offset);
            edit_ps01_entry(i, &entries[i], header->nm00_count);
            ImGui::Spacing();
            offset += sizeof(*entries);
        }
    }
}

void mapdata_editor::edit_cp00_entries(s32 offset) noexcept {
    if (offset < 0) {
        ImGui::Text("No CP00 entry (offset %d)", offset);
        return;
    }

    cp00_t* header = (cp00_t*)(map.data + offset);
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

void mapdata_editor::edit_oc00_entries(s32 offset, s32 end_offset) noexcept {
    if (ImGui::Button("Dump to oc00.obj")) {
        dump_oc00(offset, end_offset);
    }
}

void mapdata_editor::dump_oc00(s32 offset, s32 end_offset) noexcept {
    assert(end_offset > offset);

    FILE* f = fopen("oc00.obj", "wb");
    if (!f) {
        return;
    }

    while (end_offset > offset) {
        auto* generic_header = (oc00_header*)(map.data + offset);
        switch (generic_header->id) {
        case 3: { // Index buffer
            fprintf(f, "g objCol_%x\n", offset);
            auto* header = (oc00_idxbuf*)generic_header;
            for (u32 i = 0; i < header->num_entries; i++) {
                const u16* indices = header->entries[i].indices;
                fprintf(f, "f %hd %hd %hd\n", indices[0], indices[1], indices[2]);
            }
            break;
        }
        case 8: { // Vertex buffer
            auto* header = (oc00_vertbuf*)generic_header;
            const u32 num_entries = generic_header->size / sizeof(*header->points);
            for (u32 i = 0; i < num_entries; i++) {
                const vec3f& p = header->points[i];
                fprintf(f, "v %f %f %f\n", p.x, p.y, p.z);
            }
            break;
        }
        default:
            break;
            // return;
        }

        offset += sizeof(*generic_header);
        if (offset < end_offset) {
            offset += generic_header->size;
        }
    }

    fclose(f);
}

void mapdata_editor::draw_custom_editor() {
    if (!ImGui::BeginTabBar("")) {
        return;
    }
    st00_t* header = map.get_header();
    auto* ps00_entries = (ps01_entry*)(map.data + header->chunk_size);
    auto* ps01_entries = (ps01_entry*)(map.data + header->ps01_offset);
    if (ImGui::BeginTabItem("PS0/")) {
        edit_ps01_entries(header, ps00_entries);
        ImGui::EndTabItem();
    }

    if (ImGui::BeginTabItem("PS01")) {
        edit_ps01_entries(header, ps01_entries);
        ImGui::EndTabItem();
    }

    if (ImGui::BeginTabItem("NM00")) {
        if (map.offset_is_reasonable(header->nm00_offset)) {
            char* txt = (char*)(map.data + header->nm00_offset);
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
        if (map.offset_is_reasonable(header->CP00_offset1)) {
            edit_cp00_entries(header->CP00_offset1);
        }
        ImGui::EndTabItem();
    }

    if (ImGui::BeginTabItem("CP00 (2)")) {
        if (map.offset_is_reasonable(header->CP00_offset2)) {
            edit_cp00_entries(header->CP00_offset2);
        }
        ImGui::EndTabItem();
    }

    if (ImGui::BeginTabItem("CP00 (3)")) {
        if (map.offset_is_reasonable(header->CP00_offset3)) {
            edit_cp00_entries(header->CP00_offset3);
        }
        ImGui::EndTabItem();
    }

    if (ImGui::BeginTabItem("CP00 (4)")) {
        if (map.offset_is_reasonable(header->CP00_offset4)) {
            edit_cp00_entries(header->CP00_offset4);
        }
        ImGui::EndTabItem();
    }

    if (ImGui::BeginTabItem("CP00 (5)")) {
        if (map.offset_is_reasonable(header->CP00_offset5)) {
            edit_cp00_entries(header->CP00_offset5);
        }
        ImGui::EndTabItem();
    }

    if (ImGui::BeginTabItem("OC00")) {
        if (map.offset_is_reasonable(header->OC00_offset)) {
            edit_oc00_entries(header->OC00_offset, header->OC01_offset);
        }
        ImGui::EndTabItem();
    }

    if (ImGui::BeginTabItem("OC01")) {
        if (map.offset_is_reasonable(header->OC01_offset)) {
            edit_oc00_entries(header->OC01_offset, header->OC02_offset);
        }
        ImGui::EndTabItem();
    }

    ImGui::EndTabBar();
}

void mapdata_editor::do_gui() noexcept {
    if (!this->initialized) {
        return; // Ignore if not initialized
    }

    ImGui::Begin(".dat Data");

    if (ImGui::BeginTabBar("Chunk Tabs")) {
        if (ImGui::BeginTabItem("Chunks")) {
            draw_custom_editor();
            ImGui::EndTabItem();
        }
        if (ImGui::BeginTabItem("Raw file data")) {
            hex_edit.DrawContents((void *) map.data, map.size);
            ImGui::EndTabItem();
        }

        ImGui::EndTabBar();
    }

    ImGui::End();
}
