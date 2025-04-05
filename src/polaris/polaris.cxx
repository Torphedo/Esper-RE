#include <cmath>
#include <cstdio>
#include <glad/glad.h>
#include <imgui_curve.hpp>
#include <imgui.h>
#include <imgui_internal.h>
#include <cglm/struct.h>
#include <nfd.h>

#include <common/int.h>
#include <common/vfile.h>
#include <common/file.h>
#include <common/vmem.h>
#include <common/logging.h>

#include <formats/pd_common.h>
#include <formats/alr.h>

#include "alr_texture.hxx"
#include "pd_mesh.hxx"
#include "imgui_utils.hxx"
#include "polaris/renderlist.hxx"
#include "polaris.hxx"

enum {
    // The power of 2 to limit texture resolutions to
    // e.g. 2^12 = 4096
    ALR_TEX_POWER_LIMIT = 12,
};

// Normally I'd make this a method, but by using a macro we can have LOG_MSG()
// automatically log the name of the method that shouldn't have been called.
#define CHUNK_ID_ASSERT(expected_id) \
do {                                 \
    if (id != expected_id) {         \
        LOG_MSG(warning, "called on 0x%x chunk @ 0x%x, when it only makes sense for 0x%x chunks!\n", id, offset, expected_id);\
        return;                      \
    }                                \
} while(0)

u32 polaris::chunk::num_indices(const polaris& pol) const noexcept {
    if (id != 0x2) {
        return 0; // Can't use the assert macro because we return a value
    }

    // Open ALR data and skip to header
    vfile vf = vfile_open(pol.alr_data, pol.alr_size);
    vf.pos = this->offset + sizeof(chunk_generic);
    const idxbuf_header header = VFILE_READ(idxbuf_header, &vf);

    if (header.unk3 == IDX_TYPE_STRIP) {
        return header.num_indices;
    } else {
        return header.num_indices;
    }

    // Buffer space available for indices
    const s32 buf_size = size - sizeof(chunk_generic) - sizeof(idxbuf_header);

    // Indices are always u16 (so far)
    return MAX(0, buf_size / (sizeof(u16)));
}

void polaris::chunk::dump_idx_buf(const polaris& pol, FILE* out, std::optional<vertbuf_entry> vert_entry) const noexcept {
    CHUNK_ID_ASSERT(0x2);
    vfile vf = vfile_open(pol.alr_data + offset, size);

    // Skip over chunk header
    const chunk_generic generic_header = VFILE_READ(chunk_generic, &vf);
    const idxbuf_header header = VFILE_READ(idxbuf_header, &vf);

    const u16* indices = (u16*)vfile_cur(vf);
    for (s32 i = 2; i < header.num_indices; i++) {
        u16 idx1 = indices[i - 2];
        u16 idx2 = indices[i - 1];
        u16 idx3 = indices[i];

        if (idx1 == idx2 || idx1 == idx3 || idx2 == idx3) {
            // One of the indices is a duplicate, so this triangle will have
            // zero area. This happens sometimes in triangle strips, telling us
            // where one strip ends and another begins. We can safely skip it,
            // because it's not really part of the geometry.
            continue;
        }

        // OBJ indices start at 1 :(
        idx1++;
        idx2++;
        idx3++;

        bool use_uvs = false;
        if (vert_entry.has_value()) {
            use_uvs = has_uvs(vert_entry->vertex_size);
        }

        if (use_uvs) {
            fprintf(out, "f %hu/%hu %hu/%hu %hu/%hu\n", idx1, idx1, idx2, idx2, idx3, idx3);
        } else {
            fprintf(out, "f %hu %hu %hu\n", idx1, idx2, idx3);
        }

        if (vert_entry.has_value()) {
            // The indicator for triangle strips seems to be in the index buffer
            // header
            if (header.unk3 != IDX_TYPE_STRIP) {
                // For triangle strips, we advance by 1 index but still read 3
                // indices per iteration. For normal index buffers, we read and
                // advance 3 at a time. Our loop counts up by 1, so we have to
                // add an extra 2.
                i += 2;
            }
        }
    }

}

void polaris::chunk::dump_vertex_buf(const polaris& pol, const char* path, vertbuf_entry entry) const noexcept {
    CHUNK_ID_ASSERT(0x16);

    // Dump to OBJ
    FILE *out = fopen(path, "wb");
    if (out != nullptr) {
        // Open resource buffer
        vfile vf = vfile_open(pol.alr_data, pol.alr_size);

        // Jump to the appropriate data
        vfile_seek(&vf, pol.resbuf_offset);
        vfile_seek(&vf, entry.data_ptr);
        bool has_uvs = false;
        for (u32 i = 0; i < entry.vertex_count; i++) {
            const s64 next_pos = vf.pos + entry.vertex_size;
            // Read the vertex (this abstracts away the many different formats)
            const std_vertex vert = standardize_pd_vertex(vfile_cur(vf), entry.vertex_size);

            // Save whatever vertex data we got
            if (vert.pos.has_value()) {
                const vec3s pos = vert.pos.value();
                fprintf(out, "v %f %f %f\n", pos.x, pos.y, pos.z);
            }

            if (vert.texcoord.has_value()) {
                has_uvs = true;
                const vec2s uv = vert.texcoord.value();
                fprintf(out, "vt %f %f\n", uv.x, uv.y);
            }

            // Skip to the next vertex
            vf.pos = next_pos;
        }

        // Vertices are dumped, now for indices
        for (chunk idx_chunk : pol.chunks) {
            if (idx_chunk.id == this->id && idx_chunk.offset > this->offset) {
                // We've hit a mesh metadata chunk past our own, so any
                // further index buffers will be garbage data to us. Quit.
                break;
            }

            if (idx_chunk.id != 0x2) {
                // We only want index buffer chunks
                continue;
            }

            if (idx_chunk.offset < offset) {
                // This index buffer is from a previous mesh, so it's
                // garbage data to us. Skip.
                continue;
            }

            // Skip to idx_chunk and skip header
            vf.pos = idx_chunk.offset;
            vfile_seek(&vf, sizeof(chunk_generic));
            const idxbuf_header header = VFILE_READ(idxbuf_header, &vf);

            // We only want index buffers meant for this vertex buffer
            if (header.vertex_buf != window_0x16.selected_vertex_buf && header.vertex_buf2 != window_0x16.selected_vertex_buf) {
                continue;
            }

            fprintf(out, "\ng idxbuf_0x%lx\n", idx_chunk.offset);
            idx_chunk.dump_idx_buf(pol, out, entry);
        }

        // Cleanup
        fclose(out);
    }
}

