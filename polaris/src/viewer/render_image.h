#include <common/image.h>

typedef struct {
    gl_obj vertex_array;
    gl_obj vertex_buffer;
    gl_obj shader_program;
    gl_obj gl_img;
    texture img;

    gl_obj u_img_aspect;
    gl_obj u_proj;
    gl_obj u_model;
    gl_obj u_view;
}img_state;

img_state image_init(texture img);
void image_render(img_state* state, void* window);
void image_destroy(img_state state);
