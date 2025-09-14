#pragma once
/// @author Torphedo
/// @brief Functions to export ("dump") data from an ALR data structure to standard files.
#include <cstdio>
#include <cglm/struct.h>

#include <common/vfile.h>
#include <formats/alr.h>

namespace al {

void dump_armature(FILE* f, vfile armature_data);
mat4s transform_from_joint(const joint_t & joint);

} // namespace al
