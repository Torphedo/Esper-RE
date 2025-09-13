#pragma once
#include <common/int.h>
#include <glad/glad.h>
#include <vector>
// Structures for rendering arbitrary format vertices

// All supported vertex attribute slots
typedef enum : u8 {
    ATTRIBUTE_POSITION,
    ATTRIBUTE_TEXCOORD,
    ATTRIBUTE_NORMAL,
    ATTRIBUTE_ENUM_MAX,
}attribute_idx;

static const char* attribute_names[] = {
    "Position",
    "Texture Coordinates",
    "Normal",
    "[Invalid]",
};

// Vertex attribute data for glVertexAttribPointer()
struct vertex_attribute {
    u16 type = GL_FLOAT; // Data type like GL_FLOAT, GL_UNSIGNED_BYTE, etc.
    u16 offset = 0;
    u8 components = 1; // This can only be between 1 and 4
    // Whether this is an unused entry (the poor man's reverse std::optional).
    // Please don't manually overwrite, I put it last so you can leave it blank
    bool empty = false;

    /// @brief Dear ImGui menu to edit the attribute (for an existing window)
    void edit_menu();
};

struct index_buffer {
    // TODO: Remove this remaining data pointer, since it's never used after construction
    const u8* data = nullptr;
    u32 num = 0; // Number of indices
    u32 vertex_group = 0;
    // OpenGL object to bind to GL_ELEMENT_ARRAY_BUFFER
    gl_obj obj = 0;
    u16 albedo_tex_idx = 0;
    u16 normal_tex_idx = 0;
    u16 draw_mode = GL_TRIANGLES;
    bool enabled = true; // Whether to render this index buffer
};

// We need a forward declaration instead of a header include, since a class
// in this file is a member of polaris.
class polaris;

struct mesh_view {
    // Meshes tend to have 1 vertex buffer and many index buffers, so we store
    // index buffer info in a dynamic array.
    std::vector<index_buffer> idx_buffers;

    // There can be many vertex attributes, but we have to edit the shader to
    // handle every attribute as we discover them. We could make the shader
    // user-editable and make a whole complex dynamic uniform system, or just
    // keep it simple with a static array and update the shader when needed.
    vertex_attribute attributes[ATTRIBUTE_ENUM_MAX] = {0};
    u16 vertex_size = 0;
    gl_obj vbo = 0;
    gl_obj vao = 0;

    // Texture coordinates are divided by this value in the shader before use.
    u32 uv_divisor = 1;

    // Divide by the maximum value of the current integer type instead of using
    // the divisor value. This maps an integer value into the range [0, 1]
    // based on its type, and is usually what you want.
    bool use_type_divisor = true;

    bool initialized = false;

    // Whether to render this mesh
    bool active = true;

    // Constructor, made manual to avoid accidental resource deletion when copying
    // Check the [initialized] field to see if it failed
    bool setup();
    // Destructor, made manual to avoid accidental resource deletion when copying
    void destroy();

    /// @brief Upload/update the GPU-side vertex buffer.
    ///
    /// @param buf The vertex buffer to upload
    /// @param size The size of the vertex buffer
    /// @param mode The triangle drawing mode (e.g. GL_TRIANGLES)
    bool update_vertex_buf(const u8* buf, u32 size);

    /// @brief Upload an index buffer to the GPU for this mesh
    ///
    /// @param buf Index buffer structure to upload
    bool add_index_buf(index_buffer buf);

    /// @brief Upload the new vertex format settings to the GPU
    ///
    /// Takes whatever's in the current attribute array and sends it to OpenGL.
    bool apply_attributes();

    /// @brief ImGui menu to edit the mesh properties
    void edit_menu(const polaris* pol);
};
