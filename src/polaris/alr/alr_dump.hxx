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

/// Extract information about an animation key based on its size
/// @param key_size The size of the animation keys you're working with
/// @param frame_type Output to receive the data type of the frame value.
/// @param component_type Output to receive the data type of the components
/// @param num_components Output to receive the number of components in the key
void anim_key_info(u32 key_size, ImGuiDataType& frame_type, ImGuiDataType& component_type, u32& num_components);

/// Generically read an animation key
/// @param key Pointer to key data
/// @param key_size Size of a single animation key, as given in the ALR data
/// @param frame_out Optional output location to receive the key's frame value
/// @param next_key_out Optional output location to receive pointer to next key
/// @return Key converted to floating point. Unused components are left as 0.
vec3s anim_read_key(const u8* key, u32 key_size, float* frame_out, const u8** next_key_out);

/// Find an animation chunk for the specified joint in the specified animation
/// @param alr ALR data to parse
/// @param alr_size Size of ALR buffer
/// @param idx Internal animation ID
/// @param joint_idx Index of the joint to find the animation for
/// @return Offset to the animation chunk, or -1 on failure
s32 animation_by_idx(const u8* alr, u32 alr_size, u32 idx, u32 joint_idx);

/// Get a transform of a joint at a specific frame of the specified animation
/// @param alr ALR data to parse
/// @param alr_size Size of ALR buffer
/// @param anim_id Internal animation ID (within the ALR)
/// @param joint_idx Index of joint being animated
/// @param cur_frame Current animation frame
/// @return Transform to right-multiply with joint transform
mat4s anim_xform_for_joint(u8* alr, u32 alr_size, u32 anim_id, s32 joint_idx, float cur_frame);

/// Export an armature chunk to a COLLADA (.dae) file
/// @param f File stream to output to
/// @param armature_data Buffer view pointing to an armature (ID 0x3) chunk
void dump_armature_dae(FILE* f, vfile armature_data);

/// @brief Calculate transformation matrix from a joint's position/rotation/scale
///
/// This function only considers the single joint. To place the joint correctly,
/// all transforms up the chain of parents need to be applied.
/// @param joint The joint to calculate the transform from
/// @return 4x4 transformation matrix
mat4s transform_from_joint(const joint_t& joint);

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

/// Export an animation chunk to an Autodesk Maya (.anim) animation file
///
/// These files are usable outside of Maya, you can find a Blender plugin here:
/// https://github.com/PositionWizard/Blender_io-scene-ANIM
/// Thanks to Czarpos for the plugin.
/// @param anim_chunk
/// @param outpath
/// @param bone_name
/// @return
bool dump_animation_maya(const anim_header* anim_chunk, const char* outpath, const char* bone_name);

/// @brief Import a mesh into the ALR
///
/// @param txt OBJ text data
/// @param alr The ALR to import into
/// @param vertbuf_chunk_offset Offset of the 0x16 chunk to import into
/// @param entry_idx The index of the vertex buffer to replace within the chunk
bool obj_import(const char* txt, al::resource& alr, u32 vertbuf_chunk_offset, u32 entry_idx);

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
