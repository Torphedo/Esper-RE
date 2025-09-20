// Need this define to use operators on ImGui vector types
#define IMGUI_DEFINE_MATH_OPERATORS
#include "editor_alr.hxx"
#include <imgui_internal.h>
#include <nfd.h>

#include <common/file.h>
#include <common/vfile.h>
#include <common/vmem.h>
#include <common/logging.h>
#include <common/crc32.h>

#include <formats/pd_common.h>
#include <formats/alr.h>

#include "alr_texture.hxx"
#include "alr_dump.hxx"
#include "pd_mesh.hxx"

#include "imgui_utils.hxx"
#include "validation.hxx"
#include "alr_dump.hxx"
#include "alr_imgui.hxx"

// Normally I'd make this a method, but by using a macro we can have LOG_MSG()
// automatically log the name of the method that shouldn't have been called.
#define CHUNK_ID_ASSERT(expected_id) \
do {                                 \
    if (id != expected_id) {         \
        LOG_MSG(warning, "called on 0x%x chunk @ 0x%x, when it only makes sense for 0x%x chunks!\n", id, offset, expected_id);\
        return;                      \
    }                                \
} while(0)

namespace al {

void resource::chunk::chunk_0x1(const resource& alr, viewport_t& viewport) noexcept {
    CHUNK_ID_ASSERT(0x1);

    vfile vf = vfile_open(alr.data, alr.alr_size);
    vf.pos = this->offset;
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

void resource::chunk::chunk_0x2(const resource& alr, viewport_t& viewport) noexcept {
    CHUNK_ID_ASSERT(0x2);

    if (ImGui::Button("Export to OBJ")) {
        // Display the file picker
        nfdu8filteritem_t filters[] = { { "3D Model", "obj"} };
        char* path = nullptr;
        nfdresult_t result = NFD_SaveDialogU8(&path, filters, ARRAY_SIZE(filters), nullptr, nullptr);
        if (result == NFD_OKAY && path != nullptr) {
            FILE* out = fopen(path, "ab");
            if (out) {
                al::dump_idx_buf(alr.data, offset, out, false);
                fclose(out);
            }
        }
        free(path);
    }

    // Index buffer editing
    vfile vf = vfile_open(alr.data + this->offset, this->size);
    chunk_generic chunk = VFILE_READ(chunk_generic, &vf);
    // We get the header pointer so we can modify it in-place
    idxbuf_header* header = (idxbuf_header*)vfile_cur(vf);
    vfile_seek(&vf, sizeof(*header)); // Skip past the header

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
            u16* idx = (u16*)vfile_cur(vf);
            vfile_seek(&vf, sizeof(*idx));
            ImGui::InputU16(label, idx);
        }
    }

    ImGui::PopItemWidth();
}

void resource::chunk::chunk_0x3(const resource& alr, viewport_t& viewport) noexcept {
    CHUNK_ID_ASSERT(0x3);

    vfile vf = vfile_open(alr.data + offset, size);
    vfile_seek(&vf, sizeof(chunk_generic)); // Skip ID & size

    const u32 num_joints = (size - sizeof(chunk_generic) - sizeof(chunk_armature)) / sizeof(joint_t);
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
    al::edit_joint_t(joint, vf, hex_edit);
}

void resource::chunk::chunk_0x5(const resource& alr, viewport_t& viewport) noexcept {
    vfile vf = vfile_open(alr.data + offset, size);
    anim_header* header = (anim_header*)vfile_cur(vf);
    vfile_seek(&vf, sizeof(*header));

    ImGui::Text("Length: %.3f frames", header->length);
    ImGui::Text("%d translation keys, 0x%X bytes each", header->translation_key_count, header->translation_key_size);
    ImGui::Text("%d rotation keys, 0x%X bytes each", header->rotation_key_count, header->rotation_key_size);
    ImGui::Text("%d scale keys", header->scale_key_count);

    // Edit and skip to the next set of keys
    if (header->translation_key_count > 0 && ImGui::CollapsingHeader("Translation Keys")) {
        edit_keyframes(header->translation_key_size, header->translation_key_count, vfile_cur(vf), "trans");
    }
    vfile_seek(&vf, header->translation_key_size * header->translation_key_count);

    if (header->rotation_key_count > 0 && ImGui::CollapsingHeader("Rotation Keys")) {
        edit_keyframes(header->rotation_key_size, header->rotation_key_count, vfile_cur(vf), "rot");
    }
    vfile_seek(&vf, header->rotation_key_size * header->rotation_key_count);
}

