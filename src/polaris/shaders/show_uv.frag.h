const char* show_uv_frag = R"(
#version 330 core
out vec4 fragment_rgba;

in vec2 texcoord;
in vec2 lightmap_uv;
in vec3 normal;
uniform sampler2D albedo_texture;
uniform sampler2D normal_texture;
uniform sampler2D lightmap_texture;

void main() {
    fragment_rgba = vec4(texcoord, 0.0, 1.0);
}
)";
