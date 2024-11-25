#version 330 core
out vec4 fragment_rgba;

in vec2 tex_coord;

uniform sampler2D perlin;

void main() {
    fragment_rgba = texture(perlin, tex_coord);
}

