#pragma once
#include <vector>
#include <glad/glad.h>
#include <cglm/struct.h>

#include <common/int.h>
#include <formats/alr.h>

/// @brief Dear ImGui menu to edit an attribute
///
/// This doesn't call ImGui::Begin()/End(), use it in an existing window.
void edit_menu(vertex_attribute& attr);

struct index_buffer {
    u32 idx_chunk_offset = 0;
    // OpenGL object to bind to GL_ELEMENT_ARRAY_BUFFER
    gl_obj obj = 0;

    // If true, we're using a transform calculated from an 0x3 chunk
    // (and updating it won't affect the file's data)
    // If false, we're referencing a position/rotation from a .dat file in memory.
    bool is_precalc_transform = false;
    mat4s transform = glms_mat4_identity();
    vec3s* position = nullptr;
    vec3s* rotation = nullptr;

    bool enabled = true; // Whether to render this index buffer

    mat4s get_transform() const noexcept;
    index_buffer() = default;
    index_buffer(u32 offset, mat4s transform) noexcept
        : idx_chunk_offset(offset), transform(transform) {
        is_precalc_transform = true;
    }
    index_buffer(u32 offset, vec3s* position, vec3s* rotation) noexcept
        : idx_chunk_offset(offset), position(position), rotation(rotation) {
        return;
    }
};

// We need a forward declaration, since a class in this file is a member of polaris.
namespace al {
    class resource;
}

struct mesh_view {
    std::vector<index_buffer> idx_buffers;

    vertex_attribute attributes[ATTRIBUTE_ENUM_MAX] = {0};
    u16 vertex_size = 0;
    gl_obj vbo = 0;
    gl_obj vao = 0;

    // Texture coordinates are divided by this value in the shader before use
    u32 uv_divisor = 1;

    // Whether to render this mesh
    bool active = true;

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
    bool update_vertex_buf(const u8* buf, u32 size) const noexcept;

    /// @brief Upload an index buffer to the GPU for this mesh
    ///
    /// @param alr_data Get from al::resource.data
    /// @param alr_size Get from al::resource.alr_size
    /// @param buf Index buffer structure to upload
    bool add_index_buf(const u8* alr_data, u32 alr_size, index_buffer buf) noexcept;

    /// @brief Upload the new vertex format settings to the GPU
    ///
    /// Takes whatever's in the current attribute array and sends it to OpenGL.
    bool apply_attributes() const noexcept;

    /// @brief ImGui menu to edit the mesh properties
    void edit_menu(al::resource& alr) noexcept;
};
