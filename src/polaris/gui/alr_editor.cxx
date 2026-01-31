// Need this define to use operators on ImGui vector types
#define IMGUI_DEFINE_MATH_OPERATORS
#include "alr_editor.hxx"
#include <nfd.h>

#include <common/file.h>
#include <common/vfile.h>
#include <common/vmem.h>
#include <common/logging.h>
#include <common/crc32.h>

#include <formats/pd_common.h>
#include <formats/alr.h>

#include <validation.hxx>
#include <alr/alr_dump.hxx>
#include <util/imgui_utils.hxx>

#include "alr_assets.hxx"
#include "alr_imgui.hxx"
#include "mesh_view.hxx"
#include "viewport.hxx"

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

void editor::window_state::draw_chunk_0x1(const file& alr, file::chunk& chunk) noexcept {
    CHUNK_ID_ASSERT(0x1);

    vfile vf = vfile_open(alr.data, alr.alr_size);
    vf.pos = chunk.offset;
    const chunk_0x1_header header = VFILE_READ(chunk_0x1_header, &vf);
    auto entries = (chunk_0x1_entry*) vfile_cur(vf);

    ImGui::BeginChild("Entries", ImVec2(300, 0));
    for (u32 i = 0; i < header.num_entries; i++) {
        char buf[0x30] = {0};
        snprintf(buf, sizeof(buf) - 1, "Entry #%d", i);

        const bool is_selected = window_0x1.selected_entry == i;
        if (ImGui::Selectable(buf, is_selected)) {
            window_0x1.selected_entry = i;
        }
    }

    ImGui::EndChild();
    ImGui::SameLine();

    chunk_0x1_entry* entry = &entries[window_0x1.selected_entry];
    // Explicit constructor
    hex_chunk.DrawContents(entry, sizeof(*entry), (uintptr_t)entry - (uintptr_t)alr.data);
}

