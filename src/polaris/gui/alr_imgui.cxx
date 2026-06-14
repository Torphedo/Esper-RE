#include "alr_imgui.hxx"
#include <string>
#include <nfd.h>

#include <common/crc32.h>

#include <alr/alr_dump.hxx>
#include "alr_assets.hxx"
#include "alr_editor.hxx"
#include <util/imgui_utils.hxx>
#include <util/utils.hxx>

namespace alr {
    bool edit_chunk_layout(chunk_layout& layout, editor& ed) {
        file& alr = ed.alr;
        const u32 size = sizeof(layout) + layout.offset_array_size * sizeof(*layout.offsets);
        const u32 hash = crc32fast((u8*)&layout, size);
        const int hex_flags = ImGuiInputTextFlags_CharsHexadecimal;

        ImGui::InputU32("Resource Buffer Offset", &layout.texbuf_offset, hex_flags);
        ImGui::InputU32("Resource Buffer Size", &layout.texbuf_size, hex_flags);

        if (ImGui::InputU32("Chunk Offset Count", &layout.offset_array_size)) {
            const ptrdiff_t offset = (uintptr_t)&layout - (uintptr_t)alr.data;
            const s32 new_size = sizeof(layout) + layout.offset_array_size * sizeof(*layout.offsets);
            alr.set_chunk_size(offset, new_size);
            ed.reload_meshes();
        }

        if (ImGui::CollapsingHeader("Offsets")) {
            for (u32 i = 0; i < layout.offset_array_size; i++) {
                std::string label;
                str_format_append(label, "##%d", i);
                ImGui::InputS32(label.c_str(), &layout.offsets[i], hex_flags);
            }
        }

        return (hash != crc32fast((u8*)&layout, sizeof(layout)));
    }

    bool edit_material_entry(material_entry& entry) {
        const u32 hash = crc32fast((u8*)&entry, sizeof(entry));
        ImGui::PushItemWidth(ImGui::CharWidth() * 20);

        ImGui::InputPDString("Shader Name", &entry.text1, &entry.text2);
        ImGui::InputU8("Vertex Format ID", &entry.vertbuf_format);
        ImGui::InputU8("Vertex Format Size", &entry.vert_size);
        ImGui::InputU8("Index", &entry.entry_idx);
        ImGui::InputU16("Diffuse Texture ID", &entry.texture_idx);

        const char* normal_label = "Normal Map Texture ID";
        if (entry.vertbuf_format == ALR_VERTFMT_LBTS) {
            normal_label = "Baked Lightmap Texture ID";
        }
        ImGui::InputU16(normal_label, &entry.normal_idx);

        if (entry.vertbuf_format == ALR_VERTFMT_LBTS) {
            ImGui::InputU16("Normal Map Texture ID", &entry.normal_backup_idx);
        }

        const u32 newhash = crc32fast((u8*)&entry, sizeof(entry));
        return (hash != newhash);
    }

    bool edit_texture_entry(texture_entry& entry) {
        const u32 hash = crc32fast((u8*)&entry, sizeof(entry));

        alr_pixel_format pixel_fmt_table[] = {
            FORMAT_R8, FORMAT_R8_2, FORMAT_A8,
            FORMAT_BGRA_5551, FORMAT_BGRA_4444, FORMAT_BGR_565,
            FORMAT_RGBA8, FORMAT_RGBA8_2,
            FORMAT_DXT1, FORMAT_DXT3, FORMAT_DXT5,
        };

        ImGui::PushItemWidth(ImGui::CharWidth() * 35);

        ImGui::InputPDString("Name", &entry.text1, &entry.text2);
        ImGui::InputU32("Resource Buffer Offset", &entry.data_ptr);

        std::string label;
        str_format_append(label, "%s (0x%X)", texformat_str((alr_pixel_format)entry.pixel_format), entry.pixel_format);

        if (ImGui::BeginCombo("Pixel format", label.c_str())) {
            for (alr_pixel_format fmt : pixel_fmt_table) {
                label = "";
                str_format_append(label, "%s (0x%X)", texformat_str(fmt), fmt);

                bool selected = (fmt == entry.pixel_format);
                if (ImGui::Selectable(label.c_str(), &selected)) {
                    entry.pixel_format = fmt;
                }
            }
            ImGui::EndCombo();
        }

        u16 height = 0;
        u16 width = 0;
        alr_texture_get_dimensions(entry, &height, &width);

        bool dimensions_changed = false;
        dimensions_changed |= ImGui::InputU16("Height", &height);
        dimensions_changed |= ImGui::InputU16("Width", &width);
        if (dimensions_changed) {
            alr_texture_set_dimensions(&entry, height, width);
        }

        // These are bitfields so we can't edit them directly
        u8 mip_count = entry.mipmap_count;
        bool cubemap = entry.is_cubemap;
        if (ImGui::InputU8("Mipmap Count", &mip_count)) {
            entry.mipmap_count = mip_count;
        }

        if (ImGui::Checkbox("Cubemap", &cubemap)) {
            entry.is_cubemap = cubemap;
        }
        ImGui::PopItemWidth();

        return (hash != crc32fast((u8*)&entry, sizeof(entry)));
    }

