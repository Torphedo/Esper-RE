#include "mesh_view.hxx"
#include <cstdio>

#include <common/vfile.h>
#include <formats/alr_animations.h>

#include "util/imgui_utils.hxx"
#include "alr/alr_dump.hxx"

const u16 gl_type_table[DATA_TYPE_COUNT] = {
    GL_BYTE, GL_UNSIGNED_BYTE,
    GL_SHORT, GL_UNSIGNED_SHORT,
    GL_INT, GL_UNSIGNED_INT,
    GL_FLOAT, GL_DOUBLE,
};

void edit_menu(vertex_attribute& attr) {
    const u8 min_components = 1;
    const u8 max_components = 4;
    ImGui::ScopedWidth width(20);

    ImGui::SliderScalar("# Components", ImGuiDataType_U8, &attr.components, &min_components, &max_components);
    ImGui::InputU16("Offset", &attr.offset);

    if (ImGui::BeginCombo("Data Type", nameof_type(attr.type))) {
        for (u32 i = 0; i < ARRAY_SIZE(data_type_names); i++) {
            // https://github.com/ocornut/imgui/issues/1658
            const bool selected = (attr.type == i);
            if (ImGui::Selectable(data_type_names[i])) {
                attr.type = data_type(i); // Update selection
            }
            if (selected) {
                // Focus on the selected entry
                ImGui::SetItemDefaultFocus();
            }
        }
        ImGui::EndCombo();
    }
    ImGui::Checkbox("Enable attribute", &attr.exists);
}

mat4s index_buffer::get_transform(const alr::file& alr) const noexcept {
    if (!is_skele_transform) {
        assert(position);
        assert(rotation);
        mat4s rot_xform = glms_euler_zyx(*rotation);
        mat4s pos_xform = glms_translate(GLMS_MAT4_IDENTITY_INIT, *position);
        return glms_mat4_mul(pos_xform, rot_xform);
    }

    vfile vf = vfile_open(alr.data, alr.alr_size);
    vf.pos = armature_chunk_offset;
    vfile_seek(&vf, sizeof(chunk_generic));
    const auto* joint_header = VFILE_READ_PTR(chunk_armature, &vf);
    const auto* joints = VFILE_READ_PTR(joint_t, &vf);

    vf.pos = idx_chunk_offset;
    vfile_seek(&vf, sizeof(chunk_generic));
    const auto* idx_header = VFILE_READ_PTR(idxbuf_header, &vf);

    const u32 anim_id = BAS01_WAIT0;
    float cur_frame = 0.0f;

    // Calculate the object's xform by applying all of its parent xforms
    s32 joint_idx = idx_header->transform_idx;
    const joint_t* joint = &joints[joint_idx];
    mat4s obj_transform = GLMS_MAT4_IDENTITY_INIT;
    do {
        mat4s joint_xform = alr::transform_from_joint(*joint);
        mat4s anim_xform = alr::anim_xform_for_joint(alr.data, alr.alr_size, anim_id, joint_idx, cur_frame);

        // HACK: If there's an animation for this joint, discard joint rotation to fix broken limbs.
        mat4s identity = GLMS_MAT4_IDENTITY_INIT;
        if (memcmp(identity.raw, anim_xform.raw, sizeof(identity)) != 0) {
            vec4s translation = {};
            mat4s rot_xform = {};
            vec3s scale = {};
            glms_decompose(joint_xform, &translation, &rot_xform, &scale);
            joint_xform = glms_translate_make(glms_vec3(translation));
        }

        joint_xform = glms_mat4_mul(joint_xform, anim_xform);
        obj_transform = glms_mat4_mul(joint_xform, obj_transform);
        if (joint->parent_idx < 0 || joint->parent_idx >= joint_header->joint_count) {
            break;
        }
        joint_idx = joint->parent_idx;
        joint = &joints[joint_idx];
    } while (true);

    return obj_transform;
}

bool mesh_view::setup() noexcept {
    if (initialized) {
        return true; // Don't setup twice and leak OpenGL objects
    }

    glGenVertexArrays(1, &vao);
    if (vao == 0) {
        return false;
    }

    glGenBuffers(1, &vbo);
    if (vbo == 0) {
        return false;
    }

    initialized = true;
    return true;
}

void mesh_view::destroy() noexcept {
    glBindVertexArray(0);
    glBindBuffer(GL_ARRAY_BUFFER, 0);
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, 0);
    glDeleteVertexArrays(1, &vao);
    glDeleteBuffers(1, &vbo);

    for (index_buffer buf : idx_buffers) {
        glDeleteBuffers(1, &buf.obj);
    }
}

bool mesh_view::update_vertex_buf(const u8* buf, u32 size) noexcept {
    if (!initialized) {
        return false;
    }
    vertices = buf;

    glBindBuffer(GL_ARRAY_BUFFER, vbo);
    glBufferData(GL_ARRAY_BUFFER, size, buf, GL_DYNAMIC_DRAW); 
    glBindBuffer(GL_ARRAY_BUFFER, 0);

    return true;
}

