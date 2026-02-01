#include <glad/glad.h>

#include <formats/alr.h>
#include <util/imgui_utils.hxx>
#include <util/scope_timer.hxx>
#include <alr/alr_file.hxx>

/* === Static index buffer implementation === */

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

mat4s index_buffer::get_transform(const alr::file& alr, float frame, u32 anim_id) const noexcept {
    const scope_timer draw_timer("calcAnimTransforms", true);
    if (!is_skele_transform) {
        assert(position);
        assert(rotation);
        mat4s rot_xform = glms_euler_zyx(*rotation);
        mat4s pos_xform = glms_translate(GLMS_MAT4_IDENTITY_INIT, *position);
        return glms_mat4_mul(pos_xform, rot_xform);
    }

    vfile vf = vfile_open(alr.data, alr.alr_size);
    vf.pos = armature_chunk_offset;
    const auto* joint_header = VFILE_READ_PTR(chunk_armature, &vf);
    const auto* joints = VFILE_READ_PTR(joint_t, &vf);

    vf.pos = idx_chunk_offset;
    const auto* idx_header = VFILE_READ_PTR(idxbuf_header, &vf);

    // Calculate the object's xform by applying all of its parent xforms
    const s32 joint_idx = idx_header->transform_idx;
    mat4s obj_transform = alr.joint_final_xform(joint_header, anim_id, joint_idx, frame);

    return obj_transform;
}

/* === Static vertex buffer implementation === */

bool vertex_buffer::setup() noexcept {
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

void vertex_buffer::destroy() noexcept {
    glBindVertexArray(0);
    glBindBuffer(GL_ARRAY_BUFFER, 0);
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, 0);
    glDeleteVertexArrays(1, &vao);
    glDeleteBuffers(1, &vbo);
}

bool vertex_buffer::upload_vertex_buf(const u8* buf, u32 size) noexcept {
    if (!initialized) {
        return false;
    }

    glBindBuffer(GL_ARRAY_BUFFER, vbo);
    glBufferData(GL_ARRAY_BUFFER, size, buf, GL_DYNAMIC_DRAW); 
    glBindBuffer(GL_ARRAY_BUFFER, 0);

    return true;
}

bool vertex_buffer::apply_attributes() const noexcept {
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

void vertex_buffer::edit_menu() noexcept {
    const char* format_settings_help = "These may help if a model looks corrupted, or textures are applied wrong.";

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
            if (ImGui::Button("Apply attribute changes")) {
                this->apply_attributes();
            }

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
        }
    }
}

void alr::mesh::render(file& alr, render_context& ctx) const noexcept {
    const vertbuf_entry* vertbufs = chunks.vert_chunk->entries;
    const chunk_0x1_entry* materials = chunks.mat_chunk->entries;

    for (const index_buffer& idxbuf : idxbufs) {
        const idxbuf_header* header = (idxbuf_header*) (alr.data + idxbuf.idx_chunk_offset);
        const vertex_buffer& gl_vertbuf = gl_vertbufs[header->vertex_buf];
        if (!idxbuf.active || !gl_vertbuf.active) {
            continue;
        }

        const vertbuf_entry& vertbuf = vertbufs[header->vertex_buf];
        const chunk_0x1_entry& material = materials[header->texture_idx];

        ctx.fbo.set_wireframe(idxbuf.wireframe || ctx.wireframe);
        glBindVertexArray(gl_vertbuf.vao);
        mat4s xform = idxbuf.get_transform(alr, ctx.anim_frame, ctx.anim_id);
        mat4s cam_xform = {};
        ctx.cam.proj_view((vec4*)cam_xform.raw);

        mat4s pvm = glms_mul(cam_xform, xform);

        glUniformMatrix4fv(ctx.uniform_pvm, 1, GL_FALSE, (float*)pvm.raw);
        const u32 divisor = gl_vertbuf.uv_divisor;
        glUniform1ui(ctx.uniform_uv_divisor, divisor);

        glActiveTexture(GL_TEXTURE0);
        glBindTexture(GL_TEXTURE_2D, alr.tex_manager.get(alr, material.texture_idx));
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);

        u32 normal_idx = material.normal_idx;
        u32 lightmap_idx = 0;
        if (material.vertbuf_format == 0x1F) {
            lightmap_idx = material.normal_idx;
            normal_idx = material.normal_backup_idx;
        }

        glActiveTexture(GL_TEXTURE1);
        glBindTexture(GL_TEXTURE_2D, alr.tex_manager.get(alr, normal_idx));
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);

        glActiveTexture(GL_TEXTURE2);
        if (lightmap_idx == 0) {
            // Make sure lightmap samples all zeroes
            glBindTexture(GL_TEXTURE_2D, 0);
        } else {
            glBindTexture(GL_TEXTURE_2D, alr.tex_manager.get(alr, lightmap_idx));
            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
        }

        const u16 draw_mode = (header->primitive_type == IDX_TYPE_STRIP) ? GL_TRIANGLE_STRIP : GL_TRIANGLES;
        glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, idxbuf.obj);
        glDrawElements(draw_mode, header->num_indices, GL_UNSIGNED_SHORT, 0);
    }

    glBindTexture(GL_TEXTURE_2D, 0);
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, 0);
    glBindVertexArray(0);
}

