#pragma once
/// @author Torphedo
/// @brief Functions to export ("dump") data from an ALR data structure to standard files.
#include <cstdio>

#include <common/vfile.h>

namespace al {

void dump_armature(FILE* f, vfile armature_data);

} // namespace al