void resource::chunk::chunk_0x7(const resource& alr, viewport_t& viewport) noexcept {
    // This is the same as normal animation frames, but seems to ignore the
    // existing keyframe size fields.
    vfile vf = vfile_open(alr.data + offset, size);
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

void resource::chunk::chunk_0x10(resource& alr, viewport_t& viewport) noexcept {
    CHUNK_ID_ASSERT(0x10);

    vfile vf = vfile_open(alr.data + offset, size);
    // Skip over the ID and size fields we already have
    vfile_seek(&vf, sizeof(chunk_generic));

    auto* header = (atlas_header*) vfile_cur(vf);
    vfile_seek(&vf, sizeof(*header));

    auto* atlas_names = (atlas_name*) vfile_cur(vf);
    vfile_seek(&vf, sizeof(*atlas_names) * header->atlas_count);

    auto* atlases = (atlas_entry*) vfile_cur(vf);
    vfile_seek(&vf, sizeof(*atlases) * header->atlas_count);

    auto* textures = (atlas_tex_entry *) vfile_cur(vf);
    vfile_seek(&vf, sizeof(*textures) * header->texture_count);

    // We have to look up texture entries to find out where each texture is
    texture_entry* entries = nullptr;
    resource::chunk c = alr.first_chunk_by_id(0x15);
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
    if (al::edit_atlas_entry(*atlas, *aName)) {
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

    al::edit_atlas_texture(*tex);
    ImGui::draw_image(window_0x10.gl_tex_id, tex->width, tex->height, &window_0x10.use_actual_size, &window_0x10.scale, "texture", uv0, uv1);
}

void resource::chunk::chunk_0x11(const resource& alr, viewport_t& viewport) const noexcept {
    CHUNK_ID_ASSERT(0x11);
    vfile vf = vfile_open(alr.data + offset, size);
    auto* layout = (chunk_layout*)vfile_cur(vf);

    al::edit_chunk_layout(*layout);
}

void resource::chunk::import_dds_0x15(const resource& alr, const char* path, u32 num_entries, texture_entry* entries) noexcept {
    CHUNK_ID_ASSERT(0x15);

    const texture_entry cur = entries[window_0x15.selected_texture];
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

    window_0x15.tex = image_buf_load(path, window_0x15.tex.data, tex_size);

    // The loaded image might not be an even power of 2, here we round to the
    // nearest one
    const u16 res = MAX(window_0x15.tex.width, window_0x15.tex.height);
    for (u8 i = 0; i < TEX_POWER_LIMIT; i++) {
        if (exponent(2, i) > res) {
            // This power is larger than the largest target resolution
            break;
        }
        // Save the current power of 2 back to the ALR
        entries[window_0x15.selected_texture].resolution_pwr = i;
    }

    // Update our visual dimensions to match the new ALR value
    const u8 power = entries[window_0x15.selected_texture].resolution_pwr;
    window_0x15.tex.height = window_0x15.tex.width = exponent(2, power);
}

void resource::chunk::chunk_0x15(resource& alr, viewport_t& viewport) noexcept {
    CHUNK_ID_ASSERT(0x15);

    // We use the vfile API to handle the chunk data
    vfile vf = vfile_open(alr.data + offset, size);
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

    al::edit_texture_entry(entry);

    if (ImGui::Button("Import DDS")) {
        // Display the file picker
        nfdu8filteritem_t filters[] = { { "DDS Image", "dds"} };
        char* path = nullptr;
        nfdresult_t result = NFD_OpenDialogU8(&path, filters, ARRAY_SIZE(filters), nullptr);
        if (result == NFD_OKAY && path != nullptr) {
            this->import_dds_0x15(alr, path, num_entries, entries);
        }
        free(path);
    }

    window_0x15.tex = convert_tex(alr.resource_buffer(), entry);
    window_0x15.gl_tex_id = alr.tex_manager.get(alr, window_0x15.selected_texture);
    ImGui::SameLine();
    if (ImGui::Button("Export DDS")) {
        alr.tex_edit.tex_export_active = true;
        alr.tex_edit.export_cfg = window_0x15.tex;
        alr.tex_edit.export_cfg.data = (u8*)uintptr_t(entry.data_ptr);
        alr.tex_edit.export_tex_idx = window_0x15.selected_texture;
    }

    ImGui::draw_image(window_0x15.gl_tex_id, window_0x15.tex.width, window_0x15.tex.height, &window_0x15.use_actual_size, &window_0x15.scale, "preview");
    ImGui::EndGroup();
}

void resource::chunk::send_vertbuf_to_viewport(resource& alr, viewport_t& viewport) noexcept {
    CHUNK_ID_ASSERT(0x16);

    // We use the vfile API to handle the chunk data
    vfile vf = vfile_open(alr.data + offset, size);

    // Skip to entries
    vfile_seek(&vf, sizeof(chunk_generic));
    const u32 num_entries = VFILE_READ(u32, &vf);
    auto* entries = (vertbuf_entry*)vfile_cur(vf);

    const vertbuf_entry entry = entries[window_0x16.selected_vertex_buf];

    // Open ALR buffer
    vf = vfile_open(alr.data, alr.alr_size);

    // Jump to the appropriate data in the resource buffer
    vfile_seek(&vf, alr.resbuf_offset + entry.data_ptr);

    // Setup mesh data
    mesh_view mesh;
    mesh.setup();

    // Upload vertex buffer
    const u8* vertex_buf = (u8*)vfile_cur(vf);
    mesh.update_vertex_buf(vertex_buf, entry.vertex_size * entry.vertex_count);

    get_vert_attribute(&mesh, entry);
    mesh.apply_attributes();

    bool has_strips = false;
    const chunk_0x1_entry* texinfo_entries = nullptr;
    const joint_t* transform_entries = nullptr;
    // Upload the index buffers
    for (chunk c : alr.chunks) {
        if (c.offset < this->offset) {
            if (c.id == 0x1) {
                // Skip to chunk and get header
                vf.pos = c.offset;
                const chunk_0x1_header header = VFILE_READ(chunk_0x1_header, &vf);
                texinfo_entries = (chunk_0x1_entry *) vfile_cur(vf);
            }
            if (c.id == 0x3) {
                // Skip to chunk and get header
                vf.pos = c.offset;
                const auto genheader = VFILE_READ(chunk_generic, &vf);
                const chunk_armature header = VFILE_READ(chunk_armature, &vf);
                transform_entries = (joint_t*)vfile_cur(vf);
            }
        }

        if (c.id == this->id && c.offset > this->offset) {
            // We've hit a mesh metadata chunk past our own, so any
            // further index buffers will be garbage data to us. Quit.
            break;
        }

        // We only want index buffer chunks for the current mesh
        if (c.id == 0x2 && c.offset >= offset) {
            // Skip to chunk and get header
            vf.pos = c.offset + sizeof(chunk_generic);
            const idxbuf_header idx_header = VFILE_READ(idxbuf_header, &vf);
            // We only want index buffers meant for this vertex buffer
            if (idx_header.vertex_buf != window_0x16.selected_vertex_buf) {
                continue;
            }

            const bool tri_strip = (idx_header.primitive_type == IDX_TYPE_STRIP);
            has_strips |= tri_strip;
            const joint_t* joint = &transform_entries[idx_header.transform_idx];
            mat4s obj_transform = transform_from_joint(*joint);
            while (joint->parent_idx > 0) {
                joint = &transform_entries[joint->parent_idx];
                obj_transform = glms_mul(obj_transform, transform_from_joint(*joint));
            }

            const index_buffer idx_buf = {
                .idx_chunk_offset = u32(c.offset),
                .transform = obj_transform,
            };

            mesh.add_index_buf(alr.data, alr.alr_size, idx_buf);
        }
    }

    viewport.meshes.push_back(mesh);
}

void resource::chunk::chunk_0x16(resource& alr, viewport_t& viewport) noexcept {
    CHUNK_ID_ASSERT(0x16);

    // We use the vfile API to handle the chunk data
    vfile vf = vfile_open(alr.data + offset, size);
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
            al::dump_vertex_buf(alr, path, offset, window_0x16.selected_vertex_buf);
        }
        free(path);
    }

    if (ImGui::Button("Send to Viewport")) {
        this->send_vertbuf_to_viewport(alr, viewport);
    }

    al::edit_vertbuf_entry(*entry);
    ImGui::EndChild();

    ImGui::BeginChild("Vertex Buffer Hex Editor", ImVec2(800, 500));

    // Hex editor for vertex buffer data
    u8* vertbuf = alr.resource_buffer() + entry->data_ptr;
    window_0x16.hex_vertbuf.DrawContents(vertbuf, entry->vertex_count * entry->vertex_size);
    ImGui::EndChild();
}

