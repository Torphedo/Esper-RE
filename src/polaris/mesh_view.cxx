#include "mesh_view.hxx"
#include "pd_mesh.hxx"
#include "polaris.hxx"

#include <cstdio>
#include <imgui.h>
#include "imgui_utils.hxx"

typedef struct {
    const char* string;
    u16 gl_type;
}type_lookup_entry;

// Table mapping OpenGL type constants (stored in each vertex attribute) to a
// less amiguous human-readable string.
type_lookup_entry gl_type_table[] = {
    { "float", GL_FLOAT },
    { "u8", GL_UNSIGNED_BYTE },
    { "s8", GL_BYTE },
    { "u16", GL_UNSIGNED_SHORT },
    { "s16", GL_SHORT },
};

void edit_menu(vertex_attribute& attr) {
    const u8 min_components = 1;
    const u8 max_components = 4;
    ImGui::SliderScalar("# Components", ImGuiDataType_U8, &attr.components, &min_components, &max_components);

    ImGui::InputU16("Offset", &attr.offset);
    ImGui::Checkbox("Enable attribute", &attr.exists);

    // Find the index of our current type in the lookup table
    u16 current_type = 0;
    for (u32 i = 0; i < ARRAY_SIZE(gl_type_table); i++) {
        if (attr.type == gl_type_table[i].gl_type) {
            current_type = i;
            break;
        }
    }

    if (ImGui::BeginCombo("Data Type", gl_type_table[current_type].string)) {
        for (u32 i = 0; i < ARRAY_SIZE(gl_type_table); i++) {
            // https://github.com/ocornut/imgui/issues/1658
            const bool selected = current_type == i;
            if (ImGui::Selectable(gl_type_table[i].string)) {
                current_type = i; // Update selection
            }
            if (selected) {
                // Focus on the selected entry
                ImGui::SetItemDefaultFocus();
            }
        }
        ImGui::EndCombo();
    }

    // Update current type if needed.
    attr.type = gl_type_table[current_type].gl_type;
}

bool mesh_view::setup() {
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

void mesh_view::destroy() {
    glBindVertexArray(0);
    glBindBuffer(GL_ARRAY_BUFFER, 0);
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, 0);
    glDeleteVertexArrays(1, &vao);
    glDeleteBuffers(1, &vbo);

    for (index_buffer buf : idx_buffers) {
        glDeleteBuffers(1, &buf.obj);
    }
}

bool mesh_view::update_vertex_buf(const u8* buf, u32 size) {
    if (!initialized) {
        return false;
    }

    // TODO: Use glBufferSubData() when the size hasn't increased
    glBindBuffer(GL_ARRAY_BUFFER, vbo);
    glBufferData(GL_ARRAY_BUFFER, size, buf, GL_DYNAMIC_DRAW); 
    glBindBuffer(GL_ARRAY_BUFFER, 0);

    return true;
}

bool mesh_view::add_index_buf(index_buffer buf) {
    if (!initialized) {
        return false;
    }
    glBindVertexArray(vao);

    glGenBuffers(1, &buf.obj);
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, buf.obj);
    glBufferData(GL_ELEMENT_ARRAY_BUFFER, buf.num * sizeof(u16), buf.data, GL_DYNAMIC_DRAW);

    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, 0);
    glBindVertexArray(0);

    this->idx_buffers.push_back(buf);

    return true;
}

/// @brief Upload the new vertex format settings to the GPU
bool mesh_view::apply_attributes() {
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

        if (use_type_divisor) {
            uv_divisor = gl_type_max(attr.type);
        }

        // Update vertex format w/ OpenGL
        glEnableVertexAttribArray(i);
        glVertexAttribPointer(i, attr.components, attr.type, GL_FALSE, this->vertex_size, (void*)(u64)attr.offset);
    }
    glBindBuffer(GL_ARRAY_BUFFER, 0);
    glBindVertexArray(0);
    
    return true;
}

void mesh_view::edit_menu(const polaris* pol) {
    ImGui::Checkbox("Render mesh", &active);
    ImGui::InputU16("Vertex size", &this->vertex_size);

    // Edit menus per attribute
    ImGui::Text("Vertex Attributes:");
    for (u32 i = 0; i < ARRAY_SIZE(attributes); i++) {
        // Give child window a unique name to avoid ImGui errors
        char label[32] = {0};
        snprintf(label, sizeof(label) - 1, "##%d", i);
        ImGui::Text("%s:", attribute_names[i]);
        ImGui::BeginChild(label, ImVec2(0, 0), ImGuiChildFlags_AutoResizeY);

        if (i == ATTRIBUTE_TEXCOORD) {
            ImGui::Checkbox("Auto-scale integer UVs", &use_type_divisor);
        }

        if (i == ATTRIBUTE_TEXCOORD && !this->use_type_divisor) {
            ImGui::InputU32("Custom UV Divisor", &this->uv_divisor);
        }

        ::edit_menu(attributes[i]);

        ImGui::NewLine();

        ImGui::EndChild();
    }

    if (ImGui::Button("Apply attribute changes")) {
        this->apply_attributes();
    }
    ImGui::Spacing();

    ImGui::Text("Index buffers");
    ImGui::NewLine();

    std::optional<gl_obj> copy_albedo;
    std::optional<gl_obj> copy_normal;
    std::optional<u32> prev_vertex_group;

    for (u32 i = 0; i < idx_buffers.size(); i++) {
        ImGui::Text("Index buffer %d", i);
        index_buffer& buf = idx_buffers.at(i);
        char label[32] = {0};

        snprintf(label, sizeof(label) - 1, "Render ##%d", i);
        ImGui::Checkbox(label, &buf.enabled);

        snprintf(label, sizeof(label) - 1, "Albedo Texture Index ##%d", i);
        bool albedo_changed = ImGui::InputU16(label, &buf.albedo_tex_idx);

        snprintf(label, sizeof(label) - 1, "Normal texture Index ##%d", i);
        bool normal_changed = ImGui::InputU16(label, &buf.normal_tex_idx);
        buf.albedo_tex_idx %= pol->gl_textures.size();
        buf.normal_tex_idx %= pol->gl_textures.size();

        snprintf(label, sizeof(label) - 1, "Show textures ##%d", i);
        if (ImGui::CollapsingHeader(label)) {
            ImGui::Image(pol->gl_textures.at(buf.albedo_tex_idx), ImVec2(512, 512));
            ImGui::Image(pol->gl_textures.at(buf.normal_tex_idx), ImVec2(512, 512));
        }

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

        if (albedo_changed) {
            copy_albedo = buf.albedo_tex_idx;
            prev_vertex_group = buf.vertex_group;
        }
        if (normal_changed) {
            copy_normal = buf.normal_tex_idx;
            prev_vertex_group = buf.vertex_group;
        }

        // Copy texture indices if they were edited within this vertex group
        if (copy_albedo.has_value() && buf.vertex_group == prev_vertex_group.value()) {
            buf.albedo_tex_idx = copy_albedo.value();
        }
        if (copy_normal.has_value() && buf.vertex_group == prev_vertex_group.value()) {
            buf.normal_tex_idx = copy_normal.value();
        }

        ImGui::NewLine();
    }
}