    bool edit_atlas_entry(atlas_entry& entry, atlas_name& name_entry) {
        const u32 hash = crc32fast((u8*)&entry, sizeof(entry));

        ImGui::PushItemWidth(ImGui::CharWidth() * sizeof(name_entry.name));
        ImGui::InputText("Atlas Name", name_entry.name, sizeof(name_entry.name) - 1);
        ImGui::InputU16("Width", &entry.width);
        ImGui::InputU16("Height", &entry.height);
        ImGui::InputU32("Texture Count", &entry.tex_count);

        if (ImGui::CollapsingHeader("Unknown Fields")) {
            ImGui::InputU32("Texture size", &entry.approx_size);
            ImGui::InputU32("Unknown 1", &entry.unknown);
            ImGui::InputU32("Padding", &entry.pad);

            ImGui::InputU32("Unknown Name Field 1", &name_entry.unk1);
            ImGui::InputU32("Unknown Name Field 2", &name_entry.unk2);
            ImGui::InputU32("Unknown Name Field 3", &name_entry.unk3);
            ImGui::InputU32("Unknown Name Field 4", &name_entry.unk4);
        }
        ImGui::PopItemWidth();

        return (hash != crc32fast((u8*)&entry, sizeof(entry)));
    }

    bool edit_atlas_texture(atlas_tex_entry& entry) {
        const u32 hash = crc32fast((u8*)&entry, sizeof(entry));
        ImGui::SetNextItemWidth(ImGui::CharWidth() * sizeof(entry.filename));

        ImGui::InputText("Texture Name", entry.filename, sizeof(entry.filename) - 1);

        ImGui::PushItemWidth(ImGui::CharWidth() * 16);
        ImGui::InputU32("Parent Atlas", &entry.index);
        ImGui::InputFloat2("Texture Coordinates (upper left)", entry.uv0);
        ImGui::InputFloat2("Texture Coordinates (bottom right)", entry.uv1);
        ImGui::InputU32("Texture Width", &entry.width);
        ImGui::InputU32("Texture Height", &entry.height);

        ImGui::PopItemWidth();
        return (hash != crc32fast((u8*)&entry, sizeof(entry)));
    }

    bool edit_vertbuf_entry(vertbuf_entry& entry) {
        const u32 hash = crc32fast((u8*)&entry, sizeof(entry));
        const int hex_flags = ImGuiInputTextFlags_CharsHexadecimal;
        ImGui::PushItemWidth(ImGui::CharWidth() * 16);

        ImGui::InputU8("Vertex Format", &entry.format, hex_flags);
        ImGui::InputU8("Vertex Size", &entry.vertex_size, hex_flags);
        ImGui::InputU8("Vertex Size 2", &entry.vertex_size2, hex_flags);
        ImGui::InputU32("# of Vertices", &entry.vertex_count, hex_flags);
        ImGui::InputU32("Resource Buffer Offset", &entry.data_ptr, hex_flags);

        if (ImGui::CollapsingHeader("Unknown Fields")) {
            ImGui::InputU8("Unknown 1", &entry.unknown1);
            ImGui::InputU32("Unknown 2", &entry.unknown2);
            ImGui::InputU32("Unknown 3", &entry.unknown3);
        }

        ImGui::PopItemWidth();

        return (hash != crc32fast((u8*)&entry, sizeof(entry)));
    }

