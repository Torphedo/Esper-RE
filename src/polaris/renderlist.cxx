#include "renderlist.hxx"
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

void vertex_attribute::edit_menu() {
    const u8 min_components = 1;
    const u8 max_components = 4;
    ImGui::SliderScalar("# Components", ImGuiDataType_U8, &components, &min_components, &max_components);

    ImGui::InputU16("Stride (vertex size)", &stride);
    ImGui::InputU16("Offset", &offset);
    ImGui::Checkbox("Disable attribute", &empty);

    // Find the index of our current type in the lookup table
    u16 current_type = 0;
    for (u32 i = 0; i < ARRAY_SIZE(gl_type_table); i++) {
        if (type == gl_type_table[i].gl_type) {
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
    type = gl_type_table[current_type].gl_type;
}

bool mesh_view::setup() {
    if (initialized) {
        return true; // Don't setup twice and leak OpenGL objects
    }

    // Set all the attributes empty, so that when the caller places things
    // in the array they automatically get marked non-empty. This lets us
    // use it instead of a dynamic array
    for (u32 i = 0; i < ARRAY_SIZE(attributes); i++) {
        attributes[i].empty = true;
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
        if (attr.empty) {
            continue;
        }

        if (use_type_divisor) {
            switch (attr.type) {
            case GL_BYTE:
                uv_divisor = INT8_MAX;
                break;
            case GL_UNSIGNED_BYTE:
                uv_divisor = UINT8_MAX;
                break;
            case GL_SHORT:
                uv_divisor = INT16_MAX;
                break;
            case GL_UNSIGNED_SHORT:
                uv_divisor = UINT16_MAX;
                break;
            case GL_FLOAT:
                // This doesn't really apply
                uv_divisor = 1;
                break;
            }
        }

        // Update vertex format w/ OpenGL
        glEnableVertexAttribArray(i);
        glVertexAttribPointer(i, attr.components, attr.type, GL_FALSE, attr.stride, (void*)(u64)attr.offset);
    }
    glBindBuffer(GL_ARRAY_BUFFER, 0);
    glBindVertexArray(0);
    
    return true;
}

void mesh_view::edit_menu(const std::vector<gl_obj>& tex_array) {
    // Edit triangle mode
    const char* gl_type_strings[] = {
        "GL_TRIANGLES", "GL_TRIANGLE_STRIP", "GL_TRIANGLE_FAN", "GL_POINTS", "GL_LINES", "GL_LINE_STRIP",
    };

    const u16 gl_types[] = {
        GL_TRIANGLES, GL_TRIANGLE_STRIP, GL_TRIANGLE_FAN, GL_POINTS, GL_LINES, GL_LINE_STRIP,
    };
    ImGui::Checkbox("Render mesh", &active);

    // Find index of the selected primitive type in the lookup table
    u16 current_type = 0;
    for (u32 i = 0; i < ARRAY_SIZE(gl_types); i++) {
        if (draw_mode == gl_types[i]) {
            current_type = i;
            break;
        }
    }

    if (ImGui::BeginCombo("Primitive type", gl_type_strings[current_type])) {
        for (u32 i = 0; i < ARRAY_SIZE(gl_types); i++) {
            const bool selected = current_type == i;
            if (ImGui::Selectable(gl_type_strings[i], selected)) {
                current_type = i;
            }
            if (selected) {
                ImGui::SetItemDefaultFocus();
            }
        }
        ImGui::EndCombo();
    }

    // Save new primitive type if needed
    draw_mode = gl_types[current_type];

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

        attributes[i].edit_menu(); 
        ImGui::NewLine();

        ImGui::EndChild();
    }

    if (ImGui::Button("Apply attribute changes")) {
        this->apply_attributes();
    }
    ImGui::Spacing();

    ImGui::Text("Index buffers");
    for (u32 i = 0; i < idx_buffers.size(); i++) {
        ImGui::Text("Buffer %d", i);
        index_buffer& buf = idx_buffers.at(i);
        char label[32] = {0};

        snprintf(label, sizeof(label) - 1, "Render ##%d", i);
        ImGui::Checkbox(label, &buf.enabled);

        snprintf(label, sizeof(label) - 1, "Albedo Texture Index ##%d", i);
        ImGui::InputU16(label, &buf.albedo_tex_idx);

        snprintf(label, sizeof(label) - 1, "Show albedo texture ##%d", i);
        buf.albedo_tex_idx %= tex_array.size();
        if (ImGui::CollapsingHeader(label)) {
            ImGui::Image(tex_array.at(buf.albedo_tex_idx), ImVec2(512, 512));
        }

        snprintf(label, sizeof(label) - 1, "Normal texture Index ##%d", i);
        ImGui::InputU16(label, &buf.normal_tex_idx);
        buf.normal_tex_idx %= tex_array.size();

        snprintf(label, sizeof(label) - 1, "Show normal texture ##%d", i);
        if (ImGui::CollapsingHeader(label)) {
            ImGui::Image(tex_array.at(buf.normal_tex_idx), ImVec2(512, 512));
        }
    }
}
