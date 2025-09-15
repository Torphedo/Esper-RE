#include "alr_dump.hxx"
#include <vector>
#include <set>
#include <cglm/struct.h>

#include <common/vfile.h>

#include <formats/alr.h>
#include <formats/pd_common.h>
#include "editor_alr.hxx"
#include "pd_mesh.hxx"

namespace al {

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
    const mat4s rot = glms_euler_xyz(*(vec3s*)&joint.rotation);

    mat4s transform = glms_mul(rot, pos);
    transform = glms_scale(transform, *(vec3s*)&joint.scale);

    return transform;
}

// Just the game's structure, but made so that we can traverse down from the
// root instead of up from the leaves
struct joint_tree {
    joint_t joint;
    std::vector<u32> children;
};

void xml_dump_joint(FILE* f, const joint_tree* joints, u32 idx, mat4s parent_xform = glms_mat4_identity()) {
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
        xml_dump_joint(f, joints, i, xform);
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
        fprint_obj_idx(out, idx1, has_uvs, false);
        fprint_obj_idx(out, idx2, has_uvs, false);
        fprint_obj_idx(out, idx3, has_uvs, false);
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

} // namespace al