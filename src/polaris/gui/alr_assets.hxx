#pragma once
/// @file alr_resources.hxx
/// Helpers for managing ALR textures and meshes
#include <unordered_map>

#include <common/image.h>
#include <formats/alr.h>
#include <gui/mesh_view.hxx>

typedef struct {
    chunk_0x1_header* mat_chunk; // Materials
    chunk_armature*  skel_chunk;
    vertbuf_header*  vert_chunk;
    idxbuf_header*   idx_chunk;
}alr_model_desc;

/// @brief Gets a human-readable description of the pixel format
const char* texformat_str(alr_pixel_format format);

/// @brief Convert an ALR texture entry into our standard structure
texture convert_tex(u8* resbuf, texture_entry entry);

/// @brief Update an OpenGL texture to render a texture on the CPU.
///
/// This will re-upload the entire texture to the GPU, even if only the data
/// format or dimensions changed (but not the texture buffer).
void update_gl_tex(texture img, gl_obj texture_id);

struct texture_manager {
    // Offset of the texture chunk
    u32 texheader_offset = 0;

    std::unordered_map<u32, gl_obj> gl_tex_map;

    gl_obj get(alr::file& alr, u32 idx) noexcept;
    bool get_material(alr::file& alr, u32 material_header_offset, u32 idx, chunk_0x1_entry** entry_out) const noexcept;
    bool get_material(alr::file& alr, u32 material_header_offset, u32 idx, chunk_0x1_entry* entry_out) const noexcept;

    void invalidate(u32 idx) noexcept;
    void destroy() noexcept;

    ~texture_manager() noexcept {
        destroy();
    }
};

namespace alr {
    struct mesh {
        // 1. Loop over all index buffers.
        // 2. Index into a vertbuf_entry* and bind the corresponding VAO
        // 3. Use index buffer info to get the final animated xform, or just use
        //    the optional pos/rot pointers
        // 4. Index into materials to get texture IDs, then fetch via manager
        alr_model_desc chunks;
        std::vector<mesh_view> vaos;
        std::vector<index_buffer> idxbufs;

        void render(file& alr, u32 anim_id, float frame, mat4s cam_xform, gl_obj u_pvm, gl_obj u_divisor) const noexcept;
        void destroy() noexcept;

        // Need:
        // - VAO
        // - Index buffers
        // - Texture (ideally material info)
        // - Transform (need ALR index buffer for the joint ID, or pos/rot pointers)
    };
}

alr::mesh mesh_at_idx(const alr::file& alr, u32 idx);
