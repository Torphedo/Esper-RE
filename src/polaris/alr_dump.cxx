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

    mat4s transform = glms_mul(pos, rot);
    transform = glms_scale(transform, *(vec3s*)&joint.scale);

    return transform;
}

void xml_dump_joint(FILE* f, const char* name, const mat4s& xform) {
    fprintf(f, R"(<node id="%s" name="%s" sid="%s" type="JOINT">%c)",
        name, name, name, '\n');

    fprintf(f, "<matrix sid=\"transform\">");
    const float* raw = (float*)&xform;
    for (u32 i = 0; i < (sizeof(mat4s) / sizeof(float)); i++) {
        fprintf(f, "%f ", raw[i]);
    }
    fprintf(f, "</matrix>");
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

    for (u32 i = 0; i < header.joint_count; i++) {
        if (joints[i].name == UINT32_MAX) {
            continue; // Skip bones with no name
        }

        joint_t cur_joint = joints[i];
        // Need to get the name now, before [cur_joint] changes
        decoded_text name = {0};
        decode_single32(name.data, cur_joint.name);

        // We need the transform of this bone in the bind pose - the "original"
        // pose of the skeleton (usually a T-pose or A-pose).
        mat4s xform = transform_from_joint(cur_joint);

        // Apply parent transforms to get final bone transform
        while (cur_joint.parent_idx > 0) {
            cur_joint = joints[cur_joint.parent_idx];
            mat4s xform_parent = transform_from_joint(cur_joint);
            xform = glms_mul(xform, xform_parent);
        }

        // 3D software wants the inverse bind pose transform
        xform = glms_mat4_inv(xform);
        xform = glms_mat4_transpose(xform); // DAE is row-major, we're column-major

        xml_dump_joint(f, name.data, xform);
    }

    fprintf(f, "</node>\n");
    fprintf(f, DAE_FOOTER);
}

} // namespace al