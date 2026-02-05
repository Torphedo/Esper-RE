#pragma once
/// @author Torphedo
/// @brief Functions to export ("dump") data from an ALR data structure to standard files.
#include "alr_file.hxx"
#include <formats/ssb.h>

namespace alr {

/// Extract information about an animation key based on its size
/// @param key_size The size of the animation keys you're working with
/// @param frame_type Output to receive the data type of the frame value.
/// @param component_type Output to receive the data type of the components
/// @param num_components Output to receive the number of components in the key
void anim_key_info(u32 key_size, data_type& frame_type, data_type& component_type, u32& num_components);

/// Generically read an animation key
/// @param key Pointer to key data
/// @param key_size Size of a single animation key, as given in the ALR data
/// @param frame_out Optional output location to receive the key's frame value
/// @param next_key_out Optional output location to receive pointer to next key
/// @return Key converted to floating point. Unused components are left as 0.
vec3s anim_read_key(const u8* key, u32 key_size, float& frame_out, const u8** next_key_out = nullptr);

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

/// @brief Calculate the bind pose transform of a joint
///
/// This function does not apply animation.
/// @param joint_header Skeleton data
/// @param joint_idx The joint whose transform will be calculated
/// @return Final local space transform of the joint
mat4s joint_bind_xform(const chunk_armature* joint_header, s32 joint_idx);

/// @brief Dump materials to a file in MTL format (used with OBJ)
/// @param f Standard C file to output to
/// @param materials ALR material array
/// @param num_mats Number of materials in the array
/// @param texture_names List of all texture names (must be in same order as in the ALR)
/// @param num_names Number of texture names in the array
void dump_materials_obj(FILE* f, const material_entry* materials, u32 num_mats, const decoded_text* texture_names, u32 num_names);

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
void dump_vertex_buf(const file& alr, const char* path, u32 vertchunk_offset, u32 vert_entry_idx);

/// @brief Export all textures to DDS files in a "textures" folder in the current working directory.
/// @param alr ALR to dump textures from
bool dump_all_textures(const file& alr);

/// @brief Export texture assignment information to a .mtl file for use with OBJ models
/// @param alr ALR to dump materials from
/// @param output_path Path where the .mtl file will be saved
bool dump_all_materials(const file& alr, const char* output_path);

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
bool obj_import(const char* txt, file& alr, u32 vertbuf_chunk_offset, u32 entry_idx);

typedef struct {
    bool has_uvs;
    u32 vert_count;
    u32 idx_count;

    u16* indices;
    vec3s* positions;
    vec2s* texcoords;
}parsed_obj;

} // namespace al
