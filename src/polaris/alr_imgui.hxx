#pragma once
#include <imgui.h>
#include <imgui_hex_editor.h>

#include <common/vfile.h>
#include <formats/alr.h>

/// @brief Stateless ImGui editor menus for ALR structures

namespace al {
    enum {
        // The power of 2 to limit texture resolutions to
        // e.g. 2^12 = 4096
        TEX_POWER_LIMIT = 12,
    };

    bool edit_chunk_layout(chunk_layout& layout);

    bool edit_texture_entry(texture_entry& entry);

    /// @brief Edit an entry representing a texture atlas
    bool edit_atlas_entry(atlas_entry& entry, atlas_name& name_entry);

    /// @brief Edit an entry representing a texture within an atlas
    bool edit_atlas_texture(atlas_tex_entry& entry);

    bool edit_vertbuf_entry(vertbuf_entry& entry);

    /// @param armature_vf A vfile view of the 0x3 (armature) chunk. Position value is ignored.
    /// @param hex_edit Hex editor state for this entry
    bool edit_joint_t(joint_t& joint, vfile armature_vf, MemoryEditor& hex_edit);

    /// @brief Create input boxes for ALR animation keys of any type or size
    ///
    /// @param key_size The size of each animation key
    /// @param key_count The number of animation keys
    /// @param keyframes The address of the first key
    /// @param label_extra A unique name of this set of keyframes (must not be
    /// nullptr). This won't be displayed, only used to give the input boxes a
    /// unique ID in ImGui.
    void edit_keyframes(u16 key_size, u16 key_count, void* keyframes, const char* label_extra);
} // namespace al
