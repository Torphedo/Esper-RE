#version 330 core
layout (location = 0) in vec3 a_pos;
layout (location = 1) in vec2 a_tex_coord;

out vec2 tex_coord;

uniform mat4 model;
uniform mat4 view;
uniform mat4 projection;
uniform float img_aspect;

void main() {
    gl_Position = projection * view * model * vec4(a_pos, 1.0);

    // Adjust with the aspect ratio of the current image to prevent stretching
    gl_Position.x *= img_aspect;

    tex_coord = a_tex_coord;
}

