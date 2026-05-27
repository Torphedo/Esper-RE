const char* vkblink_frag = R"(
#version 330 core
out vec4 fragment_rgba;

in vec2 texcoord;
in vec2 lightmap_uv;
in vec3 normal;
uniform sampler2D albedo_texture;
uniform sampler2D normal_texture;

void main() {
    vec4 color = texture(albedo_texture, texcoord);
    color.a = length(color.rgb);

    fragment_rgba = color;
}

)";