void polaris::chunk::chunk_0x2(const polaris& pol) noexcept {
    CHUNK_ID_ASSERT(0x2);

    if (ImGui::Button("Export to OBJ")) {
        // Display the file picker
        nfdu8filteritem_t filters[] = { { "3D Model", "obj"} };
        char* path = nullptr;
        nfdresult_t result = NFD_SaveDialogU8(&path, filters, ARRAY_SIZE(filters), nullptr, nullptr);
        if (result == NFD_OKAY && path != nullptr) {
            // Dump to OBJ
            FILE* out = fopen(path, "ab");
            if (out == nullptr) {
                return;
            }

            this->dump_idx_buf(pol, out);
            fclose(out);
        }
        free(path);
    }

    // Index buffer editing
    vfile vf = vfile_open(pol.alr_data + this->offset, this->size);
    chunk_generic chunk = VFILE_READ(chunk_generic, &vf);
    // We get the header pointer so we can modify it in-place
    idxbuf_header* header = (idxbuf_header*)vfile_cur(vf);
    vfile_seek(&vf, sizeof(*header)); // Skip past the header

    // Sanity check some of our assumptions & show warning messages if they fail
    idxbuf_header temp = {0};
    const char* pad_warning = "WARNING: What I thought was padding @ chunk offset 0x%x had real data!";
    ImGui::PlsReportIf(memcmp(header->pad, temp.pad, sizeof(temp.pad)) != 0, pad_warning, offsetof(idxbuf_header, pad));
    ImGui::PlsReportIf(memcmp(header->pad2, temp.pad2, sizeof(temp.pad2)) != 0, pad_warning, offsetof(idxbuf_header, pad));
    ImGui::PlsReportIf(header->vertex_buf != header->vertex_buf2, "What I thought was duplicate data actually isn't!");

    const u16 first_idx = VFILE_READ(u16, &vf);
    vf.pos -= sizeof(first_idx);
    ImGui::PlsReportIf(first_idx > header->first_idx, "What I thought was the first index value isn't that OR the smallest index!");
    ImGui::PlsReportIf(first_idx < header->first_idx && first_idx != header->first_idx, "What I thought was the first index value seems to actually be the smallest index.");


    if (ImGui::InputU16("Vertex Buffer", &header->vertex_buf)) {
        // There are always 2 copies of this data for some reason, so update
        // the other when this one is updated.
        header->vertex_buf2 = header->vertex_buf;
    }
    ImGui::InputU16("Vertex Buffer 2", &header->vertex_buf2);

    // If we trust the file about the number of triangles, many indices will be
    // missing.
    const u16 lower_bound = header->first_idx;
    const u32 idx_buf_size = size - sizeof(chunk_generic) - sizeof(*header);
    s32 num_tris = MAX(0, idx_buf_size / (3 * sizeof(u16)));
    // Percentage of how close the predicted count is to the size reported by
    // the ALR
    const float capacity_diff = ((float)header->num_tris / num_tris) * 100.0f;
    ImGui::Text("ALR says there's %d triangles, buffer can hold %d\n(Using %.1f%% of capacity)", header->num_tris, num_tris, capacity_diff);

    ImGui::Checkbox("Use ALR's triangle count", &window_0x2.trust_alr_tri_count);
    if (window_0x2.trust_alr_tri_count) {
        num_tris = header->num_tris;
    }

    ImGui::InputU32("# of triangles", &header->num_tris);
    ImGui::InputU32("# of indices", &header->num_indices);
    ImGui::InputU32("Smallest index", &header->first_idx);
    ImGui::InputFloat3("Center point", header->center);
    ImGui::InputFloat3("AABB Min", header->aabb_min);
    ImGui::InputFloat3("AABB Max", header->aabb_max);

    if (ImGui::CollapsingHeader("Unknown Fields")) {
        ImGui::InputFloat("Unknown float 1", &header->unk_float);

        ImGui::InputU32("Unknown integer 1", &header->unk1);
        ImGui::InputU16("Unknown integer 2", &header->unk2);
        ImGui::InputU16("Unknown integer 3", &header->unk3);
    }
    for (u32 i = 0; i < 5; i++) {
        ImGui::Spacing();
    }

    for (s32 i = 0; i < num_tris; i++) {
        // User inputs for this triangle
        char label[0x20] = {0};
        snprintf(label, sizeof(label), "Triangle %d", i + 1);
        ImGui::InputScalarN(label, ImGuiDataType_U16, vfile_cur(vf), 3);

        const u16 idx1 = VFILE_READ(u16, &vf);
        const u16 idx2 = VFILE_READ(u16, &vf);
        const u16 idx3 = VFILE_READ(u16, &vf);
        const bool idx_too_small = idx1 < lower_bound || idx2 < lower_bound || idx3 < lower_bound;

        // If the ALR has a bad count we *will* go out of bounds here.
        // Since we have the whole file loaded this won't cause any crashes,
        // and may help illustrate where the end of the real data is.
        if (idx_too_small && !window_0x2.trust_alr_tri_count) {
            // We're hitting some invalid data, give up.
            break;
        }
    }
}

void polaris::chunk::chunk_0x3(const polaris& pol) noexcept {
    CHUNK_ID_ASSERT(0x3);

    // TODO: add a 3D viewport here so we can see all the matrix positions
    vfile vf = vfile_open(pol.alr_data + offset, size);
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

    joint_t* joint = &joints[window_0x3.selected_joint];
    ImGui::InputPDString("Joint Name", &joint->name);
    ImGui::Text("Parent index: %d", joint->parent_idx);

    if (ImGui::BeginTabBar("editors")) {
        if (ImGui::BeginTabItem("Float editor")) {

            // Matrix inputs
            ImGui::PushItemWidth(200.0f); // Make inputs narrower
            for (u32 j = 0; j < 3; j++) {
                for (u32 k = 0; k < 3; k++) {
                    char buf[0x20] = {0};
                    snprintf(buf, sizeof(buf), "##%d%d", j, k);
                    mat3* mat = &joint->mat;
                    ImGui::InputFloat(buf, &(*mat)[j][k]);
                    ImGui::SameLine();
                }
                ImGui::Text(" "); // Cause a new line
            }
            ImGui::PopItemWidth();
            ImGui::EndTabItem();
        }

        if (ImGui::BeginTabItem("Hex Editor")) {
            // Show hex editor
            hex_edit.DrawContents(joint, sizeof(*joint));

            ImGui::EndTabItem();
        }

        ImGui::EndTabBar();
    }
}

