#include "alr_dump.hxx"
#include <vector>
#include <cglm/struct.h>
#include <imgui_internal.h>

#include <common/vfile.h>
#include <common/file.h>

#include <formats/alr.h>
#include <formats/pd_common.h>

#include <polaris/mesh_view.hxx>
#include <polaris/version.h>
#include "editor_alr.hxx"

namespace al {

void anim_key_info(u32 key_size, ImGuiDataType& frame_type, ImGuiDataType& component_type, u32& num_components) {
    frame_type = ImGuiDataType_COUNT;
    component_type = ImGuiDataType_COUNT;

    switch (key_size) {
    // Integer keys
    case 3:
    case 5:
    case 7:
        frame_type = ImGuiDataType_U8;
        component_type = ImGuiDataType_U16;
        break;

    // Floating point keys
    case 8:
    case 12:
    case 16:
        frame_type = component_type = ImGuiDataType_Float;
    default:
        break;
    }

    const u32 component_size = ImGui::DataTypeGetInfo(component_type)->Size;
    const u32 frame_size = ImGui::DataTypeGetInfo(frame_type)->Size;

    // We know component and frame value size, so we can find out the # of components
    num_components = (key_size - frame_size) / component_size;
}

// IMPORTANT: If Blender complains and won't import the DAE, make sure you
// haven't accidentally made the XML start with a newline. If you do, Blender's
// XML parser will freak out.
// Be careful with the raw string literal, you can't escape newlines.
const char* DAE_HEADER = R"(<?xml version="1.0" encoding="utf-8"?>
<COLLADA xmlns="http://www.collada.org/2005/11/COLLADASchema" version="1.4.1" xmlns:xsi="http://www.w3.org/2001/XMLSchema-instance">
  <asset>
    <contributor>
      <authoring_tool>Polaris</authoring_tool>
    </contributor>
    <unit name="meter" meter="1"/>
    <up_axis>Z_UP</up_axis>
  </asset>
  <library_images/>
  <library_controllers/>
  <library_visual_scenes>
    <visual_scene id="Scene" name="Scene">
)";

const char* DAE_FOOTER = R"(
    </visual_scene>
  </library_visual_scenes>
</COLLADA>
)";

mat4s transform_from_joint(const joint_t & joint) {
    const mat4s pos = glms_translate_make(*(vec3s*)&joint.position);
    const mat4s rot = glms_euler_zyx(*(vec3s*)&joint.rotation);

    mat4s transform = glms_mat4_mul(pos, rot);
    transform = glms_scale(transform, *(vec3s*)&joint.scale);

    return transform;
}

// Just the game's structure, but made so that we can traverse down from the
// root instead of up from the leaves
struct joint_tree {
    joint_t joint;
    std::vector<u32> children;
};

void xml_dump_joint(FILE* f, const joint_tree* joints, u32 idx) {
    const joint_tree& node = joints[idx];
    if (node.joint.name == UINT32_MAX && node.children.empty()) {
        // Unnamed node that won't affect the rest of the skeleton
        return;
    }

    decoded_text name = {0};
    decode_single32(name.data, node.joint.name);
    fprintf(f, R"(<node id="%s_%d" name="%s_%d" sid="%s_%d" type="JOINT">%c)",
        name.data, idx, name.data, idx, name.data, idx, '\n');

    // We need the transform of this bone in the bind pose - the "original"
    // pose of the skeleton (usually a T-pose or A-pose).
    mat4s xform = transform_from_joint(node.joint);

    // 3D software wants the inverse bind pose transform
    // We transpose because DAE is row-major, and we're column-major
    const mat4s inv_bind_xform = glms_mat4_transpose(glms_mat4_inv(xform));
    fprintf(f, "<matrix sid=\"transform\">");
    const float* raw = (float*)&inv_bind_xform;
    for (u32 i = 0; i < (sizeof(mat4s) / sizeof(float)); i++) {
        fprintf(f, "%f ", raw[i]);
    }
    fprintf(f, "</matrix>");

    for (u32 i : node.children) {
        xml_dump_joint(f, joints, i);
    }

    fprintf(f, "\n</node>\n");
}

