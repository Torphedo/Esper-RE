#pragma once
#include <common/int.h>
#include <glad/glad.h>
#include <vector>

// Structures for rendering arbitrary format vertices
typedef enum : u8 {
    ATTRIBUTE_POSITION,       
    ATTRIBUTE_TEXCOORD,       
}attribute_idx;

// Vertex attribute data for glVertexAttribPointer()
typedef struct {
    u8 components: 4; // This can only be between 1 and 4
    // Whether to consider this a valid entry (the poor man's std::optional)
    bool exists: 1;
    u16 type; // Data type like GL_FLOAT, GL_UNSIGNED_BYTE, etc.
    u16 stride;
    u16 offset;
}vertex_attribute;

// TODO: This isn't tightly packed, maybe put 2 entries per struct to fix that?
struct index_buffer {
    u8* data = nullptr;
    u32 num = 0; // Number of indices
    // Integer type of index buffer (GL_UNSIGNED_BYTE, GL_UNSIGNED_SHORT, or GL_UNSIGNED_INT)
    u16 indices_type = GL_UNSIGNED_SHORT;
    gl_obj obj;
};

struct mesh_view {
    // Meshes tend to have 1 vertex buffer and many index buffers, so we store
    // index buffer info in a dynamic array.
    std::vector<index_buffer> idx_buffers;
    gl_obj vbo = 0;
    u16 draw_mode = GL_TRIANGLES;
    gl_obj vao = 0;
    bool initialized = false;

    bool setup() {
        if (initialized) {
            return true;
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

    bool update_vertex_buf(u8* buf, u32 size, u16 mode) {
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

        u8 index_size = 0;
        switch (buf.indices_type) {
        case GL_UNSIGNED_BYTE:
            index_size = 1;
            break;
        case GL_UNSIGNED_SHORT:
            index_size = 2;
            break;
        case GL_UNSIGNED_INT:
            index_size = 4;
            break;
        }

        glBindVertexArray(vao);
        gl_obj idx_buf_obj = 0;
        glGenBuffers(1, &idx_buf_obj);
        glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, idx_buf_obj);
        glBufferData(GL_ELEMENT_ARRAY_BUFFER, buf.num * index_size, buf.data, GL_DYNAMIC_DRAW);
        buf.obj = idx_buf_obj;
        this->idx_buffers.push_back(buf);

        glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, 0);
        glBindVertexArray(0);
        
        return true;
    }

    bool set_attribute(vertex_attribute attr, attribute_idx idx) {
        if (!initialized) {
            return false;
        }

        glBindVertexArray(vao);
        glBindBuffer(GL_ARRAY_BUFFER, vbo);
        glEnableVertexAttribArray(idx);

        glVertexAttribPointer(idx, attr.components, attr.type, GL_FALSE, attr.stride, (void*)(u64)attr.offset);

        glBindBuffer(GL_ARRAY_BUFFER, 0);
        glBindVertexArray(0);
        
        return true;
    }
};