/// @brief Create input boxes for ALR animation keys of any type or size
///
/// @param key_size The size of each animation key
/// @param key_count The number of animation keys
/// @param keyframes The address of the first key
/// @param label_extra A unique name of this set of keyframes (must not be
/// nullptr). This won't be displayed, only used to give the input boxes a
/// unique ID in ImGui.
static void edit_keyframes(u16 key_size, u16 key_count, void* keyframes, const char* label_extra) {
    if (keyframes == nullptr || label_extra == nullptr) {
        ImGui::Text("Programmer error: %s() was passed a null value", __func__);
        return;
    }

    ImGuiDataType frame_type = ImGuiDataType_COUNT;
    ImGuiDataType component_type = ImGuiDataType_COUNT;
    u16 num_components = 0;

    switch (key_size) {
    // Integer keys
    case 3:
    case 5:
    case 7:
        frame_type = ImGuiDataType_U8;
        component_type = ImGuiDataType_U16;
        // We know component and frame value size, so we can find out the # of components
        num_components = (key_size - sizeof(u8)) / sizeof(u16);
        break;

    // Floating point keys
    case 8:
    case 12:
    case 16:
        frame_type = component_type = ImGuiDataType_Float;
        // Same deal as above
        num_components = (key_size - sizeof(float)) / sizeof(float);
    }

    if (num_components == 0 || component_type == ImGuiDataType_COUNT) {
        // Something wasn't filled out, probably unknown format
        ImGui::Text("Unknown keyframe format (0x%X bytes)", key_size);
        return;
    }

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
        ImGui::InputScalar(frame_label, frame_type, vfile_cur(vf));

        // Skip over frame value
        if (frame_type == ImGuiDataType_Float) {
            vfile_seek(&vf, sizeof(float));
        }
        else if (frame_type == ImGuiDataType_U8) {
            vfile_seek(&vf, sizeof(u8));
        }

        ImGui::InputScalarN(component_label, component_type, vfile_cur(vf), num_components);

        // Space between keys keeps things readable
        ImGui::Spacing();
        ImGui::Spacing();

        // Skip to next key
        vf.pos = next_pos;
    }

    ImVec2* graph_points = (ImVec2*)calloc(key_count, sizeof(*graph_points));
    if (graph_points == nullptr) {
        return;
    }

    for (u32 cur_component = 0; cur_component < num_components; cur_component++) {
        ImVec2 min = ImVec2(0, INFINITY);
        ImVec2 max = ImVec2(key_count, -INFINITY);

        // Reset seek position
        vf.pos = 0;
        for (u32 cur_key = 0; cur_key < key_count; cur_key++) {
            const u64 next_pos = vf.pos + key_size;
            float frame = 0.0f;
            switch (frame_type) {
            case ImGuiDataType_Float:
                frame = VFILE_READ(float, &vf);
                break;
            case ImGuiDataType_U8:
                frame = VFILE_READ(u8, &vf);
                break;
            default:
                break;
            }

            float val = 0.0f;
            switch (component_type) {
            case ImGuiDataType_Float:
                // Skip to the component we want and read it
                vfile_seek(&vf, sizeof(float) * cur_component);
                val = VFILE_READ(float, &vf);
                break;
            case ImGuiDataType_U16:
                // Skip to the component we want and read it
                vfile_seek(&vf, sizeof(u16) * cur_component);
                val = (VFILE_READ(u16, &vf)) / (float)INT16_MAX;
                break;
            default:
                break;
            }

            // Update our min and max positions (determines graph bounds)
            min.y = MIN(min.y, val);
            max.y = MAX(max.y, val);

            graph_points[cur_key].x = frame;
            graph_points[cur_key].y = val;

            // Skip to next key
            vf.pos = next_pos;
        }

        char buf[128] = {0};
        snprintf(buf, sizeof(buf) - 1, "Curve editor %d", cur_component);
        bool modified = ImGui::Curve(buf, ImVec2(500, 500), key_count, graph_points, nullptr, min, max);

        // Because the library doesn't support custom step or any other way to
        // graph strangely spaced data, we have to copy all the data back to
        // the original buffer if something changes. At some point we should
        // probably modify it to modify our buffer directly and handle more
        // data types.

        // if (modified)
        {
            for (u32 cur_key = 0; cur_key < key_count; cur_key++) {
                void* key = ((u8*)keyframes + (cur_key * key_size));
                switch (component_type) {
                case ImGuiDataType_Float:
                    ((float*)key)[1 + cur_component] = graph_points[cur_key].y;
                    break;
                case ImGuiDataType_U16:
                    ((u16*)key)[1 + cur_component] = graph_points[cur_key].y;
                    break;
                default:
                    break;
                }
            }
        }
    }

    free(graph_points);
}

void polaris::chunk::chunk_0x5(const polaris& pol) noexcept {
    vfile vf = vfile_open(pol.alr_data + offset, size);
    anim_header* header = (anim_header*)vfile_cur(vf);
    vfile_seek(&vf, sizeof(*header));

    // TODO: Add an animation graph to add to the more manual editor
    ImGui::Text("Length: %.3f frames", header->length);
    ImGui::Text("%d translation keys, 0x%X bytes each", header->translation_key_count, header->translation_key_size);
    ImGui::Text("%d rotation keys, 0x%X bytes each", header->rotation_key_count, header->rotation_key_size);
    ImGui::Text("%d scale keys", header->scale_key_count);

    // Edit and skip to the next set of keys
    if (header->translation_key_count > 0 && ImGui::CollapsingHeader("Translation Keys")) {
        edit_keyframes(header->translation_key_size, header->translation_key_count, vfile_cur(vf), "trans");
        vfile_seek(&vf, header->translation_key_size * header->translation_key_count);
    }

    if (header->rotation_key_count > 0 && ImGui::CollapsingHeader("Rotation Keys")) {
        edit_keyframes(header->rotation_key_size, header->rotation_key_count, vfile_cur(vf), "rot");
        vfile_seek(&vf, header->rotation_key_size * header->rotation_key_count);
    }
}