void resource::chunk::draw(resource& alr, viewport_t& viewport) noexcept {
    if (alr.data == nullptr || alr.alr_size == 0) {
        // There's no data to work on, we can't display any useful data.
        return;
    }

    // Sanity check some of our assumptions & show warning messages if they fail
    std::string msg;

    const bool valid = alr_chunk_validate(alr, *this, msg, false);
    ImGui::PlsReportIf(msg.length() > 0, msg.c_str());

    if (ImGui::BeginTabBar("Chunk Tabs")) {
        if (ImGui::BeginTabItem("Specialized Chunk Editor")) {
            switch (id) {
                case 0x1:
                    this->chunk_0x1(alr, viewport);
                    break;
                case 0x2:
                    this->chunk_0x2(alr, viewport);
                    break;
                case 0x3:
                    this->chunk_0x3(alr, viewport);
                    break;
                case 0x5:
                    this->chunk_0x5(alr, viewport);
                    break;
                case 0x7:
                    this->chunk_0x7(alr, viewport);
                    break;
                case 0x10:
                    this->chunk_0x10(alr, viewport);
                    break;
                case 0x11:
                    this->chunk_0x11(alr, viewport);
                    break;
                case 0x15:
                    this->chunk_0x15(alr, viewport);
                    break;
                case 0x16:
                    this->chunk_0x16(alr, viewport);
                    break;
                default:
                    // Unimplemented window
                    ImGui::Text("[No special editor available]");
            }
            ImGui::EndTabItem();
        }

        if (ImGui::BeginTabItem("Raw Chunk Data")) {
            // Hex editor for the entire chunk, displayed with correct file offsets
            hex_chunk.DrawContents(alr.data + this->offset, this->size, this->offset);
            ImGui::EndTabItem();
        }
        ImGui::EndTabBar();
    }
}