void dump_armature_dae(FILE* f, vfile armature_data) {
    if (!armature_data.ptr || armature_data.size < sizeof(chunk_armature)) {
        LOG_MSG(error, "Not exporting armature because there was nothing to export.\n");
        return;
    }
    fprintf(f, DAE_HEADER);
    fprintf(f, "%s\n", R"(<node id="Armature" name="Armature" type="NODE">)");

    vfile_seek(&armature_data, sizeof(chunk_generic));

    const chunk_armature header = VFILE_READ(chunk_armature, &armature_data);
    auto* joints = (joint_t *) vfile_cur(armature_data);

    std::vector<joint_tree> roots(header.joint_count);
    for (u32 i = 0; i < header.joint_count; i++) {
        joint_t cur_joint = joints[i];
        roots[i].joint = cur_joint;

        // Bounds check
        if (cur_joint.parent_idx < 0 || cur_joint.parent_idx > header.joint_count - 1) {
            continue;
        }

        roots[cur_joint.parent_idx].children.push_back(i);
    }

    for (u32 i = 0; i < roots.size(); i++) {
        const bool no_name = roots[i].joint.name == UINT32_MAX;
        const bool is_root = roots[i].joint.parent_idx < 0;
        const bool out_of_bounds = roots[i].joint.parent_idx > s32(roots.size() - 1);
        const bool has_children = roots[i].children.size() > 0;

        if (!is_root || out_of_bounds || (no_name && !has_children)) {
            continue;
        }

        xml_dump_joint(f, roots.data(), i);
    }

    fprintf(f, "</node>\n");
    fprintf(f, DAE_FOOTER);
}

void fprint_obj_idx(FILE* out, bool uv, bool normal, u16 idx) {
    fprintf(out, "%hu", idx);
    if (uv) {
        fprintf(out, "/%hu", idx);
    }
    if (normal) {
        fprintf(out, "/%hu", idx);
    }
    fprintf(out, " ");
}

void dump_idx_buf(const u8* alr_data, u32 offset, FILE* out, bool has_uvs) {
    vfile vf = vfile_open((void*)(alr_data + offset), 0x10);

    // We use the temporary size until we can get the actual size here
    const chunk_generic generic_header = VFILE_READ(chunk_generic, &vf);
    vf.size = generic_header.size; // This just sets the limit of how much we can read
    const idxbuf_header header = VFILE_READ(idxbuf_header, &vf);

    const u16* indices = (u16*)vfile_cur(vf);
    for (s32 i = 2; i < header.num_indices; i++) {
        // We add 1 because OBJ indices start at 1
        u16 idx1 = indices[i - 2] + 1;
        u16 idx2 = indices[i - 1] + 1;
        u16 idx3 = indices[i] + 1;

        if (idx1 == idx2 || idx1 == idx3 || idx2 == idx3) {
            // Triangle strips will repeat 1 index to create a triangle with an
            // area of 0, which is used to end a strip and start another.
            // We skip these since they're not part of the geometry.
            continue;
        }

        fprintf(out, "f ");
        fprint_obj_idx(out, has_uvs, false, idx1);
        fprint_obj_idx(out, has_uvs, false, idx2);
        fprint_obj_idx(out, has_uvs, false, idx3);
        fprintf(out, "\n");

        if (header.primitive_type != IDX_TYPE_STRIP) {
            // For triangle strips we advance 1 each loop, but for normal
            // triangles we need to make up the difference to advance 3 each loop.
            i += 2;
        }
    }
}

void dump_vertex_buf(const resource& alr, const char* path, u32 vertchunk_offset, u32 vert_entry_idx) {
    // Dump to OBJ
    FILE *out = fopen(path, "wb");
    if (out != nullptr) {
        // Open resource buffer
        vfile vf = vfile_open(alr.data, alr.alr_size);
        vfile_seek(&vf, vertchunk_offset);
        const auto genheader = VFILE_READ(chunk_generic, &vf);
        const u32 vert_entries = VFILE_READ(u32, &vf);
        const vertbuf_entry* entries = (vertbuf_entry*)vfile_cur(vf);
        const vertbuf_entry entry = entries[vert_entry_idx];

        // Jump to the appropriate data
        vf.pos = alr.resbuf_offset;
        vfile_seek(&vf, entry.data_ptr);
        bool has_uvs = false;
        for (u32 i = 0; i < entry.vertex_count; i++) {
            const s64 next_pos = vf.pos + entry.vertex_size;
            // Read the vertex (this abstracts away the many different formats)
            const std_vertex vert = standardize_pd_vertex(vfile_cur(vf), entry.format);

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

            if (vert.normal.has_value()) {
                const vec3s normal = vert.normal.value();
                fprintf(out, "vn %f %f %f\n", normal.x, normal.y, normal.z);
            }

            // Skip to the next vertex
            vf.pos = next_pos;
        }

        // Vertices are dumped, now for indices
        for (al::resource::chunk idx_chunk : alr.chunks) {
            if (idx_chunk.id == 0x16 && idx_chunk.offset > vertchunk_offset) {
                // We've hit a mesh metadata chunk past our own, so any
                // further index buffers will be garbage data to us. Quit.
                break;
            }

            if (idx_chunk.id != 0x2) {
                // We only want index buffer chunks
                continue;
            }

            if (idx_chunk.offset < vertchunk_offset) {
                // This index buffer is from a previous mesh, so it's
                // garbage data to us. Skip.
                continue;
            }

            // Skip to idx_chunk and skip header
            vf.pos = idx_chunk.offset;
            vfile_seek(&vf, sizeof(chunk_generic));
            const idxbuf_header header = VFILE_READ(idxbuf_header, &vf);

            // We only want index buffers meant for this vertex buffer
            if (header.vertex_buf != vert_entry_idx) {
                continue;
            }

            fprintf(out, "\ng idxbuf_0x%lx\n", idx_chunk.offset);
            al::dump_idx_buf(alr.data, idx_chunk.offset, out, has_uvs);
        }

        // Cleanup
        fclose(out);
    }
}