void polaris::chunk::chunk_0x7(const polaris& pol) noexcept {
    // This is the same as normal animation frames, but seems to ignore the
    // existing keyframe size fields.
    vfile vf = vfile_open(pol.alr_data + offset, size);
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
        vfile_seek(&vf, key_size * header->translation_key_count);
    }

    if (header->rotation_key_count > 0 && ImGui::CollapsingHeader("Rotation Keys")) {
        edit_keyframes(key_size, header->rotation_key_count, vfile_cur(vf), "rot");
        vfile_seek(&vf, key_size * header->rotation_key_count);
    }
}

static ImVec2 draw_image(gl_obj tex_id, u16 width, u16 height, bool* scale_to_window, float* scale_factor, const char* id, ImVec2 uv0 = ImVec2(0, 0), ImVec2 uv1 = ImVec2(1, 1)) noexcept {
    // We need unique labels every time, so just combine some values that are
    // usually different. Texture ID is the same in a texture atlas and its
    // contents. This isn't foolproof but it works.
    char label[0x20] = {0};
    snprintf(label, sizeof(label), "Scale to window##%d%lf%s", tex_id, uv1.x, id);
    ImGui::Checkbox(label, scale_to_window);

    if (*scale_to_window) {
        // Force view size == texture size to make auto-scaling work
        *scale_factor = 1.0f;
    } else {
        snprintf(label, sizeof(label), "Render Scale ##%d%lf%s", tex_id, uv1.x, id);
        ImGui::SliderFloat(label, scale_factor, 0.001f, 10.0f);
    }

    // Scale the texture depending on the current settings.
    ImVec2 view_size = ImVec2((float)width * (*scale_factor), (float)height * (*scale_factor));
    if (*scale_to_window) {
        // We try to fill the space available to us
        const ImVec2 avail = ImGui::GetContentRegionAvail();

        // This is the scale on each axis that'll make the image fill the whole window
        const ImVec2 scale_temp = avail / view_size;

        // We scale by a uniform factor to preserve aspect ratio, so pick the
        // closer axis (to keep the entire image in frame)

        // Sometimes the scale ends up negative and I'm not sure why, so I just threw an fabsf() on it.
        // - torph
        const float new_scale = fabsf(MIN(scale_temp.x, scale_temp.y));

        view_size *= new_scale;
        // Some callers rely on accurate scale info, so pass it along
        *scale_factor = new_scale;
    }

    const ImVec2 image_pos = ImGui::GetCursorScreenPos();
    // TODO: Look into showing mipmap contents
    ImGui::Image(tex_id, view_size, uv0, uv1);

    return image_pos;
}

void polaris::chunk::chunk_0x10(const polaris& pol) noexcept {
    CHUNK_ID_ASSERT(0x10);

    // We use the vfile API to handle the chunk data
    vfile vf = vfile_open(pol.alr_data + offset, size);
    // Skip over the ID and size fields we already have
    vfile_seek(&vf, sizeof(chunk_generic));

    // We use pointers instead of reading into stack copies, so we can edit the
    // data directly. I'm not usually a big fan of using auto, but it doesn't
    // hide the real data type so I think it's fine here.
    auto* header = (atlas_header*) vfile_cur(vf);
    vfile_seek(&vf, sizeof(*header));

    // Read surface names
    auto* atlas_names = (atlas_name*) vfile_cur(vf);
    vfile_seek(&vf, sizeof(*atlas_names) * header->atlas_count);

    // Read surface metadata
    auto* atlases = (atlas_entry*) vfile_cur(vf);
    vfile_seek(&vf, sizeof(*atlases) * header->atlas_count);

    // Read texture metadata
    auto* textures = (atlas_tex_entry *) vfile_cur(vf);
    vfile_seek(&vf, sizeof(*textures) * header->texture_count);

    // We have to look up texture entries to find out where each texture is
    texture_entry* entries = nullptr;
    for (chunk c : pol.chunks) {
        if (c.id == 0x15) {
            // Skip to the chunk
            vfile tmp = vfile_open(pol.alr_data + c.offset, c.size);
            vfile_seek(&tmp, sizeof(chunk_generic));

            const u32 num_entries = VFILE_READ(u32, &tmp);
            entries = (texture_entry*)vfile_cur(tmp);
            break;
        }
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
            // Once we have a mechanism to find the atlases' position in the
            // texture buffer, clicking on a texture should set it as the
            // active texture and display it.
        }
    }
    ImGui::EndChild();

    atlas_name* aName = &atlas_names[window_0x10.selected_atlas];
    atlas_entry* atlas = &atlases[window_0x10.selected_atlas];
    atlas_tex_entry* tex = &textures[window_0x10.selected_atlas_texture];

    texture cur_tex = convert_tex(pol.alr_data + pol.resbuf_offset, entries[tex->index]);
    // Override dimensions, we only want format info from the other chunk
    cur_tex.height = atlas->height;
    cur_tex.width = atlas->width;


    if (window_0x10.gl_tex_id == 0) {
        // Create & upload initial texture state
        glGenTextures(1, &window_0x10.gl_tex_id);
        if (window_0x10.gl_tex_id == 0) {
            const char* name = (char*)&textures[window_0x10.selected_atlas_texture].filename;
            LOG_MSG(error, "Failed to create OpenGL texture for \"%s\"\n", name);
        }

        update_gl_tex(cur_tex, window_0x10.gl_tex_id);
        window_0x10.tex = cur_tex;
        window_0x10.scale = 1.0f;
    }
    else if (memcmp(&cur_tex, &window_0x10.tex, sizeof(cur_tex)) != 0) {
        // The texture changed since last frame, update the OpenGL state
        update_gl_tex(cur_tex, window_0x10.gl_tex_id);
        window_0x10.tex = cur_tex;
    }


    // User input for atlas properties
    const float char_width = ImGui::CalcTextSize("1").x;
    ImGui::SetNextItemWidth(char_width * (sizeof(aName->name) - 1 + 5));
    ImGui::InputText("Atlas Name", &aName->name[0], sizeof(aName->name) - 1);

    ImGui::Text("Atlas uses texture index %d, see 0x15 chunk for offset & format", window_0x10.selected_atlas);

    ImGui::SetNextItemWidth(char_width * 15);
    ImGui::InputU16("Atlas Height", &atlas->height);
    ImGui::SetNextItemWidth(char_width * 15);
    ImGui::InputU16("Atlas Width", &atlas->width);
    ImGui::SetNextItemWidth(char_width * 15);
    ImGui::InputU32("Atlas Texture Count", &atlas->tex_count);

    // Draw the whole atlas
    ImVec2 image_pos = draw_image(window_0x10.gl_tex_id, atlas->width, atlas->height, &window_0x10.use_actual_size_atlas, &window_0x10.scale_atlas, "atlas");

    // Calculate UVs of the selected texture in the atlas
    const ImVec2 uv1 = ImVec2(tex->atlas_texcoords[0], tex->atlas_texcoords[1]);
    const ImVec2 uv0 = ImVec2(uv1.x - ((float)tex->width / atlas->width), uv1.y - ((float)tex->height / atlas->height));

    // Draw a bounding box over a single texture in the atlas
    const ImVec2 atlas_drawn_size = ImVec2(atlas->width, atlas->height) * window_0x10.scale_atlas;
    const ImVec2 start = image_pos + (atlas_drawn_size * uv0);
    const ImVec2 end = image_pos + (atlas_drawn_size * uv1);
    ImGui::GetWindowDrawList()->AddRect(start, end, 0xFF00FF00);

    ImGui::SetNextItemWidth(char_width * (sizeof(tex->filename) - 1 + 5));
    ImGui::InputText("Texture Name", &tex->filename[0], sizeof(tex->filename) - 1);

    ImGui::Text("%dx%d pixels, UV coords (%.3f, %.3f)", tex->height, tex->width, tex->atlas_texcoords[0], tex->atlas_texcoords[1]);

    draw_image(window_0x10.gl_tex_id, tex->width, tex->height, &window_0x10.use_actual_size, &window_0x10.scale, "texture", uv0, uv1);
}

