#pragma once
/// @author Torphedo
/// @brief Functions to export ("dump") data from an ALR data structure to standard files.
#include <cstdio>
#include <cglm/struct.h>

#include <common/vfile.h>
#include <formats/alr.h>
#include "editor_alr.hxx"

namespace al {

void dump_armature_dae(FILE* f, vfile armature_data);
mat4s transform_from_joint(const joint_t & joint);

/// @brief Dump index buffer in OBJ format
///
/// @param alr_data ALR file data
/// @param offset Offset of the 0x2 chunk to dump
/// @param out The already open, writable OBJ file handle
/// @param has_uvs Whether the rest of your OBJ data will have UVs
void dump_idx_buf(const u8* alr_data, u32 offset, FILE* out, bool has_uvs);

/// @brief Dump vertex data to an OBJ file
///
/// @param alr ALR file where the data is
/// @param path Path to save the OBJ file
/// @param vertchunk_offset Offset of the 0x16 chunk in the ALR
/// @param vert_entry_idx Index of the vertex buffer entry in the 0x16 chunk
void dump_vertex_buf(const al::resource& alr, const char* path, u32 vertchunk_offset, u32 vert_entry_idx);

} // namespace al
