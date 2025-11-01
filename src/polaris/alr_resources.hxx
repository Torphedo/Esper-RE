#pragma once
/// @file alr_resources.hxx
/// Helpers for managing ALR textures and meshes

#include <map>

#include <common/image.h>
#include <formats/alr.h>
#include "mesh_view.hxx"

/// @brief Gets a human-readable description of the pixel format
const char* texformat_str(alr_pixel_format format);

/// @brief Convert an ALR texture entry into our standard structure
texture convert_tex(u8* resbuf, texture_entry entry);

/// @brief Update an OpenGL texture to render a texture on the CPU.
///
/// This will re-upload the entire texture to the GPU, even if only the data
/// format or dimensions changed (but not the texture buffer).
void update_gl_tex(texture img, gl_obj texture_id);

namespace al {
    class resource;
}

struct texture_manager {
    // Offset of the 0x15 chunk
    u32 texheader_offset = 0;

    // Offset of the 0x10 chunk
    u32 atlasheader_offset = 0;

    std::map<u32, gl_obj> gl_tex_map;

    gl_obj get(al::resource& alr, u32 idx) noexcept;
    bool get_material(al::resource& alr, u32 material_header_offset, u32 idx, chunk_0x1_entry** entry_out) const noexcept;
    bool get_material(al::resource& alr, u32 material_header_offset, u32 idx, chunk_0x1_entry* entry_out) const noexcept;

    void invalidate(u32 idx) noexcept;
    void destroy() noexcept;

    ~texture_manager() noexcept;
};

mesh_view mesh_at_idx(const al::resource& alr, u32 idx, u32 vertbuf_idx);