void polaris::chunk::chunk_0x11(const polaris& pol) const noexcept {
    CHUNK_ID_ASSERT(0x11);

    vfile vf = vfile_open(pol.alr_data + offset, size);
    auto* layout = (chunk_layout*)vfile_cur(vf);
    vfile_seek(&vf, sizeof(*layout));
    auto* offsets = (u32*)vfile_cur(vf);

    ImGui::Text("Texture buffer @ 0x%X [%d bytes]", layout->texbuf_offset, layout->texbuf_size);
    ImGui::Text("%d offsets in array:\n", layout->offset_array_size);

    if (ImGui::BeginListBox("Offsets")) {
        for (u32 i = 0; i < layout->offset_array_size; i++) {
            ImGui::Text("0x%X", offsets[i]);
        }
        ImGui::EndListBox();
    }
}

void polaris::chunk::import_dds_0x15(const polaris& pol, const char* path, u32 num_entries, texture_entry* entries) noexcept {
    CHUNK_ID_ASSERT(0x15);

    const texture_entry cur = entries[window_0x15.selected_texture];
    const texture_entry next = entries[window_0x15.selected_texture + 1];
    s64 tex_size = 0;
    if (window_0x15.selected_texture >= num_entries) {
        // This is the last entry, so the best guess is that it takes up the
        // rest of the file
        tex_size = pol.alr_size - cur.data_ptr;
    } else {
        // The most likely texture size is the distance betwen this texture and
        // the next
        tex_size = next.data_ptr - cur.data_ptr;
    }

    window_0x15.tex = image_buf_load(path, window_0x15.tex.data, tex_size);

    // The loaded image might not be an even power of 2, here we round to the
    // nearest one
    const u16 res = MAX(window_0x15.tex.width, window_0x15.tex.height);
    for (u8 i = 0; i < ALR_TEX_POWER_LIMIT; i++) {
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

void polaris::chunk::chunk_0x15(polaris& pol) noexcept {
    CHUNK_ID_ASSERT(0x15);

    // We use the vfile API to handle the chunk data
    vfile vf = vfile_open(pol.alr_data + offset, size);
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
    texture_entry* entry = &entries[window_0x15.selected_texture];
    decoded_text name = {0};
    decode_single32(name.data, entry->text1);
    decode_single32(&name.data[ENCODED_CHAR_COUNT], entry->text2);

    texture cur_tex = convert_tex(pol.alr_data + pol.resbuf_offset, *entry);
    ImGui::Text("Warning: These pixel counts are guesses.\nIf they look wrong, trust your own judgement\nand the 0x10 (texture atlas) window.\n\n");
    ImGui::InputPDString("Texture Name", &entry->text1, &entry->text2);
    ImGui::Text("%dx%d pixels @ resbuf+0x%X\n", cur_tex.height, cur_tex.width, entry->data_ptr);

    const char* format = texformat_str((alr_pixel_format)entry->pixel_format);
    ImGui::Text("Suspected format: %s (code 0x%X)", format, entry->pixel_format);

    if (window_0x15.gl_tex_id == 0) {
        // Create & upload initial texture state
        glGenTextures(1, &window_0x15.gl_tex_id);
        if (window_0x15.gl_tex_id == 0) {
            LOG_MSG(error, "Failed to create OpenGL texture for \"%s\"\n", name);
        }

        update_gl_tex(cur_tex, window_0x15.gl_tex_id);
        window_0x15.tex = cur_tex;
        window_0x15.scale = 1.0f;
    }
    else if (memcmp(&cur_tex, &window_0x15.tex, sizeof(cur_tex)) != 0) {
        // The texture changed since last frame, update the OpenGL state
        update_gl_tex(cur_tex, window_0x15.gl_tex_id);
        window_0x15.tex = cur_tex;
    }


    ImGui::Text("2^(resolution power) = width = height");
    const u8 step_pwr = 1; // Step for the resolution power input
    ImGui::InputU8("Resolution power", &entry->resolution_pwr, step_pwr);
    // This limits resolution to 4096^2, which is plenty for our use case
    entry->resolution_pwr = MIN(entry->resolution_pwr, ALR_TEX_POWER_LIMIT);

    if (ImGui::Button("Import DDS")) {
        // Display the file picker
        nfdu8filteritem_t filters[] = { { "DDS Image", "dds"} };
        char* path = nullptr;
        nfdresult_t result = NFD_OpenDialogU8(&path, filters, ARRAY_SIZE(filters), nullptr);
        if (result == NFD_OKAY && path != nullptr) {
            this->import_dds_0x15(pol, path, num_entries, entries);
        }
        free(path);
    }

    ImGui::SameLine();
    if (ImGui::Button("Export DDS")) {
        // Display the file picker
        nfdu8filteritem_t filters[] = { { "DDS Image", "dds"} };
        char* path = nullptr;
        nfdresult_t result = NFD_SaveDialogU8(&path, filters, ARRAY_SIZE(filters), nullptr, name.data);
        if (result == NFD_OKAY && path != nullptr) {
            img_write(window_0x15.tex, path);
        }
        free(path);
    }

    draw_image(window_0x15.gl_tex_id, window_0x15.tex.width, window_0x15.tex.height, &window_0x15.use_actual_size, &window_0x15.scale, "preview");
    ImGui::EndGroup();
}

void polaris::chunk::send_vertbuf_to_viewport(polaris& pol) noexcept {
    CHUNK_ID_ASSERT(0x16);

    // We use the vfile API to handle the chunk data
    vfile vf = vfile_open(pol.alr_data + offset, size);

    // Skip to entries
    vfile_seek(&vf, sizeof(chunk_generic));
    const u32 num_entries = VFILE_READ(u32, &vf);
    auto* entries = (vertbuf_entry*)vfile_cur(vf);

    const vertbuf_entry entry = entries[window_0x16.selected_vertex_buf];

    // Open ALR buffer
    vf = vfile_open(pol.alr_data, pol.alr_size);

    // Jump to the appropriate data in the resource buffer
    vfile_seek(&vf, pol.resbuf_offset + entry.data_ptr);

    // Setup mesh data
    mesh_view mesh;
    mesh.setup();

    // Upload vertex buffer
    const u8* vertex_buf = (u8*)vfile_cur(vf);
    mesh.update_vertex_buf(vertex_buf, entry.vertex_size * entry.vertex_count);

    // Set vertex attributes
    mesh.attributes[ATTRIBUTE_POSITION] = {
        GL_FLOAT, entry.vertex_size, 0, 3,
    };
    mesh.apply_attributes();

    // Upload the index buffers
    for (chunk idx_chunk : pol.chunks) {
        if (idx_chunk.id == this->id && idx_chunk.offset > this->offset) {
            // We've hit a mesh metadata chunk past our own, so any
            // further index buffers will be garbage data to us. Quit.
            break;
        }

        if (idx_chunk.id != 0x2 || idx_chunk.offset < offset) {
            // We only want index buffer chunks for the current mesh
            continue;
        }

        // Skip to idx_chunk and skip header
        vf.pos = idx_chunk.offset + sizeof(chunk_generic);
        const idxbuf_header header = VFILE_READ(idxbuf_header, &vf);

        // We only want index buffers meant for this vertex buffer
        if (header.vertex_buf != window_0x16.selected_vertex_buf && header.vertex_buf2 != window_0x16.selected_vertex_buf) {
            continue;
        }

        // This seems to be a reliable indicator of triangle strip meshes
        if (header.unk3 == IDX_TYPE_STRIP) {
            mesh.draw_mode = GL_TRIANGLE_STRIP;
        }

        const index_buffer idx_buf = {
            ((u8*)vfile_cur(vf)), idx_chunk.num_indices(pol), header.vertex_buf,
        };
        mesh.add_index_buf(idx_buf);
    }
    pol.viewport.meshes.push_back(mesh);
}

void polaris::chunk::chunk_0x16(polaris& pol) noexcept {
    CHUNK_ID_ASSERT(0x16);

    // We use the vfile API to handle the chunk data
    vfile vf = vfile_open(pol.alr_data + offset, size);
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

    const vertbuf_entry* entry = &entries[window_0x16.selected_vertex_buf];
    ImGui::BeginChild("Vertex Buffer Settings", ImVec2(600, 0));
    if (ImGui::Button("Dump to OBJ")) {
        // Display the file picker
        nfdu8filteritem_t filters[] = { { "3D Model", "obj"} };
        char* path = nullptr;
        nfdresult_t result = NFD_SaveDialogU8(&path, filters, ARRAY_SIZE(filters), nullptr, nullptr);
        if (result == NFD_OKAY && path != nullptr) {
            this->dump_vertex_buf(pol, path, *entry);
        }
        free(path);
    }

    if (ImGui::Button("Send to Viewport")) {
        this->send_vertbuf_to_viewport(pol);
    }

    // Hex editor for vertex buffer entry
    hex_edit.DrawContents((void*)entry, sizeof(*entries));
    ImGui::EndChild();

    ImGui::BeginChild("Vertex Buffer Hex Editor", ImVec2(800, 500));

    // Hex editor for vertex buffer data
    u8* vertbuf = pol.alr_data + pol.resbuf_offset + entry->data_ptr;
    window_0x16.hex_vertbuf.DrawContents(vertbuf, entry->vertex_count * entry->vertex_size);
    ImGui::EndChild();
}

void polaris::chunk::draw(polaris& pol) noexcept {
    if (pol.alr_data == nullptr || pol.alr_size == 0) {
        // There's no data to work on, we can't display any useful data.
        return;
    }

    if (ImGui::BeginTabBar("Chunk Tabs")) {
        if (ImGui::BeginTabItem("Specialized Chunk Editor")) {
            switch (id) {
                case 0x2:
                    this->chunk_0x2(pol);
                    break;
                case 0x3:
                    this->chunk_0x3(pol);
                    break;
                case 0x5:
                    this->chunk_0x5(pol);
                    break;
                case 0x7:
                    this->chunk_0x7(pol);
                    break;
                case 0x10:
                    this->chunk_0x10(pol);
                    break;
                case 0x11:
                    this->chunk_0x11(pol);
                    break;
                case 0x15:
                    this->chunk_0x15(pol);
                    break;
                case 0x16:
                    this->chunk_0x16(pol);
                    break;
                default:
                    // Unimplemented window
                    ImGui::Text("[No special editor available]");
            }
            ImGui::EndTabItem();
        }

        if (ImGui::BeginTabItem("Raw Chunk Data")) {
            // Hex editor for the entire chunk, displayed with correct file offsets
            hex_chunk.DrawContents(pol.alr_data + this->offset, this->size, this->offset);
            ImGui::EndTabItem();
        }
        ImGui::EndTabBar();
    }
}

polaris::chunk::chunk(u32 id, s32 size, uintptr_t offset) noexcept {
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


// =============================================================================
// The rest of this file is for the main Polaris class

polaris::polaris() noexcept {
    // TODO: Add an option to commit on reserve in bobtail
    // TODO: Look into MEM_RESET to reduce impact on page file?

    // "Expand" our reservation from 0 bytes to... not 0.
    this->expand_reservation(reserve_size);
}

void polaris::expand_reservation(s64 new_size) noexcept {
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
    if (alr_data != nullptr) {
        vmem_free(alr_data, reserve_size);
    }
    alr_data = new_buf;
    reserve_size = new_size;
}

void polaris::handle_input_suppression() noexcept {
    if (ImGui::GetIO().WantCaptureMouse) {
        // ImGui wants control of the mouse (it's probably over a window),
        // so we'll suppress the real mouse state this frame.
        input.cursor = prev_input.cursor;
        input.scroll = prev_input.scroll;
        input.click_left = prev_input.click_left;
        input.click_right = prev_input.click_right;
        input.click_middle = prev_input.click_middle;
        input.mouse_button_4 = prev_input.mouse_button_4;
        input.mouse_button_5 = prev_input.mouse_button_5;
    }

    if (ImGui::GetIO().WantCaptureKeyboard) {
        // Save non-keyboard input
        const vec2s cursor = input.cursor;
        const vec2s scroll = input.scroll;
        const bool click_left = input.click_left;
        const bool click_right = input.click_right;
        const bool click_middle = input.click_middle;
        const bool mouse_4 = input.mouse_button_4;
        const bool mouse_5 = input.mouse_button_5;

        const vec2s LS = input.LS;
        const vec2s RS = input.RS;
        const float LT = input.LT;
        const float RT = input.RT;
        const gamepad_t gp = input.gp;

        // Copy over all keyboard input
        input = prev_input;

        // Restore non-keyboard input
        input.cursor = cursor;
        input.scroll = scroll;
        input.LS = LS;
        input.RS = RS;
        input.LT = LT;
        input.RT = RT;
        input.gp = gp;
        input.click_left = click_left;
        input.click_right = click_right;
        input.click_middle = click_middle;
        input.mouse_button_4 = mouse_4;
        input.mouse_button_5 = mouse_5;
    }
}

bool polaris::load_alr(const char* path) noexcept {
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
    if (!file_load_existing(path, alr_data, size)) {
        // Some loading failure, an error message should've been printed
        return false;
    }
    alr_size = size;
    chunks = shatter_alr(alr_data, alr_size);
    this->textures_need_reload = true;
    return true;
}

void polaris::unload_gl_textures() noexcept {
    if (headless) {
        return;
    }
    glDeleteTextures(gl_textures.size(), gl_textures.data());
    gl_textures.clear();
}

bool polaris::save_alr(const char* path) const noexcept {
    FILE* out = fopen(path, "wb");
    if (out == nullptr) {
        return false;
    }

    bool result = true;
    if (fwrite(alr_data, alr_size, 1, out) != 1) {
        // Incomplete write
        result = false;
    }
    fclose(out);

    return result;
}

void polaris::do_menu_bar() noexcept {
    ImGuiViewport* viewport = ImGui::GetMainViewport();
    const float height = ImGui::GetFrameHeight();
    const ImGuiWindowFlags flags = ImGuiWindowFlags_NoScrollbar | ImGuiWindowFlags_MenuBar;

    const bool ctrl_pressed = ImGui::IsKeyDown(ImGuiKey_LeftCtrl) || ImGui::IsKeyDown(ImGuiKey_RightCtrl);
    bool load_alr = ctrl_pressed && ImGui::IsKeyPressed(ImGuiKey_L, false);
    bool save_alr = ctrl_pressed && ImGui::IsKeyPressed(ImGuiKey_S, false);

    if (ImGui::BeginViewportSideBar("MainMenu", viewport, ImGuiDir_Up, height, flags)) {
        if (ImGui::BeginMenuBar()) {
            if (ImGui::BeginMenu("File")) {
                load_alr |= ImGui::MenuItem("Load ALR", "Ctrl-L");
                save_alr |= ImGui::MenuItem("Save ALR", "Ctrl-S");
                ImGui::EndMenu();
            }

            if (ImGui::BeginMenu("View")) {
                ImGuiIO& io = ImGui::GetIO();
                ImGui::InputFloat("Font Size", &io.FontGlobalScale, 0.1f);
                ImGui::EndMenu();
            }

            if (ImGui::BeginMenu("Windows")) {
                ImGui::MenuItem("Viewport", nullptr, &this->viewport.enabled);
                ImGui::MenuItem("Viewport Editor", nullptr, &this->viewport.editor_enabled);
                ImGui::MenuItem("ImGui Demo Window", nullptr, &this->show_demo);
                ImGui::EndMenu();
            }

            ImGui::EndMenuBar();
        }
        ImGui::End();
    }

    if (load_alr) {
        // Display the file picker and load the ALR if a file is picked
        char* path = nullptr;
        const nfdu8filteritem_t filters[] = { { "AL Resource", "alr"} };
        nfdresult_t result = NFD_OpenDialogU8(&path, filters, ARRAY_SIZE(filters), nullptr);
        if (result == NFD_OKAY && path != nullptr) {
            this->load_alr(path);
        }
        free(path);
    }

    if (save_alr) {
        // Display the file picker and save the ALR if a file is picked
        nfdu8filteritem_t filters[] = { { "AL Resource", "alr"} };
        char* path = nullptr;
        nfdresult_t result = NFD_SaveDialogU8(&path, filters, ARRAY_SIZE(filters), nullptr, nullptr);
        if (result == NFD_OKAY && path != nullptr) {
            this->save_alr(path);
        }
        free(path);
    }
}

bool load_gl_textures(polaris* pol) {
    assert(!pol->headless && "Can't load textures in headless mode!");
    assert(pol->resbuf_offset != 0 && "Can't load textures without resbuf offset!");

    polaris::chunk texture_chunk = polaris::chunk(0, 0, 0);
    polaris::chunk atlas_chunk = polaris::chunk(0, 0, 0);

    // Try to find texture and texture atlas metadata, we need both to make a
    // good guess about dimensions.
    for (polaris::chunk chunk : pol->chunks) {
        if (chunk.id == 0x15) {
            texture_chunk = chunk;
        }
        if (chunk.id == 0x10) {
            atlas_chunk = chunk;
        }
    }

    // Read texture chunk data
    vfile vf = vfile_open(pol->alr_data + texture_chunk.offset, texture_chunk.size);
    // Skip over the ID and size fields we already have
    vfile_seek(&vf, sizeof(chunk_generic));
    const u32 num_entries = VFILE_READ(u32, &vf);
    texture_entry* tex_entries = (texture_entry*)vfile_cur(vf);

    pol->unload_gl_textures();
    pol->gl_textures.reserve(num_entries);

    // Read atlas chunk data
    atlas_entry* atlas_entries = nullptr;
    atlas_name* atlas_names = nullptr;
    atlas_header header_atlas = {0};
    if (atlas_chunk.size > 0) {
        vf = vfile_open(pol->alr_data + atlas_chunk.offset, atlas_chunk.size);

        // Skip over the ID and size fields we already have
        vfile_seek(&vf, sizeof(chunk_generic));
        header_atlas = VFILE_READ(atlas_header, &vf);

        // Skip over names
        atlas_names = (atlas_name*)vfile_cur(vf);
        vfile_seek(&vf, sizeof(atlas_name) * header_atlas.atlas_count);

        atlas_entries = (atlas_entry*)vfile_cur(vf);
    }

    bool result = true;
    for (u32 i = 0; i < num_entries; i++) {
        // Convert the ALR texture data to our standard texture struct
        texture cur_tex = convert_tex(pol->alr_data + pol->resbuf_offset, tex_entries[i]);

        if (atlas_entries != nullptr && header_atlas.atlas_count > i) {
            atlas_entry entry = atlas_entries[i];
            // We get better dimension info from the atlas headers, use it!
            // Dimensions from the atlas headers are almost always more
            // accurate, so we always use them unless they're obviously wrong.

            const u32 too_small = 0;
            const u32 too_big = 8192;
            if (entry.width > too_small && entry.width < too_big) {
                cur_tex.width = entry.width;
            }
            if (entry.height > too_small && entry.height < too_big) {
                cur_tex.height = entry.height;
            }
        }

        gl_obj gl_tex_id = 0;
        glGenTextures(1, &gl_tex_id);
        update_gl_tex(cur_tex, gl_tex_id);

        result &= (gl_tex_id != 0);
        pol->gl_textures.push_back(gl_tex_id);
    }

    return result;
}

void polaris::do_gui(GLFWwindow* window) noexcept {
    // Make the entire window a giant docking space
    ImGui::DockSpaceOverViewport();

    if (!headless && textures_need_reload) {
        load_gl_textures(this);
        textures_need_reload = false;
    }

    // We have to wait until we know the graphics context has been created to do
    // graphics-related initialization (since the program may run in headless
    // mode with no graphics context).
    if (!viewport.initialized) {
        // Have the viewport render in full resolution, it'll be downscale when
        // rendered as a texture by ImGui::Image
        int width = 0;
        int height = 0;
        glfwGetFramebufferSize(window, &width, &height);
        viewport.setup(width, height);
    } else {
        // TODO: Is there a good way to get a const& to ourselves?
        if (!viewport.render_contents(window, this)) {
            // We don't want to supress input if the viewport needs it
            this->handle_input_suppression();
        }
    }

    this->do_menu_bar();

    if (this->show_demo) {
        ImGui::ShowDemoWindow(&this->show_demo);
    }

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

    if (ImGui::BeginTable("alr chunks", 3, ImGuiTableFlags_ScrollY | ImGuiTableFlags_Reorderable)) {
        // Make header row that never scrolls away
        ImGui::TableSetupScrollFreeze(0, 1);

        // Setup table header
        ImGui::TableSetupColumn("ID");
        ImGui::TableSetupColumn("Offset");
        ImGui::TableSetupColumn("Size");
        ImGui::TableHeadersRow();

        // Draw a row for each chunk
        for (size_t n = 0; n < chunks.size(); n++) {
            polaris::chunk& chunk = chunks.at(n);
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
            // selectable column because no 2 chunks have the same offset.
            ImGui::TableSetColumnIndex(1);
            char buf[0x10] = {0};
            snprintf(buf, sizeof(buf), "0x%02lX", chunk.offset);
            // The extra flag makes the selection highlight go across the whole table
            if (ImGui::Selectable(buf, chunk.active, ImGuiSelectableFlags_SpanAllColumns)) {
                // Display chunk window
                chunk.active = !chunk.active;
            }

            ImGui::TableSetColumnIndex(2);
            ImGui::Text("0x%X", chunk.size);
        }
        ImGui::EndTable();
    }
    ImGui::End();

    // Draw window for all chunks being displayed right now
    for (u32 i = 0; i < chunks.size(); i++ ) {
        polaris::chunk& chunk = chunks.at(i);
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
        snprintf(buf, sizeof(buf), "0x%X %s Chunk @ 0x%lX ##%u", chunk.id, known_name, chunk.offset, i);

        if (ImGui::Begin(buf, &chunk.active)) {
            chunk.draw(*this);
        }

        ImGui::End();
    }

    // It's the end of the frame for us, save the current input
    prev_input = input;
}

std::vector<polaris::chunk> polaris::shatter_alr(const u8* buf, s64 size) noexcept {
    // Technically we cast away const here, but we don't write any data so it's
    // fine.
    vfile vf = vfile_open((void*)buf, size);
    std::vector<polaris::chunk> out;

    // Loop until we exhaust the buffer or exit early
    u32 prev_id = -1;
    while (!vfile_eof(vf)) {
        // Read chunk data. We have to copy it over 1 field at a time because we
        // don't actually want/need any more of the chunk data.
        const uintptr_t offset = vf.pos; // It's important to save offset before reading
        const u32 id = VFILE_READ(u32, &vf);
        const s32 chunk_size = VFILE_READ(s32, &vf);
        polaris::chunk chunk(id, chunk_size, offset);

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
