#pragma once
#include <optional>
#include <vector>
#include <cglm/struct.h>

#include <common/vfile.h>
#include "render_context.hxx"
#include <alr/alr_file.hxx>
#include "alr_assets.hxx"

/// @brief Dear ImGui menu to edit an attribute
///
/// This doesn't call ImGui::Begin()/End(), use it in an existing window.
void edit_menu(vertex_attribute& attr);

// Static index buffer wrapper
struct index_buffer {
    u32 idx_chunk_offset = 0;

    // OpenGL object to bind to GL_ELEMENT_ARRAY_BUFFER
    gl_obj obj = 0;

    bool active = true; // Whether to render this index buffer

    const idxbuf_header* original_data(const void* alr_data) const noexcept {
        return (idxbuf_header*)(uintptr_t(alr_data) + idx_chunk_offset);
    }

    index_buffer() = default;
    index_buffer(u32 offset) noexcept : idx_chunk_offset(offset) {

        return;
    }
};

// Static vertex buffer wrapper
struct vertex_buffer {
    vertex_attribute attributes[ATTRIBUTE_ENUM_MAX] = {};
    // The offset of the *entry*, not of the 0x16 chunk.
    u16 vertex_size = 0;
    u32 buffer_size = 0;
    const void* buffer = nullptr;
    gl_obj vbo = 0;
    gl_obj vao = 0;

    // Texture coordinates are divided by this value in the shader before use
    u32 uv_divisor = 1;

    // Whether to render this mesh
    bool active = true;
    bool wireframe = false;

    bool initialized = false;

    // Constructor
    // Check the [initialized] field to see if it failed
    bool setup() noexcept;
    // Destructor
    void destroy() noexcept;

    /// @brief Upload/update the GPU-side vertex buffer.
    ///
    /// @param buf The vertex buffer to upload
    /// @param size The size of the vertex buffer
    bool upload_vertex_buf(const u8* buf, u32 size) noexcept;

    /// @brief Upload the new vertex format settings to the GPU
    ///
    /// Takes whatever's in the current attribute array and sends it to OpenGL.
    bool apply_attributes() const noexcept;

    /// @brief ImGui menu to edit the mesh properties
    void edit_menu() noexcept;
};

namespace alr {
    struct mesh_instance;

    struct mesh {
        alr_model_desc chunks = {};
        std::vector<vertex_buffer> gl_vertbufs;
        std::vector<index_buffer> idxbufs;
        std::vector<mat4s> bind_pose;
        bool active = true; // Whether to render this mesh

        void render(texture_manager& tex_manager, alr::file& alr, render_context& ctx, const alr::mesh_instance& instance) const noexcept;
        void destroy() noexcept;
    };

    // An instance of a mesh in the world
    struct mesh_instance {
        // Optional, from .dat file
        vec3s* pos = nullptr;
        vec3s* rot = nullptr;

        float anim_frame = 0.0f;
        std::vector<mat4s> anim_pose; // Computed every frame
        std::vector<mat4s> skin_pose; // Computed every frame for skinned meshes
        const alr::mesh& mesh;

        bool raycast(const void* alr_data, ray_t ray) const noexcept;
        mat4s transform(u32 joint_idx) const noexcept;
        void update_animation(const alr::file& alr, u32 anim_id, float delta_time) noexcept;
        void update_skinning(const alr::file& alr, u32 anim_id, float delta_time) noexcept;
        void render(texture_manager& tex_manager, alr::file& alr, render_context& ctx) const noexcept;

        explicit mesh_instance(const alr::mesh& mesh, vec3s* pos = nullptr, vec3s* rot = nullptr);
    };
}

alr::mesh load_alr_mesh(const alr::file& alr, u32 idx);

/// Fill in vertex attributes on a mesh based on vertex format
/// @param out Mesh to receive vertex attribute info
/// @param vert_header ALR vertex buffer header w/ format information
void get_vert_attribute(vertex_buffer* out, vertbuf_entry vert_header);

