#pragma once
#include <common/image.h>

typedef struct {
    gl_obj vertex_array;
    gl_obj vertex_buffer;
    gl_obj shader_program;
    gl_obj gl_img;
    texture img;
    // If the image buffer you give is inside a larger buffer, free() can't be
    // called on it. This flag lets the destruction function know if it should
    // automatically free the data.
    bool free_on_destroy;

    // Shader uniforms
    gl_obj u_img_aspect;
    gl_obj u_pvm;
}img_state;

img_state image_init(texture img, bool free_on_destroy);
void image_render(img_state* state, void* window);
void image_destroy(img_state state);