resource::chunk::chunk(u32 id, s32 size, uintptr_t offset) noexcept {
    this->id = id;
    this->size = size;
    this->offset = offset;
    hex_edit.OptShowDataPreview = true;
    hex_chunk.OptShowDataPreview = true;

    // Because this is a union, we're not sure which constructors might be run
    // when, and other union members might set non-zero values to some fields.
    // To make sure the intended union member is correctly initialized, we use
    // this switch statement.
    switch (id) {
        case 0x2:
            window_0x2 = {};
            break;
        case 0x3:
            window_0x3 = {};
            hex_edit.PreviewDataType = ImGuiDataType_Float;
            break;
        case 0x5:
            window_0x5 = {};
            hex_edit.PreviewDataType = ImGuiDataType_Float;
            hex_chunk.PreviewDataType = ImGuiDataType_Float;
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
        default:
            return;
    }
}

void resource::tex_edit_state_t::draw(resource& alr) noexcept {
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

    vf.pos = offset_0x10 + sizeof(chunk_generic);
    const auto aHeader = VFILE_READ(atlas_header, &vf);
    vfile_seek(&vf, sizeof(atlas_name) * aHeader.atlas_count);
    const auto aEntries = (atlas_entry*)vfile_cur(vf);

    ImGui::Begin("Texture Editor", &tex_export_active);
    ImGui::PushItemWidth(ImGui::CharWidth() * 20);

    const u32 expected_offset = entries[export_tex_idx].data_ptr;
    const bool uninitialized = (expected_offset != (uintptr_t)export_cfg.data) && !override_buf;
    if (ImGui::InputU32("Texture index", &export_tex_idx) || uninitialized) {
        export_cfg.data = (u8 *) u64(entries[export_tex_idx].data_ptr);
    }

    ImGui::InputU16("Height", &export_cfg.height);
    ImGui::InputU16("Width", &export_cfg.width);
    ImGui::InputU16("Mipmap Level", &export_cfg.mip_level);

    ImGui::Checkbox("Compressed format", &export_cfg.compressed);
    if (!export_cfg.compressed) {
        ImGui::scope_indent indent;
        ImGui::InputU8("# Channels", &export_cfg.channels);
        // TODO: Update bobtail to get reasonable unit size
        ImGui::InputU8("Unit Size", &export_cfg.unit_size);
    }

    ImGui::Checkbox("Override buffer settings", &override_buf);
    if (override_buf) {
        ImGui::scope_indent indent;
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
    if (ImGui::CollapsingHeader("Preview")) {
        const ImVec2 size(export_cfg.width, export_cfg.height);
        ImGui::Image(alr.tex_manager.get(alr, export_tex_idx), size);
    }

    if (ImGui::Button("Export")) {
        decoded_text name = {};
        decode_single32(name.data, entries[export_tex_idx].text1);
        decode_single32(&name.data[6], entries[export_tex_idx].text2);

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

bool resource::load(const char* path) noexcept {
    if (!file_exists(path)) {
        LOG_MSG(error, "I couldn't find an ALR file named \"%s\".\n", path);
        return false;
    }

    const s64 size = file_size(path);
    if (size < 8) {
        // Smallest possible ALR chunk is 8 bytes
        LOG_MSG(error, "\"%s\" is only %d bytes, but an ALR must be at least 8 bytes.\n", path, size);
        return false;
    }

    // Expand reservation if needed
    if (size > reserve_size) {
        // If our reservation needs resizing, we're dealing with a
        // truly massive file. Just add its size to the old size,
        // more space can never hurt.
        this->expand_reservation(reserve_size + size);
    }

    // Load the file into the buffer.
    if (!file_load_existing(path, data, size)) {
        // Some loading failure, an error message should've been printed
        return false;
    }
    alr_size = size;
    chunks = shatter_alr(data, alr_size);
    tex_manager.material_header_offset = first_chunk_by_id(0x1).offset;
    tex_manager.atlasheader_offset = first_chunk_by_id(0x10).offset;
    tex_manager.texheader_offset = first_chunk_by_id(0x15).offset;
    return true;
}

bool resource::save(const char* path) const noexcept {
    FILE* out = fopen(path, "wb");
    if (out == nullptr) {
        return false;
    }

    bool result = true;
    if (fwrite(data, alr_size, 1, out) != 1) {
        // Incomplete write
        result = false;
    }
    fclose(out);

    return result;
}

std::vector<resource::chunk> resource::shatter_alr(const u8* buf, s64 size) noexcept {
    // Technically we cast away const here, but we don't write any data so it's
    // fine.
    vfile vf = vfile_open((void*)buf, size);
    std::vector<resource::chunk> out;

    // Loop until we exhaust the buffer or exit early
    u32 prev_id = -1;
    while (!vfile_eof(vf)) {
        // Read chunk data. We have to copy it over 1 field at a time because we
        // don't actually want/need any more of the chunk data.
        const uintptr_t offset = vf.pos; // It's important to save offset before reading
        const u32 id = VFILE_READ(u32, &vf);
        const s32 chunk_size = VFILE_READ(s32, &vf);
        resource::chunk chunk(id, chunk_size, offset);

        if (chunk.id == 0 && prev_id == 0) {
            // There's never multiple consecutive chunks with ID 0. This means
            // we've hit an empty area, probably the end of the chunk data.
            break;
        }

        if (chunk.id == 0x11) {
            // This chunk has the resource buffer offset, save it for later
            // Our layout structure includes the id & size, so we have to seek
            // back for that...
            vf.pos -= sizeof(chunk_generic);
            chunk_layout layout = VFILE_READ(chunk_layout, &vf);
            resbuf_offset = layout.texbuf_offset;

            // Reset the position to what it used to be
            vf.pos -= sizeof(layout);
        }

        // Advance to the next chunk & add to output
        vf.pos = chunk.offset + chunk.size;
        out.push_back(chunk);
        prev_id = chunk.id; // Save current ID
    }

    return out;
}

resource::chunk resource::first_chunk_by_id(u32 id) const noexcept {
    return first_chunk_in_range(id, 0, alr_size);
}

resource::chunk resource::prev_chunk_by_id(u32 id, u32 high, u32 low) const noexcept {
    assert(low < high && "Low bound must be < high bound!");
    for (s64 i = chunks.size() - 1; i > 0; i--) {
        const chunk& c = chunks[i];
        if (high < c.offset) {
            continue; // Skip to starting offset
        }
        if (c.offset < low) {
            break; // We passed the low bound
        }
        if (c.id == id) {
            return c; // Found it!
        }
    }

    return chunk(0, 0, 0); // Nothin...
}

resource::chunk resource::first_chunk_in_range(u32 id, u32 low, u32 high) const noexcept {
    // TODO: Add an overload to find a chunk within an offset range. Since the list is sorted we can do a sort of binary search by starting @ the middle
    assert(low < high && "Low bound must be < high bound!");

    for (const auto & c : chunks) {
        if (high < c.offset) {
            break;
        }
        if (c.offset < low) {
            continue;
        }
        if (c.id == id) {
            return c; // Found it!
        }
    }

    return chunk(0, 0, 0); // Nothin...
}

void resource::draw(viewport_t& viewport) noexcept {
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
        for (u32 i = 0; i < chunks.size(); i++) {
            resource::chunk &chunk = chunks[i];
            if (this->chunk_filter.has_value()) {
                if (this->chunk_filter.value() != chunk.id) {
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
            snprintf(buf, sizeof(buf), "0x%02lX", chunk.offset);
            // The extra flag makes the selection highlight go across the whole table
            const u32 select_flags = ImGuiSelectableFlags_SpanAllColumns;
            if (ImGui::Selectable(buf, chunk.active, select_flags)) {
                // Display chunk window
                chunk.active = !chunk.active;
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
    for (resource::chunk& chunk : chunks) {
        if (!chunk.active) {
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
        snprintf(buf, sizeof(buf), "0x%X %s Chunk @ 0x%lX", chunk.id, known_name, chunk.offset);

        if (ImGui::Begin(buf, &chunk.active)) {
            chunk.draw(*this, viewport);
        }

        ImGui::End();
    }

    tex_edit.draw(*this);
}

void resource::expand_reservation(s64 new_size) noexcept {
    if (new_size < reserve_size) {
        LOG_MSG(error, "No reason to shrink reservation from 0x%X -> 0x%X, ignoring!\n", reserve_size, new_size);
        return;
    }

    u8* new_buf = (u8*)vmem_reserve(new_size);
    if (new_buf == nullptr) {
        return;
    }

    // Commit the entire region. This ensures it's all accessible, but doesn't
    // comsume any physical memory until accessed. (May still create page file
    // entries)
    vmem_commit(new_buf, new_size);

    // Free old buffer and update our state
    if (data != nullptr) {
        vmem_free(data, reserve_size);
    }
    data = new_buf;
    reserve_size = new_size;
}

resource::resource() noexcept {
    // "Expand" our reservation from 0 bytes to... not 0.
    this->expand_reservation(reserve_size);
}

resource::~resource() noexcept {
    vmem_free(data, reserve_size);
}

} // namespace al
