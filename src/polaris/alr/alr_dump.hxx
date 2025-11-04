#pragma once
/// @author Torphedo
/// @brief Functions to export ("dump") data from an ALR data structure to standard files.
#include <cstdio>
#include <cglm/struct.h>

#include <common/vfile.h>
#include <common/file.h>
#include <formats/alr.h>
#include "editor_alr.hxx"

namespace al {

void anim_key_info(u32 key_size, ImGuiDataType& frame_type, ImGuiDataType& component_type, u32& num_components);

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

bool dump_animation_maya(const anim_header* anim_chunk, const char* outpath, const char* bone_name);

/// @brief Import a mesh into the ALR
///
/// @param txt OBJ text data
/// @param alr The ALR to import into
/// @param entry The vertex buffer that will be overwritten
bool obj_import(const char* txt, al::resource& alr, vertbuf_entry* entry);

// PINT == Polaris INTermediate file
struct pint_header {
    enum class content_type : u8 {
        NONE,
        // The file has a vertex buffer entry and a vertex buffer
        VERTEX_BUFFER,
        // The file just has ALR chunks in it
        CHUNKS,
    };

    u32 magic = MAGIC('P', 'I', 'N', 'T');
    u16 version = 1;
    content_type type = content_type::NONE;
    u8 reserved[24] = {};
};
static_assert(sizeof(pint_header) == 0x20);

struct pint_content {
    struct vertex_buffer {
        vertbuf_entry entry = {};
        u32 buf_size = 0;
        u8 vertex_buf[];
    };
    static_assert(sizeof(vertex_buffer) == 0x20);
};

} // namespace al