void alr::mesh::destroy() noexcept {
    for (auto& idxbuf : idxbufs) {
        glDeleteBuffers(1, &idxbuf.obj);
    }

    for (vertex_buffer& vertbuf : gl_vertbufs) {
        vertbuf.destroy();
    }
}

alr::mesh mesh_at_idx(const alr::file& alr, u32 idx) {
    alr::mesh out = {};
    alr_model_desc model = alr.model_at_idx(idx);
    out.chunks = model;

    // Get the actual vertex buffer
    const u8* resbuf = alr.resource_buffer();

    for (u32 i = 0; i < model.vert_chunk->num_entries; i++) {
        vertex_buffer vertbuf = {};
        const vertbuf_entry& entry = model.vert_chunk->entries[i];
        const u8* vertices = resbuf + entry.data_ptr;

        // Upload vertex buffer
        vertbuf.setup();
        vertbuf.upload_vertex_buf(vertices, entry.vertex_size * entry.vertex_count);
        get_vert_attribute(&vertbuf, entry);
        vertbuf.apply_attributes();
        out.gl_vertbufs.push_back(vertbuf);
    }

    // We need offsets for our other utility functions
    const ptrdiff_t skel_chunk_offset = (ptrdiff_t)model.skel_chunk - (ptrdiff_t)alr.data;
    const ptrdiff_t idx_chunk_offset = (ptrdiff_t)model.idx_chunk - (ptrdiff_t)alr.data;
    const ptrdiff_t mat_chunk_offset = (ptrdiff_t)model.mat_chunk - (ptrdiff_t)alr.data;
    vfile vf = vfile_open(alr.data, alr.alr_size);
    vfile_seek(&vf, idx_chunk_offset);

    // Parse all index buffers
    auto* chunk = (chunk_generic*)vfile_cur(vf);
    while (chunk->id != ALR_ID_END_INDICES) {
        const idxbuf_header* idx_header = (idxbuf_header*)vfile_cur(vf);
        if (chunk->id == 0x2) {
            index_buffer idxbuf(vf.pos, skel_chunk_offset, mat_chunk_offset);
            glGenBuffers(1, &idxbuf.obj);
            glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, idxbuf.obj);
            glBufferData(GL_ELEMENT_ARRAY_BUFFER, idx_header->num_indices * sizeof(u16), idx_header->indices, GL_DYNAMIC_DRAW);

            const vertbuf_entry* entries = model.vert_chunk->entries;
            if (entries[idx_header->vertex_buf].vertex_size == 12) {
                // This only has space for position, it'll be a solid color and look
                // ugly in the viewport.
                out.gl_vertbufs[idx_header->vertex_buf].active = false;
            }

            out.idxbufs.push_back(idxbuf);
        }

        // Prepare to read next index buffer
        vfile_seek(&vf, chunk->size);
        chunk = (chunk_generic*)vfile_cur(vf);
        assert(chunk->id < 0x15);
    }
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, 0);

    return out;
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

std_vertex standardize_pd_vertex(void* vertbuf, u8 format_id) {
    vertex_format_t format = format_by_id(format_id);
    std_vertex output = {};
    // Get a virtual file for the buffer
    vfile vf = vfile_open(vertbuf, format.size);

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

    vertex_attribute normal_attr = format.attributes[ATTRIBUTE_NORMAL];
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

void get_vert_attribute(vertex_buffer* out, vertbuf_entry vert_header) {
    // Search our table of known formats
    vertex_format_t format = format_by_id(vert_header.format);

    // Copy format data to the output
    out->vertex_size = format.size;
    // TODO: Make divisors per-attribute instead of UV-only
    out->uv_divisor = format.attributes[ATTRIBUTE_TEXCOORD].divisor;
    memcpy(out->attributes, format.attributes, sizeof(format.attributes));
}