bool mesh_view::add_index_buf(const u8* alr_data, u32 alr_size, index_buffer buf) noexcept {
    if (!initialized) {
        return false;
    }
    // We cast away const here but don't write to the buffer
    vfile vf = vfile_open((u8*)alr_data, alr_size);
    vf.pos = buf.idx_chunk_offset;
    const auto genheader = VFILE_READ(chunk_generic, &vf);
    assert(genheader.id == 0x2);
    const auto header = VFILE_READ(idxbuf_header, &vf);
    const auto* data = (u16*)vfile_cur(vf);

    glBindVertexArray(vao);

    glGenBuffers(1, &buf.obj);
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, buf.obj);
    glBufferData(GL_ELEMENT_ARRAY_BUFFER, header.num_indices * sizeof(u16), data, GL_DYNAMIC_DRAW);

    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, 0);
    glBindVertexArray(0);

    this->idx_buffers.push_back(buf);

    return true;
}

/// @brief Upload the new vertex format settings to the GPU
bool mesh_view::apply_attributes() const noexcept {
    if (!initialized) {
        return false;
    }

    glBindVertexArray(vao);
    glBindBuffer(GL_ARRAY_BUFFER, vbo);

    for (u32 i = 0; i < ARRAY_SIZE(attributes); i++) {
        const vertex_attribute attr = attributes[i];
        if (!attr.exists) {
            continue;
        }

        const u16 gl_type = gl_type_table[attr.type];

        // Update vertex format w/ OpenGL
        glEnableVertexAttribArray(i);
        glVertexAttribPointer(i, attr.components, gl_type, GL_FALSE, this->vertex_size, (void*)(u64)attr.offset);
    }
    glBindBuffer(GL_ARRAY_BUFFER, 0);
    glBindVertexArray(0);
    
    return true;
}

void mesh_view::edit_menu(alr::file& alr) noexcept {
    const char* format_settings_help = "These may help if a model looks corrupted, or textures are applied wrong.";
    const char* idxbuf_help = "The individual objects within the model";

    ImGui::Checkbox("Render mesh", &active);

    const bool format_settings = ImGui::CollapsingHeader("Vertex Format Settings");
    // Tooltip is placed on header, even if collapsed
    ImGui::SetItemTooltip(format_settings_help);

    if (format_settings) {
        ImGui::ScopedIndent indent(ImGui::CharWidth(2));
        ImGui::ScopedWidth width(20);

        ImGui::InputU16("Vertex size", &this->vertex_size);
        ImGui::InputU32("UV Divisor", &this->uv_divisor);

        // Edit menus per attribute
        if (ImGui::CollapsingHeader("Vertex Attributes")) {
            ImGui::ScopedIndent indent2(ImGui::CharWidth(2));

            for (u32 i = 0; i < ARRAY_SIZE(attributes); i++) {
                // Give child window a unique name to avoid ImGui errors
                char label[32] = {0};
                snprintf(label, sizeof(label) - 1, "##%d", i);
                ImGui::Text("%s:", attribute_names[i]);
                ImGui::BeginChild(label, ImVec2(0, 0), ImGuiChildFlags_AutoResizeY);

                ::edit_menu(attributes[i]);

                ImGui::NewLine();
                ImGui::EndChild();
            }

            if (ImGui::Button("Apply attribute changes")) {
                this->apply_attributes();
            }
        }
    }

    const bool idx_buf = ImGui::CollapsingHeader("Index buffers");
    ImGui::SetItemTooltip(idxbuf_help);
    if (idx_buf) {
        ImGui::ScopedIndent indent(ImGui::CharWidth(2));
        ImGui::ScopedWidth width(20);

        for (u32 i = 0; i < idx_buffers.size(); i++) {
            index_buffer &buf = idx_buffers.at(i);
            ImGui::Text("Index buffer %d (@ 0x%X)", i, buf.idx_chunk_offset);
            char label[32] = {0};

            snprintf(label, sizeof(label) - 1, "Render ##%d", i);
            ImGui::Checkbox(label, &buf.enabled);

            // We cast away const here but don't write to the buffer
            vfile vf = vfile_open(alr.data, alr.alr_size);
            vf.pos = buf.idx_chunk_offset;
            vfile_seek(&vf, sizeof(chunk_generic));
            const auto header = VFILE_READ(idxbuf_header, &vf);
            const auto mat_chunk = alr.prev_chunk_by_id(0x1, buf.idx_chunk_offset);
            chunk_0x1_entry *tex_entry = nullptr;
            alr.tex_manager.get_material(alr, mat_chunk.offset, header.texture_idx, &tex_entry);

            snprintf(label, sizeof(label) - 1, "Albedo Texture##%d", i);
            ImGui::InputU16(label, &tex_entry->texture_idx);

            snprintf(label, sizeof(label) - 1, "Normal Texture##%d", i);
            ImGui::InputU16(label, &tex_entry->normal_idx);
            // buf.albedo_tex_idx %= pol->alr.tex_manager.
            // buf.normal_tex_idx %= pol->gl_textures.size();

            snprintf(label, sizeof(label) - 1, "Show textures##%d", i);
            if (ImGui::CollapsingHeader(label)) {
                ImGui::Image(alr.tex_manager.get(alr, tex_entry->texture_idx), ImVec2(512, 512));
                ImGui::Image(alr.tex_manager.get(alr, tex_entry->normal_idx), ImVec2(512, 512));
            }

            // TODO: Bring back primitive override
            /*
            // Edit triangle mode
            const char* gl_type_strings[] = {
                "GL_TRIANGLES", "GL_TRIANGLE_STRIP", "GL_TRIANGLE_FAN", "GL_POINTS", "GL_LINES", "GL_LINE_STRIP",
            };

            const u16 gl_types[] = {
                GL_TRIANGLES, GL_TRIANGLE_STRIP, GL_TRIANGLE_FAN, GL_POINTS, GL_LINES, GL_LINE_STRIP,
            };

            // Find index of the selected primitive type in the lookup table
            u16 current_type = 0;
            for (u32 i = 0; i < ARRAY_SIZE(gl_types); i++) {
                if (buf.draw_mode == gl_types[i]) {
                    current_type = i;
                    break;
                }
            }

            if (ImGui::BeginCombo("Primitive type", gl_type_strings[current_type])) {
                for (u32 i = 0; i < ARRAY_SIZE(gl_types); i++) {
                    const bool selected = current_type == i;
                    if (ImGui::Selectable(gl_type_strings[i], selected)) {
                        // Save new primitive type if needed
                        current_type = i;
                        buf.draw_mode = gl_types[current_type];
                    }
                    if (selected) {
                        ImGui::SetItemDefaultFocus();
                    }
                }
                ImGui::EndCombo();
            }
            */

            ImGui::NewLine();
        }
    }
}

