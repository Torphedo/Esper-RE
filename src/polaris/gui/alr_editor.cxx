// Need this define to use operators on ImGui vector types
#define IMGUI_DEFINE_MATH_OPERATORS
#include <glad/glad.h>
#include "alr_editor.hxx"
#include <nfd.h>

#include <common/file.h>
#include <common/crc32.h>

#include <formats/alr.h>

#include <validation.hxx>
#include <alr/alr_dump.hxx>
#include <util/imgui_utils.hxx>

#include "alr_imgui.hxx"

// Normally I'd make this a method, but by using a macro we can have LOG_MSG()
// automatically log the name of the method that shouldn't have been called.
#define CHUNK_ID_ASSERT(expected_id) \
do {                                 \
    if (chunk.id != expected_id) {         \
        LOG_MSG(warning, "called on 0x%x chunk @ 0x%x, when it only makes sense for 0x%x chunks!\n", chunk.id, chunk.offset, expected_id);\
        return;                      \
    }                                \
} while(0)

namespace alr {

void editor::window_state::draw_chunk_material(editor& ed, file::chunk& chunk) noexcept {
    CHUNK_ID_ASSERT(ALR_ID_MATERIAL);
    file& alr = ed.alr;

    vfile vf = vfile_open(alr.data, alr.alr_size);
    vf.pos = chunk.offset;
    const material_header header = VFILE_READ(material_header, &vf);
    auto entries = (material_entry*) vfile_cur(vf);

    ImGui::BeginChildFitContent("Entries");
    for (u32 i = 0; i < header.num_entries; i++) {
        char buf[0x30] = {0};
        snprintf(buf, sizeof(buf) - 1, "Entry #%d  ", i);

        const bool is_selected = win_material.selected_entry == i;
        if (ImGui::Selectable(buf, is_selected)) {
            win_material.selected_entry = i;
        }
    }

    ImGui::EndChild();
    ImGui::SameLine();

    material_entry& entry = entries[win_material.selected_entry];
    // Explicit constructor

    ImGui::BeginChild("editor child window", ImVec2(), ImGuiChildFlags_AutoResizeX);
    if (ImGui::BeginTabBar("editor tabs")) {
        if (ImGui::BeginTabItem("Editor")) {
            edit_material_entry(entry);
            ImGui::Image(ed.tex_manager.get(alr, entry.texture_idx), ImVec2(500, 500));
            ImGui::EndTabItem();
        }
        if (ImGui::BeginTabItem("Hex Editor")) {
            hex_chunk.DrawContents(&entry, sizeof(entry), (uintptr_t) &entry - (uintptr_t) alr.data);
            ImGui::EndTabItem();
        }
        ImGui::EndTabBar();
    }
    ImGui::EndChild();
}

void editor::window_state::draw_chunk_idxbuf(file& alr, file::chunk& chunk) noexcept {
    CHUNK_ID_ASSERT(ALR_ID_INDICES);

    if (ImGui::Button("Export to OBJ")) {
        // Display the file picker
        nfdu8filteritem_t filters[] = { { "3D Model", "obj"} };
        char* path = nullptr;
        nfdresult_t result = NFD_SaveDialogU8(&path, filters, ARRAY_SIZE(filters), nullptr, nullptr);
        if (result == NFD_OKAY && path != nullptr) {
            FILE* out = fopen(path, "ab");
            if (out) {
                alr::dump_idx_buf(alr.data, chunk.offset, out, false);
                fclose(out);
            }
        }
        free(path);
    }

    // Index buffer editing
    vfile vf = vfile_open(alr.data + chunk.offset, chunk.size);
    idxbuf_header* header = VFILE_READ_PTR(idxbuf_header, &vf);

    ImGui::PushItemWidth(ImGui::CharWidth() * 32);

    ImGui::InputU16("Texture entry ID", &header->texture_idx);
    ImGui::InputU16("Transform index", &header->transform_idx);
    ImGui::InputU16("Vertex Buffer", &header->vertex_buf);

    ImGui::InputU16("Primitive type", &header->primitive_type);
    ImGui::InputU32("# of triangles", &header->num_tris);
    ImGui::InputU32("# of indices", &header->num_indices);
    ImGui::InputU32("Smallest index", &header->first_idx);
    ImGui::InputFloat3("Center point", header->center);
    ImGui::InputFloat3("AABB Min", header->aabb_min);
    ImGui::InputFloat3("AABB Max", header->aabb_max);

    for (u32 i = 0; i < 5; i++) {
        ImGui::Spacing();
    }

    if (ImGui::CollapsingHeader("Unknown Fields")) {
        ImGui::InputFloat("Unknown float 1", &header->unk_float);

        ImGui::InputU32("Unknown integer 1", &header->unk1);
        ImGui::SetNextItemWidth(ImGui::CharWidth() * 12 * 6);
        ImGui::InputScalarN("Unknown integer 4", ImGuiDataType_U16, header->unk4, 6);
    }

    if (ImGui::CollapsingHeader("Edit indices")) {
        for (s32 i = 0; i < header->num_indices; i++) {
            // User inputs for this triangle
            char label[0x20] = {0};
            snprintf(label, sizeof(label), "Index %d", i + 1);
            u16* idx = VFILE_READ_PTR(u16, &vf);
            ImGui::InputU16(label, idx);
        }
    }

    ImGui::PopItemWidth();
}

void editor::window_state::draw_chunk_skeleton(const file& alr, file::chunk& chunk) noexcept {
    CHUNK_ID_ASSERT(ALR_ID_SKELETON);

    vfile vf = vfile_open(alr.data + chunk.offset, chunk.size);

    const u32 num_joints = (chunk.size - sizeof(chunk_armature)) / sizeof(joint_t);
    const chunk_armature header = VFILE_READ(chunk_armature, &vf);
    auto* joints = (joint_t *) vfile_cur(vf);

    ImGui::Text("%d joints [%d identity]", num_joints, num_joints - header.joint_count);

    const u32 min = 0;
    const u32 max = MAX(num_joints - 1, 0);
    ImGui::Checkbox("Use slider", &win_skel.slider);
    const char* inputlabel = "Selected Joint";
    if (win_skel.slider) {
        ImGui::SliderScalar(inputlabel, ImGuiDataType_S32, &win_skel.selected_joint, &min, &max);
    } else {
        ImGui::InputScalar(inputlabel, ImGuiDataType_S32, &win_skel.selected_joint);
    }
    // Don't allow out of bounds index
    win_skel.selected_joint = CLAMP(min, win_skel.selected_joint, max);

    joint_t& joint = joints[win_skel.selected_joint];
    alr::edit_joint_t(joint, vf, hex_edit);
}

void editor::window_state::draw_chunk_animation(const file& alr, file::chunk& chunk) noexcept {
    CHUNK_ID_ASSERT(ALR_ID_ANIMATION);

    vfile vf = vfile_open(alr.data + chunk.offset, chunk.size);
    anim_header* header = (anim_header*)vfile_cur(vf);
    vfile_seek(&vf, sizeof(*header));

    if (ImGui::Button("Dump animation")) {
        file::chunk armature_chunk = alr.first_chunk_in_range(ALR_ID_SKELETON, chunk.offset, alr.resbuf_offset);
        vfile armature_vf = vfile_open(alr.data + armature_chunk.offset, armature_chunk.size);
        const auto armature_header = VFILE_READ(chunk_armature, &armature_vf);
        const auto* joints = (joint_t*)vfile_cur(armature_vf);

        const u32 joint_idx = header->joint_idx;
        const joint_t joint = joints[joint_idx];

        decoded_text decoded = {};
        decode_single32(decoded.data, joint.name);
        std::string joint_name = std::string(decoded.data) + "_" + std::to_string(joint_idx);

        // Display the file picker
        nfdu8filteritem_t filters[] = { { "Autodesk Maya Animation", "anim"} };
        char* path = nullptr;
        nfdresult_t result = NFD_SaveDialogU8(&path, filters, ARRAY_SIZE(filters), nullptr, nullptr);
        if (result == NFD_OKAY && path != nullptr) {
            dump_animation_maya(header, path, joint_name.c_str());
        }
        free(path);
    }

    {
        ImGui::ScopedWidth scope(15);
        ImGui::InputFloat("Animation Length", &header->length);
        ImGui::InputU32("# translation keys", &header->translation_key_count);
        ImGui::InputU16("Translation key size", &header->translation_key_size);

        ImGui::NewLine();
        ImGui::InputU32("# rotation keys", &header->rotation_key_count);
        ImGui::InputU16("Rotation key size", &header->rotation_key_size);

        ImGui::NewLine();
        ImGui::InputU32("# scale keys", &header->scale_key_count);

        ImGui::NewLine();
        ImGui::InputU16("Joint Index", &header->joint_idx);
        ImGui::InputU8("Unknown 2", &header->unknown_settings2);
        ImGui::InputU8("Unknown 3", &header->unknown_settings3);
    }

    // Edit and skip to the next set of keys
    if (header->translation_key_count > 0 && ImGui::CollapsingHeader("Translation Keys")) {
        edit_keyframes(header->translation_key_size, header->translation_key_count, vfile_cur(vf), "trans");
    }
    vfile_seek(&vf, header->translation_key_size * header->translation_key_count);

    if (header->rotation_key_count > 0 && ImGui::CollapsingHeader("Rotation Keys")) {
        edit_keyframes(header->rotation_key_size, header->rotation_key_count, vfile_cur(vf), "rot");
    }
    vfile_seek(&vf, header->rotation_key_size * header->rotation_key_count);
    if (header->scale_key_count > 0 && ImGui::CollapsingHeader("Scale Keys")) {
        edit_keyframes(header->unknown_settings2, header->scale_key_count, vfile_cur(vf), "scale");
    }
}

void editor::window_state::draw_chunk_cam_anim(const file& alr, file::chunk& chunk) noexcept {
    CHUNK_ID_ASSERT(ALR_ID_CAM_ANIM);
    // This is the same as normal animation frames, but seems to ignore the
    // existing keyframe size fields.
    vfile vf = vfile_open(alr.data + chunk.offset, chunk.size);
    anim_header* header = (anim_header*)vfile_cur(vf);
    vfile_seek(&vf, sizeof(*header));

    ImGui::Text("Length: %.3f frames", header->length);
    ImGui::Text("%d translation keys", header->translation_key_count);
    ImGui::Text("%d rotation keys", header->rotation_key_count);
    ImGui::Text("%d scale keys", header->scale_key_count);

    // Edit and skip to the next set of keys
    const u32 key_size = 0x10; // Camera path keys seem to always be this size.
    if (header->translation_key_count > 0 && ImGui::CollapsingHeader("Translation Keys")) {
        edit_keyframes(key_size, header->translation_key_count, vfile_cur(vf), "trans");
    }
    vfile_seek(&vf, key_size * header->translation_key_count);

    if (header->rotation_key_count > 0 && ImGui::CollapsingHeader("Rotation Keys")) {
        edit_keyframes(key_size, header->rotation_key_count, vfile_cur(vf), "rot");
    }
    vfile_seek(&vf, key_size * header->rotation_key_count);
}

void editor::window_state::draw_chunk_atlas(editor& ed, file::chunk& chunk) noexcept {
    CHUNK_ID_ASSERT(ALR_ID_TEXATLAS);
    file& alr = ed.alr;

    vfile vf = vfile_open(alr.data + chunk.offset, chunk.size);

    auto* header = VFILE_READ_PTR(atlas_header, &vf);
    auto* atlas_names = (atlas_name*)vfile_cur(vf);
    vfile_seek(&vf, sizeof(*atlas_names) * header->atlas_count);

    auto* atlases = (atlas_entry*)vfile_cur(vf);
    vfile_seek(&vf, sizeof(*atlases) * header->atlas_count);

    auto* textures = VFILE_READ_PTR(atlas_tex_entry , &vf);

    // We have to look up texture entries to find out where each texture is
    texture_entry* entries = nullptr;
    file::chunk c = alr.first_chunk_by_id(ALR_ID_TEXTURE);
    if (c.size == 0) {
        // This should never happen
        ImGui::PlsReportIf(true, "Couldn't find a texture (0x15) chunk!\n");
        return;
    } else {
        // Skip to the chunk
        vfile tmp = vfile_open(alr.data + c.offset, c.size);
        vfile_seek(&tmp, sizeof(chunk_generic));

        const u32 num_entries = VFILE_READ(u32, &tmp);
        entries = (texture_entry*)vfile_cur(tmp);
    }


    ImGui::BeginChildFitContent("selectionContainer");
    ImGui::BeginChildFitContent("Atlases");
    ImGui::Text("%d Atlases for %.*s:", header->atlas_count, (int)sizeof(header->alr_name), header->alr_name);
    for (u32 i = 0; i < header->atlas_count; i++) {
        char buf[sizeof(atlas_names[i].name) + 0x20] = {0};
        snprintf(buf, sizeof(buf) - 1, "%s##%d", atlas_names[i].name, i);

        if (ImGui::Selectable(buf, win_atlas.selected_atlas == i)) {
            win_atlas.selected_atlas = i;
        }
    }
    ImGui::EndChild();


    // The currently selected texture might be in a different atlas, which would
    // display a strange & unintuitive result. We find the first and last index
    // of textures in the atlas, and make sure the selected texture is always
    // a child of the selected atlas.
    if (textures[win_atlas.selected_atlas_texture].index != win_atlas.selected_atlas) {
        for (u32 i = 0; i < header->texture_count; i++) {
            if (textures[i].index == win_atlas.selected_atlas) {
                // It's a match!
                win_atlas.selected_atlas_texture = i;
                break;
            }
        }
    }

    // Display textures in the selected atlases
    ImGui::Text("\nTextures: ");
    ImGui::BeginChildFitContent("Textures");
    for (u32 i = 0; i < header->texture_count; i++) {
        const atlas_tex_entry tex = textures[i];
        // Only list textures belonging to the selected atlases
        if (tex.index != win_atlas.selected_atlas) {
            continue;
        }

        char buf[sizeof(textures[i].filename) + 0x20] = {0};
        snprintf(buf, sizeof(buf) - 1, "%s##%d", tex.filename, i);

        if (ImGui::Selectable(buf, win_atlas.selected_atlas_texture == i)) {
            win_atlas.selected_atlas_texture = i;
        }
    }
    ImGui::EndChild(); // End texture select

    ImGui::EndChild(); // End selectionContainer
    ImGui::SameLine();

    atlas_name* aName = &atlas_names[win_atlas.selected_atlas];
    atlas_entry* atlas = &atlases[win_atlas.selected_atlas];
    atlas_tex_entry* tex = &textures[win_atlas.selected_atlas_texture];
    texture cur_tex = convert_tex(alr.resource_buffer(), entries[tex->index]);

    ImGui::BeginChildFitContent("texture editing");
    // Override dimensions, we only want format info from the other chunk
    cur_tex.height = atlas->height;
    cur_tex.width = atlas->width;
    ImGui::Text("Atlas uses texture index %d, see texture (0x15) chunk for offset & format", win_atlas.selected_atlas);
    if (alr::edit_atlas_entry(*atlas, *aName)) {
        // Texture settings have changed, trigger reload
        ed.tex_manager.invalidate(win_atlas.selected_atlas);
    }

    win_atlas.gl_tex_id = ed.tex_manager.get(alr, win_atlas.selected_atlas);
    win_atlas.tex = cur_tex;

    // Draw the whole atlas
    ImVec2 image_pos = ImGui::draw_image(win_atlas.gl_tex_id, atlas->width, atlas->height, &win_atlas.use_actual_size_atlas, &win_atlas.scale_atlas, "atlas");

    // Calculate UVs of the selected texture in the atlas
    const ImVec2 uv1 = ImVec2(tex->atlas_texcoords[0], tex->atlas_texcoords[1]);
    const ImVec2 uv0 = ImVec2(uv1.x - ((float)tex->width / atlas->width), uv1.y - ((float)tex->height / atlas->height));

    // Draw a bounding box over a single texture in the atlas
    const ImVec2 atlas_drawn_size = ImVec2(atlas->width, atlas->height) * win_atlas.scale_atlas;
    const ImVec2 start = image_pos + (atlas_drawn_size * uv0);
    const ImVec2 end = image_pos + (atlas_drawn_size * uv1);
    ImGui::GetWindowDrawList()->AddRect(start, end, 0xFF00FF00);

    alr::edit_atlas_texture(*tex);
    ImGui::draw_image(win_atlas.gl_tex_id, tex->width, tex->height, &win_atlas.use_actual_size, &win_atlas.scale, "texture", uv0, uv1);

    ImGui::EndChild();
}

void editor::window_state::draw_chunk_header(file& alr, file::chunk& chunk) const noexcept {
    CHUNK_ID_ASSERT(ALR_ID_HEADER);
    vfile vf = vfile_open(alr.data + chunk.offset, chunk.size);
    auto* layout = (chunk_layout*)vfile_cur(vf);

    alr::edit_chunk_layout(*layout, alr);
}

void editor::window_state::import_dds_0x15(editor& ed, const char* path, u32 num_entries, texture_entry* entries) noexcept {
    file& alr = ed.alr;
    const file::chunk chunk = alr.chunks[chunk_idx];
    CHUNK_ID_ASSERT(0x15);

    if (num_entries == 0) {
        LOG_MSG(error, "There are no textures to replace!\n");
    }

    texture_entry& cur = entries[win_texture.selected_texture];
    const texture_entry& next = entries[win_texture.selected_texture + 1];
    s64 tex_size = 0;
    bool is_last = false;
    if (win_texture.selected_texture >= (num_entries - 1)) {
        // This is the last entry, so the best guess is that it takes up the
        // rest of the file
        tex_size = (alr.alr_size) - alr.resbuf_offset - (s64)cur.data_ptr;
        is_last = true;
    } else {
        // The most likely texture size is the distance betwen this texture and
        // the next
        tex_size = next.data_ptr - cur.data_ptr;
    }

    const u32 needed_size = image_required_size_file(path);
    if (needed_size != tex_size) {
        const s32 diff = (s32)needed_size - (s32)tex_size;
        if (is_last) {
            // Expand the file to make room
            alr.expand_resbuf(diff);
        } else {
            // Move the next texture forward to make room, or shrink it to come after this texture
            alr.shift_resource(next.data_ptr, diff);
        }
    }

    // Load the texture data
    u8* data = alr.resource_buffer() + cur.data_ptr;
    texture tex = image_buf_load(path, data, needed_size);

    // Update the ALR state and force a reload
    alr_texture_set_dimensions(&cur, tex.height, tex.width);
    ed.tex_manager.invalidate_all();
}

void editor::window_state::draw_chunk_texture(editor& ed, file::chunk& chunk) noexcept {
    CHUNK_ID_ASSERT(ALR_ID_TEXTURE);
    file& alr = ed.alr;

    // We use the vfile API to handle the chunk data
    vfile vf = vfile_open(alr.data + chunk.offset, chunk.size);
    // Skip over the ID and size fields we already have
    vfile_seek(&vf, sizeof(chunk_generic));

    const u32 num_entries = VFILE_READ(u32, &vf);
    auto* entries = (texture_entry*)vfile_cur(vf);

    ImGui::BeginChild("Textures", ImVec2(0, 0), ImGuiChildFlags_AutoResizeX);
    for (u32 i = 0; i < num_entries; i++) {
        char buf[0x30] = {0};
        decoded_text name = {0};
        decode_single32(name.data, entries[i].text1);
        decode_single32(&name.data[ENCODED_CHAR_COUNT], entries[i].text2);

        snprintf(buf, sizeof(buf) - 1, "#%d \"%s\" @ resbuf+0x%X", i, name.data, entries[i].data_ptr);

        if (ImGui::Selectable(buf, win_texture.selected_texture == i)) {
            win_texture.selected_texture = i;
        }
    }
    ImGui::EndChild();
    ImGui::SameLine();

    ImGui::BeginGroup();
    texture_entry& entry = entries[win_texture.selected_texture];

    if (alr::edit_texture_entry(entry)) {
        ed.tex_manager.invalidate(win_texture.selected_texture);
    }

    if (ImGui::Button("Import DDS")) {
        // Display the file picker
        nfdu8filteritem_t filters[] = { { "DDS Image", "dds"} };
        char* path = nullptr;
        nfdresult_t result = NFD_OpenDialogU8(&path, filters, ARRAY_SIZE(filters), nullptr);
        if (result == NFD_OKAY && path != nullptr) {
            import_dds_0x15(ed, path, num_entries, entries);
        }
        free(path);
    }

    win_texture.gl_tex_id = ed.tex_manager.get(alr, win_texture.selected_texture);
    ImGui::SameLine();
    if (ImGui::Button("Export DDS")) {
        ed.tex_edit.tex_export_active = true;
        ed.tex_edit.export_cfg = convert_tex(alr.resource_buffer(), entry);
        ed.tex_edit.export_cfg.data = (u8*)uintptr_t(entry.data_ptr);
        ed.tex_edit.export_tex_idx = win_texture.selected_texture;
    }

    u16 height = 0;
    u16 width = 0;
    alr_texture_get_dimensions(entry, &height, &width);
    ImGui::draw_image(win_texture.gl_tex_id, width, height, &win_texture.use_actual_size, &win_texture.scale, "preview");
    ImGui::EndGroup();
}

void editor::window_state::send_vertbuf_to_viewport(editor& ed) noexcept {
    file& alr = ed.alr;
    const file::chunk chunk = alr.chunks[chunk_idx];
    CHUNK_ID_ASSERT(ALR_ID_MODEL);

    // Find out what index we are
    s32 idx = -1;
    for (file::chunk c : alr.chunks) {
        if (c.offset > chunk.offset) {
            break;
        }
        if (c.id == ALR_ID_MATERIAL) {
            idx++;
        }
    }

    ed.instances.emplace_back(ed.meshes[idx]);
}

void editor::window_state::draw_chunk_vertbuf(editor& ed, file::chunk& chunk) noexcept {
    file& alr = ed.alr;
    CHUNK_ID_ASSERT(ALR_ID_MODEL);

    // We use the vfile API to handle the chunk data
    vfile vf = vfile_open(alr.data + chunk.offset, chunk.size);
    // Skip over the ID and size fields we already have (both 32-bit)
    vfile_seek(&vf, sizeof(chunk_generic));

    const u32 num_entries = VFILE_READ(u32, &vf);
    auto* entries = (vertbuf_entry*)vfile_cur(vf);

    ImGui::BeginChild("Vertex Buffers", ImVec2(300, 0));
    for (u32 i = 0; i < num_entries; i++) {
        char buf[0x30] = {0};
        snprintf(buf, sizeof(buf) - 1, "0x%X verts @ 0x%X", entries[i].vertex_count, entries[i].data_ptr);

        const bool is_selected = window_vertbuf.selected_vertex_buf == i;
        if (ImGui::Selectable(buf, is_selected)) {
            window_vertbuf.selected_vertex_buf = i;
        }
    }

    ImGui::EndChild();
    ImGui::SameLine();

    vertbuf_entry* entry = &entries[window_vertbuf.selected_vertex_buf];
    ImGui::BeginChild("Vertex Buffer Settings", ImVec2(600, 0));
    if (ImGui::Button("Dump to OBJ")) {
        // Display the file picker
        nfdu8filteritem_t filters[] = { { "3D Model", "obj"} };
        char* path = nullptr;
        nfdresult_t result = NFD_SaveDialogU8(&path, filters, ARRAY_SIZE(filters), nullptr, nullptr);
        if (result == NFD_OKAY && path != nullptr) {
            alr::dump_vertex_buf(alr, path, chunk.offset, window_vertbuf.selected_vertex_buf);
        }
        free(path);
    }
    if (ImGui::Button("Import OBJ")) {
        // Display the file picker
        nfdu8filteritem_t filters[] = { { "3D Model", "obj"} };
        char* path = nullptr;
        nfdresult_t result = NFD_OpenDialogU8(&path, filters, ARRAY_SIZE(filters), nullptr);
        if (result == NFD_OKAY && path != nullptr) {
            char* obj_data = (char*)file_load(path);
            if (obj_data) {
                obj_import(obj_data, alr, chunk.offset, window_vertbuf.selected_vertex_buf);
                free(obj_data);
            }
        }
        free(path);
    }

    if (ImGui::Button("Send to Viewport")) {
        send_vertbuf_to_viewport(ed);
    }

    if (ImGui::CollapsingHeader("Shift Buffer")) {
        ImGui::InputS32("Shift Amount", &window_vertbuf.shift_amount);
        if (ImGui::Button("Go!")) {
            alr.shift_resource(entry->data_ptr, window_vertbuf.shift_amount);
        }
    }

    alr::edit_vertbuf_entry(*entry);
    ImGui::EndChild();

    ImGui::BeginChild("Vertex Buffer Hex Editor", ImVec2(800, 500));

    // Hex editor for vertex buffer data
    u8* vertbuf = alr.resource_buffer() + entry->data_ptr;
    window_vertbuf.hex_vertbuf.DrawContents(vertbuf, entry->vertex_count * entry->vertex_size);
    ImGui::EndChild();
}

void editor::window_state::draw_chunk_0x14(editor& ed, file::chunk& chunk) noexcept {
    CHUNK_ID_ASSERT(0x14);
    vfile vf = ed.alr.vf_from_chunk(chunk);
    chunk_0x14* header = (chunk_0x14*)vfile_cur(vf);
    edit_chunk_0x14(*header);
}

void editor::window_state::update(editor& ed) noexcept {
    if (!ed.alr.data || ed.alr.alr_size == 0) {
        // There's no data to work on, we can't display any useful data.
        return;
    }

    file::chunk& chunk = ed.alr.chunks[chunk_idx];

    // Sanity check some of our assumptions & show warning messages if they fail
    std::string msg;

    const bool valid = alr_chunk_validate(ed.alr, chunk, msg, false);
    ImGui::PlsReportIf(msg.length() > 0, msg.c_str());

    ImGui::SetNextItemWidth(ImGui::CharWidth(12));
    ImGui::InputU32("Shift amount", &shift_amount);
    ImGui::SameLine();
    if (ImGui::Button("Shift Chunk")) {
        ed.alr.shift_chunks(chunk.offset, shift_amount);
    }

    if (ImGui::BeginTabBar("Chunk Tabs")) {
        if (ImGui::BeginTabItem("Specialized Chunk Editor")) {
            switch (chunk.id) {
                case ALR_ID_MATERIAL:
                    draw_chunk_material(ed, chunk);
                    break;
                case ALR_ID_INDICES:
                    draw_chunk_idxbuf(ed.alr, chunk);
                    break;
                case ALR_ID_SKELETON:
                    draw_chunk_skeleton(ed.alr, chunk);
                    break;
                case ALR_ID_ANIMATION:
                    draw_chunk_animation(ed.alr, chunk);
                    break;
                case ALR_ID_CAM_ANIM:
                    draw_chunk_cam_anim(ed.alr, chunk);
                    break;
                case ALR_ID_TEXATLAS:
                    draw_chunk_atlas(ed, chunk);
                    break;
                case ALR_ID_HEADER:
                    draw_chunk_header(ed.alr, chunk);
                    break;
                case ALR_ID_TEXTURE:
                    draw_chunk_texture(ed, chunk);
                    break;
                case ALR_ID_MODEL:
                    draw_chunk_vertbuf(ed, chunk);
                    break;
                case 0x14:
                    draw_chunk_0x14(ed, chunk);
                    break;
                default:
                    // Unimplemented window
                    ImGui::Text("[No special editor available]");
            }
            ImGui::EndTabItem();
        }

        if (ImGui::BeginTabItem("Raw Chunk Data")) {
            // Hex editor for the entire chunk, displayed with correct file offsets
            hex_chunk.DrawContents(ed.alr.data + chunk.offset, chunk.size, chunk.offset);
            ImGui::EndTabItem();
        }
        ImGui::EndTabBar();
    }
}

editor::window_state::window_state(u32 chunk_idx, u32 chunk_id) : chunk_idx(chunk_idx) {
    switch (chunk_id) {
    case ALR_ID_MATERIAL:
        win_material = {};
        break;
    case ALR_ID_SKELETON:
        win_skel = {};
        break;
    case ALR_ID_TEXATLAS:
        win_atlas = {};
        break;
    case ALR_ID_TEXTURE:
        win_texture = {};
        break;
    case ALR_ID_MODEL:
        window_vertbuf = {};
        break;
    }
}

void editor::tex_edit_state_t::draw(editor& ed) noexcept {
    file& alr = ed.alr;
    if (!tex_export_active) {
        return;
    }

    if (!offset_atlas) {
        offset_atlas = alr.first_chunk_by_id(ALR_ID_TEXATLAS).offset;
    }
    if (!offset_texture) {
        offset_texture = alr.first_chunk_by_id(ALR_ID_TEXTURE).offset;
    }

    const u32 hash = crc32fast((u8*)this, sizeof(*this));

    vfile vf = vfile_open(alr.data, alr.alr_size);
    vf.pos = offset_texture;
    const auto tex_header = VFILE_READ(texture_header, &vf);
    const texture_entry* entries = (texture_entry*)vfile_cur(vf);

    vf.pos = offset_atlas;
    const auto aHeader = VFILE_READ(atlas_header, &vf);
    vfile_seek(&vf, sizeof(atlas_name) * aHeader.atlas_count);
    const auto aEntries = (atlas_entry*)vfile_cur(vf);

    if (!ImGui::Begin("Advanced Texture Export", &tex_export_active)) {
        return;
    }

    ImGui::PushItemWidth(ImGui::CharWidth(20));

    const u32 expected_offset = entries[export_tex_idx].data_ptr;
    const bool uninitialized = (expected_offset != (uintptr_t)export_cfg.data) && !override_buf;
    if (ImGui::InputU32("Texture index", &export_tex_idx) || uninitialized) {
        export_cfg.data = (u8 *) u64(entries[export_tex_idx].data_ptr);
    }
    ImGui::EditTexture(export_cfg);

    ImGui::Checkbox("Override buffer settings", &override_buf);
    if (override_buf) {
        ImGui::ScopedIndent indent;
        ImGui::InputU64("Resbuf offset", (uintptr_t*)&export_cfg.data, 1, 5, nullptr, ImGuiInputTextFlags_CharsHexadecimal);
    }

    if (hash != crc32fast((u8*)this, sizeof(*this))) {
        // Settings have changed
        ed.tex_manager.invalidate(export_tex_idx);
        gl_obj tex_id = 0;
        glGenTextures(1, &tex_id);
        if (tex_id != 0) {
            // Need to turn offset into pointer while uploading texture
            export_cfg.data += (uintptr_t)alr.resource_buffer();
            update_gl_tex(export_cfg, tex_id);
            export_cfg.data -= (uintptr_t)alr.resource_buffer();
            ed.tex_manager.gl_tex_map[export_tex_idx] = tex_id;
        }
    }

    if (ImGui::CollapsingHeader("Guessing")) {
        ImGui::Checkbox("Use atlas entry data", &guess_atlas);
        if (ImGui::Button("Make a guess")) {
            export_cfg = convert_tex(alr.resource_buffer(), entries[export_tex_idx]);
            if (guess_atlas) {
                export_cfg.width = aEntries[export_tex_idx].width;
                export_cfg.height = aEntries[export_tex_idx].height;
            }
        }
    }

    if (ImGui::CollapsingHeader("Preview")) {
        const ImVec2 size(export_cfg.width, export_cfg.height);
        ImGui::Image(ed.tex_manager.get(alr, export_tex_idx), size);
    }

    if (ImGui::Button("Export")) {
        decoded_text name = decode_double(entries[export_tex_idx].text1, entries[export_tex_idx].text2);

        // Display the file picker
        nfdu8filteritem_t filters[] = { { "DDS Image", "dds"} };
        char* path = nullptr;
        nfdresult_t result = NFD_SaveDialogU8(&path, filters, ARRAY_SIZE(filters), nullptr, name.data);
        if (result == NFD_OKAY && path != nullptr) {
            // We only put a resbuf offset here normally, make it a pointer briefly
            export_cfg.data += (uintptr_t)alr.resource_buffer();
            img_write(export_cfg, path);
            export_cfg.data -= (uintptr_t)alr.resource_buffer();
            tex_export_active = false;
        }
        free(path);
    }

    ImGui::PopItemWidth();
    ImGui::End();
}


void editor::load_all_meshes() noexcept {
    u32 num_models = 0;
    alr.first_model_idx(&num_models);

    meshes.resize(num_models);
    for (u32 i = 0; i < num_models; i++) {
        alr_model_desc model = alr.model_at_idx(i);
        if (model.vert_chunk == nullptr) {
            continue;
        }
        meshes[i] = load_alr_mesh(alr, i);
    }

    // Stage ALRs have a base, background, and skybox model.
    // Player ALRs have at most a base and detail (e.g. scarf) model.
    for (u32 i = 0; i < MIN(meshes.size(), 6); i++) {
        alr_model_desc model = alr.model_at_idx(i);
        if (meshes[i].chunks.vert_chunk == nullptr) {
            continue;
        }
        instances.emplace_back(meshes[i]);
    }
}

void editor::clear_meshes() noexcept {
    for (alr::mesh& mesh : meshes) {
        mesh.destroy();
    }
    meshes.clear();
    instances.clear();
}

const char* chunk_name_by_id(u32 id) {
    switch (id) {
    case ALR_ID_MATERIAL:  return "[Material]";
    case ALR_ID_INDICES:   return "[Index Buffer]";
    case ALR_ID_SKELETON:  return "[Armature]";
    case ALR_ID_ANIMATION: return "[Animation]";
    case ALR_ID_CAM_ANIM:  return "[Camera Path]";
    case ALR_ID_TEXATLAS:  return "[Texture Atlas]";
    case ALR_ID_HEADER:    return "[Header]";
    case ALR_ID_TEXTURE:   return "[Texture]";
    case ALR_ID_MODEL:     return "[Model]";
    }

    return "";
}

void editor::update(render_context& ctx) noexcept {
    graphics_initialized = true;
    ImGui::Begin("ALR Chunks");

    const char* filter_label = "ID Filter";
    ImGuiInputTextFlags flags = ImGuiInputTextFlags_CharsHexadecimal | ImGuiInputTextFlags_AutoSelectAll;
    if (!chunk_filter.has_value()) {
        // Make the filter look empty
        flags |= ImGuiInputTextFlags_DisplayEmptyRefVal;
    }

    int val = this->chunk_filter.has_value() ? this->chunk_filter.value() : 0;
    const u32 step = 1;
    ImGui::InputInt(filter_label, &val, 1, 1, flags);
    // Input functions don't have good support for optionals where 0 is a valid
    // value, so we just assume a value of 0 is intended if it loses focus
    if (ImGui::IsItemEdited() || ImGui::IsItemDeactivated()) {
        // The value was edited, update it
        this->chunk_filter = val;
    }
    if (ImGui::Button("Clear filter")) {
        // Set to empty value
        this->chunk_filter = std::optional<u32>();
    }

    if (ImGui::BeginTable("alr chunks", 4, ImGuiTableFlags_ScrollY | ImGuiTableFlags_Reorderable)) {
        // Make header row that never scrolls away
        ImGui::TableSetupScrollFreeze(0, 1);

        // Setup table header
        ImGui::TableSetupColumn("ID");
        ImGui::TableSetupColumn("Offset");
        ImGui::TableSetupColumn("Size");
        ImGui::TableSetupColumn("Index");
        ImGui::TableHeadersRow();

        // Draw a row for each chunk. It'd be nice to use an ImGui::Clipper
        // here, but it triggers asserts in debug mode when there's an active
        // filter and we skip drawing some chunks.
        for (u32 i = 0; i < alr.chunks.size(); i++) {
            file::chunk& chunk = alr.chunks[i];

            if (chunk_filter.has_value()) {
                if (chunk_filter.value() != chunk.id) {
                    // Only show chunks that match the ID filter
                    continue;
                }
            }

            ImGui::TableNextRow();
            ImGui::TableSetColumnIndex(0);
            ImGui::Text("0x%X %s", chunk.id, chunk_name_by_id(chunk.id));

            // Selectable needs a unique ID, so we use the offset as the
            // selectable column because it's unique
            ImGui::TableSetColumnIndex(1);
            char buf[0x10] = {0};
            snprintf(buf, sizeof(buf), "0x%02llX", chunk.offset);
            // The extra flag makes the selection highlight go across the whole table
            const u32 select_flags = ImGuiSelectableFlags_SpanAllColumns;
            window_state* state = nullptr;
            for (window_state& s : states) {
                if (s.chunk_idx == i) {
                    state = &s;
                }
            }
            const bool is_active = (state) ? state->active : false;

            if (ImGui::Selectable(buf, is_active, select_flags)) {
                if (state) {
                    // Display chunk window
                    state->active = !state->active;
                } else {
                    states.emplace_back(i, chunk.id);
                }
            }

            ImGui::TableSetColumnIndex(2);
            ImGui::Text("0x%X", chunk.size);

           ImGui::TableSetColumnIndex(3);
           ImGui::Text("%d", i);
        }
        ImGui::EndTable();
    }
    ImGui::End();


    // Draw window for all chunks being displayed right now
    for (window_state& state : states) {
        file::chunk& chunk = alr.chunks[state.chunk_idx];
        if (!state.active) {
            continue;
        }

        const char* known_name = chunk_name_by_id(chunk.id);
        char buf[0x30] = {0};
        // Each window needs a unique ID, but "##x" isn't shown
        snprintf(buf, sizeof(buf), "0x%X %s Chunk @ 0x%llX", chunk.id, known_name, chunk.offset);

        if (ImGui::Begin(buf, &state.active)) {
            state.update(*this);
        }

        ImGui::End();
    }

    tex_edit.draw(*this);


    if (ctx.editor_enabled) {
        ImGui::Begin("Render Settings", &ctx.editor_enabled);
        ImGui::SetNextItemWidth(ImGui::CharWidth(20));
        ImGui::InputU16("Selected Model", &ctx.selected_mesh);

        ctx.fbo.bind();
        if (ImGui::Checkbox("Wireframe", &ctx.wireframe)) {
            ctx.fbo.set_wireframe(ctx.wireframe);
        }

        if (ImGui::Checkbox("Back-face culling", &ctx.backface_cull)) {
            ctx.fbo.set_backface_cull(ctx.backface_cull);
        }
        ctx.fbo.unbind();

        if (ImGui::Checkbox("Visualize UVs", &ctx.render_texcoords)) {
            ctx.set_shader(ctx.render_texcoords ? ctx.uv_shader : ctx.diffuse_shader);
        }

        // TODO: Bring back normal visualization
        // ImGui::Checkbox("Visualize normals", &render_normals);

        // TODO: Bring back normal map rendering
        // ImGui::Checkbox("Use normal maps", &temp_force_disable_normals);

        ImGui::Checkbox("Render selection in wireframe", &ctx.wireframe_selection);
        ImGui::Checkbox("Do vertex-skinned skeletal animation", &ctx.render_skinning);

        const float checkWidth = ImGui::CalcTextSize("Selected Object\t \tRender").x;
        const float sliderWidth = ImGui::GetContentRegionAvail().x - checkWidth;
        ImGui::SetNextItemWidth(sliderWidth);
        ImGui::SliderInt("Selected Mesh", (int*)&selected_mesh, 0, meshes.size() - 1);
        ImGui::SameLine();

        alr::mesh& mesh = meshes[selected_mesh];
        ImGui::Checkbox("Render##1", &mesh.active);

        if (mesh.gl_vertbufs.empty()) {
            ImGui::Text("[no objects on this mesh]");
        } else {
            const u32 old_selection = selected_object;
            ImGui::SetNextItemWidth(sliderWidth);
            bool changed = ImGui::SliderInt("Selected Object", (int *) &selected_object, 0, mesh.gl_vertbufs.size() - 1);
            selected_object = CLAMP(0, selected_object, mesh.gl_vertbufs.size() - 1);

            vertex_buffer &vertbuf = mesh.gl_vertbufs[selected_object];
            if (changed && ctx.wireframe_selection) {
                mesh.gl_vertbufs[old_selection].wireframe = false;
                vertbuf.wireframe = true;
            }

            ImGui::SameLine();
            ImGui::Checkbox("Render##2", &vertbuf.active);

            if (ImGui::CollapsingHeader("Object properties")) {
                vertbuf.edit_menu();
            }
        }
        ImGui::End();
    }

    // Do raycasting
    if (ctx.click_ray.has_value()) {
        bool hit = false;
        for (const mesh_instance& mesh : instances) {
            hit |= mesh.raycast(alr.data, *ctx.click_ray);
        }

        if (hit) {
            const vec3s dir = ctx.click_ray->dir;
            const vec3s pos = ctx.click_ray->origin;
        }
    }
}

void editor::render(render_context& ctx) noexcept {
    if (!ctx.visible) {
        return;
    }
    ctx.bind();
    for (alr::mesh_instance& instance : instances) {
        instance.update_animation(alr, ctx.anim_id, ImGui::GetIO().DeltaTime);
        instance.update_skinning(alr, ctx.anim_id, ImGui::GetIO().DeltaTime);
        instance.render(tex_manager, alr, ctx);
    }
    ctx.unbind();
}

bool editor::load(const char* path) noexcept {
    bool result = alr.load(path);
    tex_manager.destroy(); // Clear texture cache
    tex_manager.texheader_offset = alr.first_chunk_by_id(ALR_ID_TEXTURE).offset;

    states.clear(); // UI state doesn't transfer between files

    if (graphics_initialized) {
        load_all_meshes();
    }
    return result;
}

} // namespace al