void editor::window_state::draw_chunk_0x2(file& alr, file::chunk& chunk) noexcept {
    CHUNK_ID_ASSERT(0x2);

    if (ImGui::Button("Shift From Here")) {
        alr.shift_chunks(chunk.offset, 0x100);
    }

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

void editor::window_state::draw_chunk_0x3(const file& alr, file::chunk& chunk) noexcept {
    CHUNK_ID_ASSERT(0x3);

    vfile vf = vfile_open(alr.data + chunk.offset, chunk.size);

    const u32 num_joints = (chunk.size - sizeof(chunk_armature)) / sizeof(joint_t);
    const chunk_armature header = VFILE_READ(chunk_armature, &vf);
    auto* joints = (joint_t *) vfile_cur(vf);

    ImGui::Text("%d joints [%d identity]", num_joints, num_joints - header.joint_count);

    const u32 min = 0;
    const u32 max = MAX(num_joints - 1, 0);
    ImGui::Checkbox("Use slider", &window_0x3.slider);
    const char* inputlabel = "Selected Joint";
    if (window_0x3.slider) {
        ImGui::SliderScalar(inputlabel, ImGuiDataType_S32, &window_0x3.selected_joint, &min, &max);
    } else {
        ImGui::InputScalar(inputlabel, ImGuiDataType_S32, &window_0x3.selected_joint);
    }
    // Don't allow out of bounds index
    window_0x3.selected_joint = CLAMP(min, window_0x3.selected_joint, max);

    joint_t& joint = joints[window_0x3.selected_joint];
    alr::edit_joint_t(joint, vf, hex_edit);
}

void editor::window_state::draw_chunk_0x5(const file& alr, file::chunk& chunk) noexcept {
    vfile vf = vfile_open(alr.data + chunk.offset, chunk.size);
    anim_header* header = (anim_header*)vfile_cur(vf);
    vfile_seek(&vf, sizeof(*header));

    if (ImGui::Button("Dump animation")) {
        file::chunk armature_chunk = alr.first_chunk_in_range(0x3, chunk.offset, alr.resbuf_offset);
        vfile armature_vf = vfile_open(alr.data + armature_chunk.offset, armature_chunk.size);
        const auto armature_header = VFILE_READ(chunk_armature, &armature_vf);
        const auto* joints = (joint_t*)vfile_cur(armature_vf);

        const u32 joint_idx = header->joint_idx;
        const joint_t joint = joints[joint_idx];

        decoded_text decoded = {};
        decode_single32(decoded.data, joint.name);
        std::string joint_name = std::string(decoded.data) + "_" + std::to_string(joint_idx);

        dump_animation_maya(header, "file.anim", joint_name.c_str());
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

void editor::window_state::draw_chunk_0x7(const file& alr, file::chunk& chunk) noexcept {
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

void editor::window_state::draw_chunk_0x10(file& alr, file::chunk& chunk) noexcept {
    CHUNK_ID_ASSERT(0x10);

    vfile vf = vfile_open(alr.data + chunk.offset, chunk.size);

    auto* header = VFILE_READ_PTR(atlas_header, &vf);
    auto* atlas_names = (atlas_name*)vfile_cur(vf);
    vfile_seek(&vf, sizeof(*atlas_names) * header->atlas_count);

    auto* atlases = (atlas_entry*)vfile_cur(vf);
    vfile_seek(&vf, sizeof(*atlases) * header->atlas_count);

    auto* textures = VFILE_READ_PTR(atlas_tex_entry , &vf);

    // We have to look up texture entries to find out where each texture is
    texture_entry* entries = nullptr;
    file::chunk c = alr.first_chunk_by_id(0x15);
    if (c.size == 0) {
        // This should never happen
        ImGui::PlsReportIf(true, "Couldn't find an 0x15 chunk!\n");
        return;
    } else {
        // Skip to the chunk
        vfile tmp = vfile_open(alr.data + c.offset, c.size);
        vfile_seek(&tmp, sizeof(chunk_generic));

        const u32 num_entries = VFILE_READ(u32, &tmp);
        entries = (texture_entry*)vfile_cur(tmp);
    }


    ImGui::BeginChildFitContent("Atlases", 0.3f);
    ImGui::Text("%d Atlases for %.*s:", header->atlas_count, (int)sizeof(header->alr_name), header->alr_name);
    for (u32 i = 0; i < header->atlas_count; i++) {
        char buf[sizeof(atlas_names[i].name) + 0x20] = {0};
        snprintf(buf, sizeof(buf) - 1, "%s##%d", atlas_names[i].name, i);

        if (ImGui::Selectable(buf, window_0x10.selected_atlas == i)) {
            window_0x10.selected_atlas = i;
        }
    }
    ImGui::EndChild();
    ImGui::SameLine();


    // The currently selected texture might be in a different atlas, which would
    // display a strange & unintuitive result. We find the first and last index
    // of textures in the atlas, and make sure the selected texture is always
    // a child of the selected atlas.
    if (textures[window_0x10.selected_atlas_texture].index != window_0x10.selected_atlas) {
        for (u32 i = 0; i < header->texture_count; i++) {
            if (textures[i].index == window_0x10.selected_atlas) {
                // It's a match!
                window_0x10.selected_atlas_texture = i;
                break;
            }
        }
    }

    // Display textures in the selected atlases
    ImGui::BeginChildFitContent("Textures", 0.3f);
    for (u32 i = 0; i < header->texture_count; i++) {
        const atlas_tex_entry tex = textures[i];
        // Only list textures belonging to the selected atlases
        if (tex.index != window_0x10.selected_atlas) {
            continue;
        }

        char buf[sizeof(textures[i].filename) + 0x20] = {0};
        snprintf(buf, sizeof(buf) - 1, "%s##%d", tex.filename, i);

        if (ImGui::Selectable(buf, window_0x10.selected_atlas_texture == i)) {
            window_0x10.selected_atlas_texture = i;
        }
    }
    ImGui::EndChild();

    atlas_name* aName = &atlas_names[window_0x10.selected_atlas];
    atlas_entry* atlas = &atlases[window_0x10.selected_atlas];
    atlas_tex_entry* tex = &textures[window_0x10.selected_atlas_texture];
    texture cur_tex = convert_tex(alr.resource_buffer(), entries[tex->index]);

    // Override dimensions, we only want format info from the other chunk
    cur_tex.height = atlas->height;
    cur_tex.width = atlas->width;
    ImGui::Text("Atlas uses texture index %d, see 0x15 chunk for offset & format", window_0x10.selected_atlas);
    if (alr::edit_atlas_entry(*atlas, *aName)) {
        // Texture settings have changed, trigger reload
        alr.tex_manager.invalidate(window_0x10.selected_atlas);
    }

    window_0x10.gl_tex_id = alr.tex_manager.get(alr, window_0x10.selected_atlas);
    window_0x10.tex = cur_tex;

    // Draw the whole atlas
    ImVec2 image_pos = ImGui::draw_image(window_0x10.gl_tex_id, atlas->width, atlas->height, &window_0x10.use_actual_size_atlas, &window_0x10.scale_atlas, "atlas");

    // Calculate UVs of the selected texture in the atlas
    const ImVec2 uv1 = ImVec2(tex->atlas_texcoords[0], tex->atlas_texcoords[1]);
    const ImVec2 uv0 = ImVec2(uv1.x - ((float)tex->width / atlas->width), uv1.y - ((float)tex->height / atlas->height));

    // Draw a bounding box over a single texture in the atlas
    const ImVec2 atlas_drawn_size = ImVec2(atlas->width, atlas->height) * window_0x10.scale_atlas;
    const ImVec2 start = image_pos + (atlas_drawn_size * uv0);
    const ImVec2 end = image_pos + (atlas_drawn_size * uv1);
    ImGui::GetWindowDrawList()->AddRect(start, end, 0xFF00FF00);

    alr::edit_atlas_texture(*tex);
    ImGui::draw_image(window_0x10.gl_tex_id, tex->width, tex->height, &window_0x10.use_actual_size, &window_0x10.scale, "texture", uv0, uv1);
}

void editor::window_state::draw_chunk_0x11(const file& alr, file::chunk& chunk) const noexcept {
    CHUNK_ID_ASSERT(0x11);
    vfile vf = vfile_open(alr.data + chunk.offset, chunk.size);
    auto* layout = (chunk_layout*)vfile_cur(vf);

    alr::edit_chunk_layout(*layout);
}

void editor::window_state::import_dds_0x15(file& alr, const char* path, u32 num_entries, texture_entry* entries) noexcept {
    const file::chunk chunk = alr.chunks[chunk_idx];
    CHUNK_ID_ASSERT(0x15);

    texture_entry cur = entries[window_0x15.selected_texture];
    const texture_entry next = entries[window_0x15.selected_texture + 1];
    s64 tex_size = 0;
    if (window_0x15.selected_texture >= num_entries) {
        // This is the last entry, so the best guess is that it takes up the
        // rest of the file
        tex_size = alr.alr_size - cur.data_ptr;
    } else {
        // The most likely texture size is the distance betwen this texture and
        // the next
        tex_size = next.data_ptr - cur.data_ptr;
    }


    // Load the texture data
    u8* data = alr.resource_buffer() + cur.data_ptr;
    texture tex = image_buf_load(path, data, tex_size);

    // Update the ALR state and force a reload
    alr_texture_set_dimensions(&cur, tex.height, tex.width);
    alr.tex_manager.invalidate(window_0x15.selected_texture);
}

void editor::window_state::draw_chunk_0x15(editor& ed, file::chunk& chunk) noexcept {
    CHUNK_ID_ASSERT(0x15);
    file& alr = ed.alr;

    // We use the vfile API to handle the chunk data
    vfile vf = vfile_open(alr.data + chunk.offset, chunk.size);
    // Skip over the ID and size fields we already have
    vfile_seek(&vf, sizeof(chunk_generic));

    const u32 num_entries = VFILE_READ(u32, &vf);
    auto* entries = (texture_entry*)vfile_cur(vf);

    ImGui::BeginChildFitContent("Textures", 0.3f);
    for (u32 i = 0; i < num_entries; i++) {
        char buf[0x30] = {0};
        decoded_text name = {0};
        decode_single32(name.data, entries[i].text1);
        decode_single32(&name.data[ENCODED_CHAR_COUNT], entries[i].text2);

        snprintf(buf, sizeof(buf) - 1, "#%d \"%s\" @ resbuf+0x%X", i, name.data, entries[i].data_ptr);

        if (ImGui::Selectable(buf, window_0x15.selected_texture == i)) {
            window_0x15.selected_texture = i;
        }
    }
    ImGui::EndChild();
    ImGui::SameLine();

    ImGui::BeginGroup();
    texture_entry& entry = entries[window_0x15.selected_texture];

    if (alr::edit_texture_entry(entry)) {
        alr.tex_manager.invalidate(window_0x15.selected_texture);
    }

    if (ImGui::Button("Import DDS")) {
        // Display the file picker
        nfdu8filteritem_t filters[] = { { "DDS Image", "dds"} };
        char* path = nullptr;
        nfdresult_t result = NFD_OpenDialogU8(&path, filters, ARRAY_SIZE(filters), nullptr);
        if (result == NFD_OKAY && path != nullptr) {
            import_dds_0x15(alr, path, num_entries, entries);
        }
        free(path);
    }

    window_0x15.gl_tex_id = alr.tex_manager.get(alr, window_0x15.selected_texture);
    ImGui::SameLine();
    if (ImGui::Button("Export DDS")) {
        ed.tex_edit.tex_export_active = true;
        ed.tex_edit.export_cfg = convert_tex(alr.resource_buffer(), entry);
        ed.tex_edit.export_cfg.data = (u8*)uintptr_t(entry.data_ptr);
        ed.tex_edit.export_tex_idx = window_0x15.selected_texture;
    }

    u16 height = 0;
    u16 width = 0;
    alr_texture_get_dimensions(entry, &height, &width);
    ImGui::draw_image(window_0x15.gl_tex_id, width, height, &window_0x15.use_actual_size, &window_0x15.scale, "preview");
    ImGui::EndGroup();
}

void editor::window_state::send_vertbuf_to_viewport(file& alr, viewport_t& viewport) noexcept {
    const file::chunk chunk = alr.chunks[chunk_idx];
    CHUNK_ID_ASSERT(0x16);

    // Find out what index we are
    s32 idx = -1;
    for (file::chunk c : alr.chunks) {
        if (c.offset > chunk.offset) {
            break;
        }
        if (c.id == 0x1) {
            idx++;
        }
    }

    mesh_view mesh = mesh_at_idx(alr, idx, window_0x16.selected_vertex_buf);
    viewport.meshes.push_back(mesh);
}

void editor::window_state::draw_chunk_0x16(file& alr, file::chunk& chunk, viewport_t& viewport) noexcept {
    CHUNK_ID_ASSERT(0x16);

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

        const bool is_selected = window_0x16.selected_vertex_buf == i;
        if (ImGui::Selectable(buf, is_selected)) {
            window_0x16.selected_vertex_buf = i;
        }
    }

    ImGui::EndChild();
    ImGui::SameLine();

    vertbuf_entry* entry = &entries[window_0x16.selected_vertex_buf];
    ImGui::BeginChild("Vertex Buffer Settings", ImVec2(600, 0));
    if (ImGui::Button("Dump to OBJ")) {
        // Display the file picker
        nfdu8filteritem_t filters[] = { { "3D Model", "obj"} };
        char* path = nullptr;
        nfdresult_t result = NFD_SaveDialogU8(&path, filters, ARRAY_SIZE(filters), nullptr, nullptr);
        if (result == NFD_OKAY && path != nullptr) {
            alr::dump_vertex_buf(alr, path, chunk.offset, window_0x16.selected_vertex_buf);
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
                obj_import(obj_data, alr, chunk.offset, window_0x16.selected_vertex_buf);
                free(obj_data);
            }
        }
        free(path);
    }

    if (ImGui::Button("Send to Viewport")) {
        send_vertbuf_to_viewport(alr, viewport);
    }

    if (ImGui::CollapsingHeader("Shift Buffer")) {
        ImGui::InputS32("Shift Amount", &window_0x16.shift_amount);
        if (ImGui::Button("Go!")) {
            alr.shift_vertbuf(entry->data_ptr, window_0x16.shift_amount);
        }
    }

    alr::edit_vertbuf_entry(*entry);
    ImGui::EndChild();

    ImGui::BeginChild("Vertex Buffer Hex Editor", ImVec2(800, 500));

    // Hex editor for vertex buffer data
    u8* vertbuf = alr.resource_buffer() + entry->data_ptr;
    window_0x16.hex_vertbuf.DrawContents(vertbuf, entry->vertex_count * entry->vertex_size);
    ImGui::EndChild();
}

void editor::window_state::draw(editor& ed, viewport_t& viewport) noexcept {
    if (!ed.alr.data || ed.alr.alr_size == 0) {
        // There's no data to work on, we can't display any useful data.
        return;
    }

    file::chunk& chunk = ed.alr.chunks[chunk_idx];

    // Sanity check some of our assumptions & show warning messages if they fail
    std::string msg;

    const bool valid = alr_chunk_validate(ed.alr, chunk, msg, false);
    ImGui::PlsReportIf(msg.length() > 0, msg.c_str());

    if (ImGui::BeginTabBar("Chunk Tabs")) {
        if (ImGui::BeginTabItem("Specialized Chunk Editor")) {
            switch (chunk.id) {
                case 0x1:
                    draw_chunk_0x1(ed.alr, chunk);
                    break;
                case 0x2:
                    draw_chunk_0x2(ed.alr, chunk);
                    break;
                case 0x3:
                    draw_chunk_0x3(ed.alr, chunk);
                    break;
                case 0x5:
                    draw_chunk_0x5(ed.alr, chunk);
                    break;
                case 0x7:
                    draw_chunk_0x7(ed.alr, chunk);
                    break;
                case 0x10:
                    draw_chunk_0x10(ed.alr, chunk);
                    break;
                case 0x11:
                    draw_chunk_0x11(ed.alr, chunk);
                    break;
                case 0x15:
                    draw_chunk_0x15(ed, chunk);
                    break;
                case 0x16:
                    draw_chunk_0x16(ed.alr, chunk, viewport);
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
    case 0x1:
        window_0x1 = {};
        break;
    case 0x3:
        window_0x3 = {};
        break;
    case 0x10:
        window_0x10 = {};
        break;
    case 0x15:
        window_0x15 = {};
        break;
    case 0x16:
        window_0x16 = {};
        break;
    }
}

void editor::tex_edit_state_t::draw(file& alr) noexcept {
    if (!tex_export_active) {
        return;
    }

    if (!offset_0x10) {
        offset_0x10 = alr.first_chunk_by_id(0x10).offset;
    }
    if (!offset_0x15) {
        offset_0x15 = alr.first_chunk_by_id(0x15).offset;
    }

    const u32 hash = crc32fast((u8*)this, sizeof(*this));

    vfile vf = vfile_open(alr.data, alr.alr_size);
    vf.pos = offset_0x15;
    const auto tex_header = VFILE_READ(texture_header, &vf);
    const texture_entry* entries = (texture_entry*)vfile_cur(vf);

    vf.pos = offset_0x10;
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
        alr.tex_manager.invalidate(export_tex_idx);
        gl_obj tex_id = 0;
        glGenTextures(1, &tex_id);
        if (tex_id != 0) {
            // Need to turn offset into pointer while uploading texture
            export_cfg.data += (uintptr_t)alr.resource_buffer();
            update_gl_tex(export_cfg, tex_id);
            export_cfg.data -= (uintptr_t)alr.resource_buffer();
            alr.tex_manager.gl_tex_map[export_tex_idx] = tex_id;
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
        ImGui::Image(alr.tex_manager.get(alr, export_tex_idx), size);
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


bool editor::load(const char* path, viewport_t& viewport) noexcept {
    bool result = alr.load(path);
    states.clear(); // UI state doesn't transfer between files

    if (graphics_initialized) {
        send_all_to_viewport(viewport);
    }
    return result;
}

void editor::send_all_to_viewport(viewport_t& viewport) const noexcept {
    u32 num_models = 0;
    alr.first_model_idx(&num_models);

    // Stage ALRs have a base, background, and skybox model.
    // Player ALRs have at most a base and detail (e.g. scarf) model.
    for (u32 i = 0; i < 3; i++) {
        alr_model_desc model = alr.model_at_idx(i);
        if (model.vert_chunk == nullptr) {
            continue;
        }
        for (u32 j = 0; j < model.vert_chunk->num_entries; j++) {
            if (model.vert_chunk->entries[j].vertex_size == 12) {
                continue;
            }
            mesh_view mesh = mesh_at_idx(alr, i, j);
            viewport.meshes.push_back(mesh);
        }
    }
}

void editor::draw(viewport_t& viewport) noexcept {
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
            ImGui::Text("0x%X", chunk.id);

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

        const char* known_name = "";
        switch (chunk.id) {
        case 0x2:
            known_name = "[Index Buffer]";
            break;
        case 0x3:
            known_name = "[Armature]";
            break;
        case 0x5:
            known_name = "[Animation]";
            break;
        case 0x7:
            known_name = "[Camera Path]";
            break;
        case 0x10:
            known_name = "[Texture Atlas]";
            break;
        case 0x11:
            known_name = "[Header]";
            break;
        case 0x15:
            known_name = "[Texture Metadata]";
            break;
        case 0x16:
            known_name = "[Vertex Metadata]";
            break;
        }
        char buf[0x30] = {0};
        // Each window needs a unique ID, but "##x" isn't shown
        snprintf(buf, sizeof(buf), "0x%X %s Chunk @ 0x%llX", chunk.id, known_name, chunk.offset);

        if (ImGui::Begin(buf, &state.active)) {
            state.draw(*this, viewport);
        }

        ImGui::End();
    }

    tex_edit.draw(alr);
}

} // namespace al