    bool edit_joint_t(joint_t& joint, vfile armature_vf, MemoryEditor& hex_edit) {
        const u32 hash = crc32fast((u8*)&joint, sizeof(joint));

        ImGui::InputPDString("Joint Name", &joint.name);
        ImGui::Text("Parent index: %d", joint.parent_idx);
        ImGui::InputU16("Current Index", &joint.idx);
        ImGui::InputU8("Unknown 1", &joint.unk1);
        ImGui::InputU8("Unknown 2", &joint.unk2);
        if (ImGui::Button("Dump to file")) {
            // Display the file picker
            nfdu8filteritem_t filters[] = { { "COLLADA Skeleton", "dae"} };
            char* path = nullptr;
            nfdresult_t result = NFD_SaveDialogU8(&path, filters, ARRAY_SIZE(filters), nullptr, nullptr);
            if (result == NFD_OKAY && path != nullptr) {
                FILE* f = fopen(path, "ab");
                if (f) {
                    armature_vf.pos = 0;
                    dump_armature_dae(f, armature_vf);
                    fclose(f);
                }
            }
            free(path);
        }

        if (ImGui::BeginTabBar("editors")) {
            if (ImGui::BeginTabItem("Float editor")) {

                // Matrix inputs
                ImGui::PushItemWidth(400.0f); // Make inputs narrower
                ImGui::InputFloat3("Position", &joint.position.x);
                ImGui::InputFloat3("Euler Rotation", &joint.rotation.x);
                ImGui::InputFloat3("Scale", &joint.scale.x);

                ImGui::PopItemWidth();
                ImGui::EndTabItem();
            }

            if (ImGui::BeginTabItem("Hex Editor")) {
                // Show hex editor
                hex_edit.DrawContents(&joint, sizeof(joint));

                ImGui::EndTabItem();
            }

            ImGui::EndTabBar();
        }

        return (hash != crc32fast((u8*)&joint, sizeof(joint)));
    }

    void edit_keyframes(u16 key_size, u16 key_count, void* keyframes, const char* label_extra) {
        if (keyframes == nullptr || label_extra == nullptr) {
            ImGui::Text("Programmer error: %s() was passed a null value", __func__);
            return;
        }

        data_type frame_type = DATA_TYPE_COUNT;
        data_type component_type = DATA_TYPE_COUNT;
        u32 num_components = 0;
        anim_key_info(key_size, frame_type, component_type, num_components);
        const u32 frame_size = sizeof_type(frame_type);
        const u32 component_size = sizeof_type(component_type);

        if (num_components == 0 || component_type == DATA_TYPE_COUNT || frame_type == DATA_TYPE_COUNT) {
            // Something wasn't filled out, probably unknown format
            ImGui::Text("Unknown keyframe format (0x%X bytes)", key_size);
            return;
        }

        // Give our float inputs 12 characters width per component
        ImGui::PushItemWidth(ImGui::CharWidth() * num_components * 12);

        const ImGuiDataType imgui_frame_type = ImGui::type_table[frame_type];
        const ImGuiDataType imgui_component_type = ImGui::type_table[component_type];

        // Each keyframe has a frame value (when it happens) and components (for 3D
        // translation/rotation/scale, or weird stuff like brightness values).
        vfile vf = vfile_open(keyframes, key_count * key_size);
        for (u16 i = 0; i < key_count; i++) {
            const u64 next_pos = vf.pos + key_size;
            // Each input needs a unique label
            char frame_label[0x20] = {0};
            snprintf(frame_label, sizeof(frame_label), "Frame # ##%d##%8s", i, label_extra);

            char component_label[0x20] = {0};
            snprintf(component_label, sizeof(component_label), "##component_%d_%s", i, label_extra);

            // Display the input fields
            ImGui::InputScalar(frame_label, imgui_frame_type, vfile_cur(vf));

            // Skip over frame value
            vfile_seek(&vf, sizeof_type(frame_type));

            ImGui::InputScalarN(component_label, imgui_component_type, vfile_cur(vf), num_components);

            // Space between keys keeps things readable
            ImGui::Spacing();
            ImGui::Spacing();

            // Skip to next key
            vf.pos = next_pos;
        }
        ImGui::PopItemWidth();

        for (u32 cur_component = 0; cur_component < num_components; cur_component++) {
            // Reset seek position
            vf.pos = 0;


            char buf[128] = {0};
            snprintf(buf, sizeof(buf) - 1, "Curve editor %d", cur_component);

            const u16 component_offset = frame_size + (component_size * cur_component);
            const ImGui::graph_info info = {
                keyframes, key_count, key_size,
                0, component_offset, imgui_frame_type, imgui_component_type,
                ImVec2(0, 0), 1.0f,
            };
            ImGui::GraphData(info);
        }
    }

    bool edit_chunk_0x14(chunk_0x14& chunk) {
        ImGui::ScopedWidth width(16);
        bool res = false;
        res |= ImGui::InputPDString("Shader", &chunk.text1, &chunk.text2);
        res |= ImGui::InputU16("Material ID", &chunk.material_idx);
        res |= ImGui::InputU16("Shader ID", &chunk.shader_idx);
        return res;
    }
} // namespace al
