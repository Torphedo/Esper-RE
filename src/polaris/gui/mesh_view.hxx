#pragma once
#include <optional>
#include <vector>
#include <glad/glad.h>
#include <cglm/struct.h>

#include <common/int.h>
#include <common/vfile.h>
#include <formats/alr.h>

namespace alr {
    class file;
};

/// @brief Dear ImGui menu to edit an attribute
///
/// This doesn't call ImGui::Begin()/End(), use it in an existing window.
void edit_menu(vertex_attribute& attr);

struct index_buffer {
    u32 idx_chunk_offset = 0;
    u32 armature_chunk_offset = 0;

    // OpenGL object to bind to GL_ELEMENT_ARRAY_BUFFER
    gl_obj obj = 0;

    bool enabled = true; // Whether to render this index buffer

    // If true, we're using a transform calculated from an 0x3 chunk
    // (and updating it won't affect the file's data)
    // If false, we're referencing a position/rotation from a .dat file in memory.
    bool is_skele_transform = false;
    mat4s transform = glms_mat4_identity();
    vec3s* position = nullptr;
    vec3s* rotation = nullptr;

    mat4s get_transform(const alr::file& alr) const noexcept;
    index_buffer() = default;
    index_buffer(u32 idx_offset, u32 skele_offset) noexcept
        : idx_chunk_offset(idx_offset), armature_chunk_offset(skele_offset) {
        is_skele_transform = true;
    }
};

struct mesh_view {
    std::vector<index_buffer> idx_buffers;

    const u8* vertices = nullptr;
    vertex_attribute attributes[ATTRIBUTE_ENUM_MAX] = {};
    // The offset of the *entry*, not of the 0x16 chunk.
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
    bool update_vertex_buf(const u8* buf, u32 size) noexcept;

    /// @brief Upload an index buffer to the GPU for this mesh
    ///
    /// @param alr_data Get from alr.data
    /// @param alr_size Get from alr.alr_size
    /// @param buf Index buffer structure to upload
    bool add_index_buf(const u8* alr_data, u32 alr_size, index_buffer buf) noexcept;

    /// @brief Upload the new vertex format settings to the GPU
    ///
    /// Takes whatever's in the current attribute array and sends it to OpenGL.
    bool apply_attributes() const noexcept;

    /// @brief ImGui menu to edit the mesh properties
    void edit_menu(alr::file& alr) noexcept;
};

// Standardized vertex format that can express all known Phantom Dust vertex
// formats. Will change often as new information is found.
struct std_vertex {
    // 3D position of the vertex. Should always be present.
    std::optional<vec3s> pos;

    // 2D texture coordinates. Should often be present.
    std::optional<vec2s> texcoord;

    std::optional<vec3s> normal;
};

/// @brief Convert a single vertex to the standard format.
///
/// @param vertbuf Buffer containing PD vertex data (must be at least [vert_size] bytes)
/// @param vert_size The format ID found in the vertex buffer entry
/// @return A vertex in standard format
std_vertex standardize_pd_vertex(void* vertbuf, u8 format_id);

/// Read data from a vertex based on the format in the vertex attribute
/// @param vf Buffer view to use
/// @param attr Vertex attribute specifying the format
/// @return Up to 4 components read from the vertex
vec4s read_attr(vfile& vf, vertex_attribute attr);

/// Fill in vertex attributes on a mesh based on vertex format
/// @param out Mesh to receive vertex attribute info
/// @param vert_header ALR vertex buffer header w/ format information
void get_vert_attribute(mesh_view* out, vertbuf_entry vert_header);
