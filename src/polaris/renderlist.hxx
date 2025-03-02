#pragma once
#include <common/int.h>
#include <glad/glad.h>
#include <vector>

// Structures for rendering arbitrary format vertices
typedef enum : u8 {
    ATTRIBUTE_POSITION,
    ATTRIBUTE_TEXCOORD,
    ATTRIBUTE_ENUM_MAX,
}attribute_idx;

// Vertex attribute data for glVertexAttribPointer()
struct vertex_attribute {
    u16 type; // Data type like GL_FLOAT, GL_UNSIGNED_BYTE, etc.
    u16 stride;
    u16 offset;
    u8 components; // This can only be between 1 and 4
    // Whether this is an unused entry (the poor man's reverse std::optional).
    // Please don't manually overwrite, I put it last so you can leave it blank
    bool empty = false;
};

struct index_buffer {
    // TODO: Remove this remaining data pointer, since it's never used after construction
    const u8* data = nullptr;
    u32 num = 0; // Number of indices
    // OpenGL object to bind to GL_ELEMENT_ARRAY_BUFFER
    gl_obj obj = 0;
};

struct mesh_view {
    // Meshes tend to have 1 vertex buffer and many index buffers, so we store
    // index buffer info in a dynamic array.
    std::vector<index_buffer> idx_buffers;

    // There can be many vertex attributes, but we have to edit the shader to
    // handle every attribute as we discover them. We could make the shader
    // user-editable and make a whole complex dynamic uniform system, or just
    // keep it simple with a static array and update the shader when needed.
    vertex_attribute attributes[ATTRIBUTE_ENUM_MAX];
    gl_obj vbo = 0;
    gl_obj vao = 0;
    u16 draw_mode = GL_TRIANGLES;
    bool initialized = false;

    bool setup() {
        if (initialized) {
            return true; // Don't setup twice and leak OpenGL objects
        }

        // Set all the attributes empty, so that when the caller places things
        // in the array they automatically get marked non-empty. This lets us
        // use it instead of a dynamic array
        for (u32 i = 0; i < ARRAY_SIZE(attributes); i++) {
            attributes[i].empty = true;
        }

        glGenVertexArrays(1, &vao);
        if (vao == 0) {
            return false;
        }

        glGenBuffers(1, &vbo);
        if (vbo == 0) {
            return false;
        }

        initialized = true;
        return true;
    }

    void destroy() {
        glBindVertexArray(0);
        glBindBuffer(GL_ARRAY_BUFFER, 0);
        glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, 0);
        glDeleteVertexArrays(1, &vao);
        glDeleteBuffers(1, &vbo);

        for (index_buffer buf : idx_buffers) {
            glDeleteBuffers(1, &buf.obj);
        }
    }

    bool update_vertex_buf(const u8* buf, u32 size, u16 mode) {
        if (!initialized) {
            return false;
        }
        this->draw_mode = mode;

        // TODO: Use glBufferSubData() when the size hasn't increased
        glBindBuffer(GL_ARRAY_BUFFER, vbo);
        glBufferData(GL_ARRAY_BUFFER, size, buf, GL_DYNAMIC_DRAW); 
        glBindBuffer(GL_ARRAY_BUFFER, 0);

        return true;
    }

    bool add_index_buf(index_buffer buf) {
        if (!initialized) {
            return false;
        }

        glBindVertexArray(vao);
        gl_obj idx_buf_obj = 0;
        glGenBuffers(1, &idx_buf_obj);
        glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, idx_buf_obj);
        glBufferData(GL_ELEMENT_ARRAY_BUFFER, buf.num * sizeof(u16), buf.data, GL_DYNAMIC_DRAW);
        buf.obj = idx_buf_obj;
        this->idx_buffers.push_back(buf);

        glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, 0);
        glBindVertexArray(0);
        
        return true;
    }

    /// @brief Upload the new vertex format settings to the GPU
    bool apply_attributes() {
        if (!initialized) {
            return false;
        }

        glBindVertexArray(vao);
        glBindBuffer(GL_ARRAY_BUFFER, vbo);

        for (u32 i = 0; i < ARRAY_SIZE(attributes); i++) {
            const vertex_attribute attr = attributes[i];
            if (attr.empty) {
                continue;
            }

            // Update vertex format w/ OpenGL
            glEnableVertexAttribArray(i);
            glVertexAttribPointer(i, attr.components, attr.type, GL_FALSE, attr.stride, (void*)(u64)attr.offset);
        }
        glBindBuffer(GL_ARRAY_BUFFER, 0);
        glBindVertexArray(0);
        
        return true;
    }
};