vec4s read_attr(vfile& vf, vertex_attribute attr) {
    vec4s result = {};
    if (!attr.exists) {
        return result;
    }

    vf.pos = attr.offset;
    for (u32 i = 0; i < attr.components; i++) {
        float val = 0.0f;
        switch (attr.type) {
            case DATA_TYPE_FLOAT:
                val = VFILE_READ(float, &vf);
                break;
            case DATA_TYPE_S8:
                val = VFILE_READ(s8, &vf);
                break;
            case DATA_TYPE_U8:
                val = VFILE_READ(u8, &vf);
                break;
            case DATA_TYPE_S16:
                val = VFILE_READ(s16, &vf);
                break;
            case DATA_TYPE_U16:
                val = VFILE_READ(u16, &vf);
                break;
            default:
                LOG_MSG(warning, "Unimplemented data type '%s'! (%d bytes)\n", nameof_type(attr.type), sizeof_type(attr.type));
                break;
        }
        if (attr.divisor > 0) {
            val /= float(attr.divisor);
        }

        result.raw[i] = val;
    }

    return result;
}

// TODO: Make this also use the format table.
std_vertex standardize_pd_vertex(void* vertbuf, u8 format_id) {
    vertex_format_t format = format_by_id(format_id);
    std_vertex output = {};
    // Get a virtual file for the buffer
    vfile vf = vfile_open(vertbuf, format.size);

    // The only thing consistent across formats is that they always start with
    // the 3D position.

    vertex_attribute pos_attr = format.attributes[ATTRIBUTE_POSITION];
    if (pos_attr.exists) {
        vec4s pos = read_attr(vf, pos_attr);
        output.pos = vec3s{pos.x, pos.y, pos.z};
    }

    vertex_attribute uv_attr = format.attributes[ATTRIBUTE_TEXCOORD];
    if (uv_attr.exists) {
        vec4s uv = read_attr(vf, uv_attr);
        output.texcoord = vec2s{uv.x, uv.y};
    }

    vertex_attribute normal_attr = format.attributes[ATTRIBUTE_TEXCOORD];
    if (normal_attr.exists) {
        vec4s normal_temp = read_attr(vf, normal_attr);
        vec3s normal = vec3s{normal_temp.x, normal_temp.y, normal_temp.z};
        normal = glms_normalize(normal);

        output.normal = normal;
    }

    // Fix vertically flipped UVs to match what Blender expects
    if (output.texcoord.has_value()) {
        output.texcoord.value().y = reflect(output.texcoord.value().y, 0.5f);
    }

    return output;
}

void get_vert_attribute(mesh_view* out, vertbuf_entry vert_header) {
    // Search our table of known formats
    vertex_format_t format = format_by_id(vert_header.format);

    // Copy format data to the output
    out->vertex_size = format.size;
    // TODO: Make divisors per-attribute instead of UV-only
    out->uv_divisor = format.attributes[ATTRIBUTE_TEXCOORD].divisor;
    memcpy(out->attributes, format.attributes, sizeof(format.attributes));
}
