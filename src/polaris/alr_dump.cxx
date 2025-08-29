#include "alr_dump.hxx"
#include <vector>
#include <set>
#include <cglm/struct.h>

#include <formats/alr.h>
#include <formats/pd_common.h>

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

void dump_armature(FILE* f, vfile armature_data) {
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

} // namespace al