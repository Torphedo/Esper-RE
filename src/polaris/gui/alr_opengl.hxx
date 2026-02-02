#pragma once
#include <optional>
#include <vector>
#include <cglm/struct.h>

#include <common/vfile.h>
#include "render_context.hxx"

namespace alr {
    class file;
};

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

    index_buffer() = default;
    index_buffer(u32 idx_offset) noexcept : idx_chunk_offset(idx_offset) {
        return;
    }
};

// Static vertex buffer wrapper
struct vertex_buffer {
    vertex_attribute attributes[ATTRIBUTE_ENUM_MAX] = {};
    // The offset of the *entry*, not of the 0x16 chunk.
    u16 vertex_size = 0;
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

typedef struct {
    material_header* mat_chunk; // Materials
    chunk_armature*  skel_chunk;
    vertbuf_header*  vert_chunk;
    idxbuf_header*   idx_chunk;
}alr_model_desc;

namespace alr {
    struct mesh_instance;

    struct mesh {
        alr_model_desc chunks = {};
        std::vector<vertex_buffer> gl_vertbufs;
        std::vector<index_buffer> idxbufs;
        bool active = true; // Whether to render this mesh

        void render(file& alr, render_context& ctx, const alr::mesh_instance& instance) const noexcept;
        void destroy() noexcept;
    };

    // An instance of a mesh in the world
    struct mesh_instance {
        // Optional, from .dat file
        vec3s* pos = nullptr;
        vec3s* rot = nullptr;

        float anim_frame = 0.0f;
        std::vector<mat4s> anim_pose; // Computed every frame
        const alr::mesh& mesh;

        mat4s transform(u32 joint_idx) const noexcept;
        void update_animation(const alr::file& alr, u32 anim_id, float delta_time) noexcept;
        void render(alr::file& alr, render_context& ctx) const noexcept;

        mesh_instance(const alr::mesh& mesh, vec3s* pos = nullptr, vec3s* rot = nullptr);
    };
}

alr::mesh load_alr_mesh(const alr::file& alr, u32 idx);

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
void get_vert_attribute(vertex_buffer* out, vertbuf_entry vert_header);

