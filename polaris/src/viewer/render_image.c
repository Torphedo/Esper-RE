#include <glad/glad.h>
#include <GLFW/glfw3.h>

#include <common/gl/shader.h>
#include <common/gl/model.h>
#include <common/logging.h>
#include "camera.h"
#include "render_image.h"

#define QUAD_SIZE (3.0f)
tex_vertex quad_vertices[] = {
    { .position = {QUAD_SIZE, QUAD_SIZE, 0.0f},
      .texcoord = {1.0f, 1.0f}
    },
    {
      .position = {QUAD_SIZE, -QUAD_SIZE, 0.0f},
      .texcoord = {1.0f, 0.0f}
    },
    {
      .position = {-QUAD_SIZE, -QUAD_SIZE, 0.0f},
      .texcoord = {0.0f, 0.0f}
    },
    {
      .position = {-QUAD_SIZE,  QUAD_SIZE, 0.0f},
      .texcoord = {0.0f, 1.0f}
    },
    { .position = {QUAD_SIZE, QUAD_SIZE, 0.0f},
      .texcoord = {1.0f, 1.0f}
    },
    {
      .position = {-QUAD_SIZE, -QUAD_SIZE, 0.0f},
      .texcoord = {0.0f, 0.0f}
    }
};

static const char frag[] = {
    #include "shader/tex_viewer.frag.h"
};

static const char vert[] = {
    #include "shader/tex_viewer.vert.h"
};

img_state image_init(texture img) {
    img_state state = {.img = img};

    // Setup VAO to store our state
    glGenVertexArrays(1, &state.vertex_array);
    glBindVertexArray(state.vertex_array);
    
    // Setup vertex buffer
    glGenBuffers(1, &state.vertex_buffer);
    glBindBuffer(GL_ARRAY_BUFFER, state.vertex_buffer);
    glBufferData(GL_ARRAY_BUFFER, sizeof(quad_vertices), &quad_vertices, GL_STATIC_DRAW);
    
    // Create vertex layout
    glVertexAttribPointer(0, sizeof(vec3s) / sizeof(float), GL_FLOAT, GL_FALSE, sizeof(tex_vertex), (void*)offsetof(tex_vertex, position));
    glEnableVertexAttribArray(0);
    glVertexAttribPointer(1, sizeof(vec2s) / sizeof(float), GL_FLOAT, GL_FALSE, sizeof(tex_vertex), (void*)offsetof(tex_vertex, texcoord));
    glEnableVertexAttribArray(1);
    
    // Load and compile shaders
    state.shader_program = program_compile_src(vert, frag);

    // Make sure linking succeeded
    if (!shader_link_check(state.shader_program)) {
        // No need to print, link check prints messages on failure.
        return (img_state){0};
    }

    // Get uniform locations
    glUseProgram(state.shader_program);
    state.u_img_aspect = glGetUniformLocation(state.shader_program, "img_aspect");
    state.u_pvm = glGetUniformLocation(state.shader_program, "pvm");


    // Load texture
    glGenTextures(1, &state.gl_img);
    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D, state.gl_img);

    glCompressedTexImage2D(GL_TEXTURE_2D, 0, GL_COMPRESSED_RGBA_S3TC_DXT1_EXT, img.width, img.height, 0, (img.width * img.height) / 2, img.data);
    glGenerateMipmap(GL_TEXTURE_2D);

    // Wrapping & filtering settings
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_BORDER);	
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_BORDER);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST_MIPMAP_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);

    // Unbind our buffers & reset state
    glBindVertexArray(0);
    glBindBuffer(GL_ARRAY_BUFFER, 0);
    glBindTexture(GL_TEXTURE_2D, 0);

    return state;
}

void image_render(img_state* state, void* window) {
    // We have to make the shader active to upload uniforms
    glUseProgram(state->shader_program);
    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D, state->gl_img);

    // Upload current texture aspect ratio
    const float ratio = (float)state->img.width / (float)state->img.height;
    glUniform1f(state->u_img_aspect, ratio);

    // Upload transform matrix
    mat4 transform = {0};
    camera_proj_view(transform);
    glm_rotate(transform, glm_rad(180.0f), (vec3){1.0f, 0.0f, 0.0f});
    glUniformMatrix4fv(state->u_pvm, 1, GL_FALSE, (const float*)transform);

    // Draw
    glBindVertexArray(state->vertex_array);
    glDrawArrays(GL_TRIANGLES, 0, sizeof(quad_vertices) / sizeof(*quad_vertices));

    glBindVertexArray(0);
    glBindBuffer(GL_ARRAY_BUFFER, 0);
    glBindTexture(GL_TEXTURE_2D, 0);
}

void image_destroy(img_state state) {
    glDeleteProgram(state.shader_program);
    glDeleteVertexArrays(1, &state.vertex_array);
    glDeleteBuffers(1, &state.vertex_buffer);
    glDeleteTextures(1, &state.gl_img);
    state.img = (texture){0};
}