const char *anim_boilerplate = R"(animVersion 1.1;
mayaVersion %s; # This is actually the Polaris version
timeUnit film; # Frames
linearUnit m;
angularUnit deg;
startTime 0;
endTime %d;
)";

void fprint_anim_boilerplate(FILE* f, const char* polaris_version, float anim_length) {
    fprintf(f, anim_boilerplate, polaris_version, (u32)anim_length);
}

void fprintf_anim_key(FILE* f, float frame, float val, u32 tan_locked, u32 weight_locked, u32 breakdown) {
    fprintf(f, "    %f %f auto auto %d %d %d;", frame, val, tan_locked, weight_locked, breakdown);
}

enum anim_key_type : u8 {
    KEY_TRANSLATE,
    KEY_ROTATE,
    KEY_SCALE,
    KEY_TYPE_ENUM_MAX,
};

void fprintf_anim_data_start(FILE* f, anim_key_type type, char axis, const char* joint_name) {
    type = MIN(type, KEY_SCALE); // Keep in bounds
    const char* anim_types[KEY_TYPE_ENUM_MAX] = {
        "translate",
        "rotate",
        "scale",
    };
    const char* unit_types[KEY_TYPE_ENUM_MAX] = {
        "linear",
        "angular",
        "unitless",
    };

    const char* type_str = anim_types[type];
    const char* units = unit_types[type];
    s32 attr_idx = (type * 3) - 1;
    switch (axis) {
    case 'Z':
        attr_idx++;
        [[fallthrough]];
    case 'Y':
        attr_idx++;
        [[fallthrough]];
    case 'X':
        attr_idx++;
    default:
        break;
    }

    fprintf(f, "anim %s.%s%c %s%c ", type_str, type_str, axis, type_str, axis);
    fprintf(f, "%s 0 1 %d;\n", joint_name, attr_idx);
    fprintf(f, R"(animData {
  input time;
  output %s;
  weighted 1;
  preInfinity constant;
  postInfinity constant;
  keys {)", units);
    fprintf(f, "\n");
}

void fprintf_anim_data_end(FILE* f) {
    fprintf(f, "\n  }\n}\n\n");
}

void dump_anim_channel(u32 key_size, u32 num_keys, const void* keydata, FILE* f, anim_key_type type, const char* bone_name) {
    ImGuiDataType frame_type = ImGuiDataType_COUNT;
    ImGuiDataType component_type = ImGuiDataType_COUNT;
    u32 num_components = 0;
    anim_key_info(key_size, frame_type, component_type, num_components);
    const u32 frame_size = ImGui::DataTypeGetInfo(frame_type)->Size;
    const u32 component_size = ImGui::DataTypeGetInfo(component_type)->Size;
    const char* axes = "XZY";

    vfile vf = vfile_open((void*)keydata, num_keys * key_size);
    for (u32 i = 0; i < num_components; i++) {
        if (num_keys == 0) {
            break; // Don't print empty key blocks, they break the parser
        }

        fprintf_anim_data_start(f, type, axes[i], bone_name);
        for (u32 j = 0; j < num_keys; j++) {
            const u32 next_key_pos = vf.pos + key_size;
            float frame = 0.0f;
            switch (frame_type) {
                case ImGuiDataType_Float:
                    frame = VFILE_READ(float, &vf);
                    break;
                case ImGuiDataType_U8:
                    frame = VFILE_READ(u8, &vf);
                    break;
                default:
                    LOG_MSG(warning, "Unknown key format with size %d!\n", key_size);
                    break;
            }

            // Skip to component we want
            float component = 0.0f;
            vfile_seek(&vf, component_size * i);
            switch (component_type) {
                case ImGuiDataType_Float:
                    component = VFILE_READ(float, &vf);
                    if (type == KEY_ROTATE) {
                        component = glm_deg(component); // Convert to degrees
                    }
                    break;
                case ImGuiDataType_U16:
                    component = VFILE_READ(s16, &vf);
                    component /= float(INT16_MAX);
                    if (type == KEY_ROTATE) {
                        component *= 180.0f; // Convert to degrees
                    }
                    break;
                default:
                    LOG_MSG(warning, "Unknown key format with size %d!\n", key_size);
                    break;
            }

            fprintf_anim_key(f, frame, component, 1, 0, 0);
            if (j < (num_keys - 1)) {
                // This is load-bearing. If we have a trailing newline in our
                // key {} block, the Blender plugin will crash.
                fprintf(f, "\n");
            }
            vf.pos = next_key_pos;
        }

        fprintf_anim_data_end(f);
        vf.pos = 0;
    }
}

bool dump_animation_maya(const anim_header* anim_chunk, const char* outpath, const char* bone_name) {
    const bool exists = file_exists(outpath);
    FILE* f = fopen(outpath, "ab");
    if (!f) {
        return false;
    }

    if (!exists) {
        fprint_anim_boilerplate(f, POLARIS_VERSION, anim_chunk->length);
    }

    vfile vf = vfile_open((void*)anim_chunk, anim_chunk->size);
    vfile_seek(&vf, sizeof(*anim_chunk));

    dump_anim_channel(anim_chunk->translation_key_size, anim_chunk->translation_key_count, vfile_cur(vf), f, KEY_TRANSLATE, bone_name);
    vfile_seek(&vf, anim_chunk->translation_key_size * anim_chunk->translation_key_count);

    dump_anim_channel(anim_chunk->rotation_key_size, anim_chunk->rotation_key_count, vfile_cur(vf), f, KEY_ROTATE, bone_name);
    vfile_seek(&vf, anim_chunk->rotation_key_size * anim_chunk->rotation_key_count);

    fclose(f);
    return true;
}

void obj_get_info(const char* txt, u32& out_vert_count, u32& out_idx_count, bool& out_has_uvs) {
    assert(txt != nullptr);
    u16 vert_count = 0;
    u16 idx_count = 0;
    bool has_uvs = false;

    const char* line = txt;
    while (*line != 0x00) {
        // Find end of line (NUL or newline)
        const char* line_end = strchr(line, '\n');
        if (!line_end) {
            break;
        }

        if (line[0] != '#') {
            // Faces
            if (strncmp(line, "f ", 2) == 0) {
                // Each face requires 3 indices
                idx_count += 3;
            }
            // Vertex position
            if (strncmp(line, "v ", 2) == 0) {
                vert_count++;
            }
            // vt == vertex texture coordinate
            if (strncmp(line, "vt ", 3) == 0) {
                has_uvs = true;
            }
        }

        // Advance to next line
        line = line_end + 1;
    }

    out_has_uvs = has_uvs;
    out_vert_count = vert_count;
    out_idx_count = idx_count;
}

bool obj_import(const char* txt, al::resource& alr, vertbuf_entry* entry) {
    bool has_uvs = false;
    u32 vert_count = 0;
    u32 idx_count = 0;
    obj_get_info(txt, vert_count, idx_count, has_uvs);
    if (vert_count > entry->vertex_count) {
        // Make space for the extra data
        const u32 diff = vert_count - entry->vertex_count;
        if (!alr.shift_vertbuf(entry->data_ptr, diff * entry->vertex_size)) {
            return false;
        }
    }

    u32 vert_pos = 0;
    u8* vertices = alr.resource_buffer() + entry->data_ptr;
    const char* line = txt;
    while (*line != 0x00) {
        // Find end of line (NUL or newline)
        const char* line_end = strchr(line, '\n');
        if (!line_end) {
            break;
        }

        if (line[0] != '#') {
            // Faces
            if (strncmp(line, "f ", 2) == 0) {
            }

            // Vertex position
            if (strncmp(line, "v ", 2) == 0) {
                vec3s pos = {};
                sscanf(line, "v %f %f %f", &pos.x, &pos.y, &pos.z);
                memcpy(vertices, &pos.raw, sizeof(pos.raw));
                vertices += entry->vertex_size;
                vert_pos++;
            }

            // vt == vertex texture coordinate
            if (strncmp(line, "vt ", 3) == 0) {
            }
        }

        // Advance to next line
        line = line_end + 1;
    }

    for (u32 i = vert_pos; i < entry->vertex_count; i++) {
        // Wipe vertex position data
        memset(vertices, 0, sizeof(vec3s));
        vertices += entry->vertex_size;
    }

    return true;
}

} // namespace al