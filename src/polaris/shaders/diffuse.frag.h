const char* diffuse_frag = R"(
#version 330 core
out vec4 fragment_rgba;

in vec2 texcoord;
in vec2 lightmap_uv;
in vec3 normal;
uniform sampler2D albedo_texture;
uniform sampler2D normal_texture;
uniform sampler2D lightmap_texture;

void main() {
    vec4 color = texture(albedo_texture, texcoord);

    // TODO: Do alpha blending here. This is low-priority since most textures have BC1 1-bit alpha (except for a few normal maps).
    if (color.a < 0.1) {
        discard;
    }

    vec4 lightmap_color = vec4(texture(lightmap_texture, lightmap_uv).rgb, 1.0);
    lightmap_color = vec4(lightmap_color.rgb * lightmap_color.a, 0.0);

    fragment_rgba = color + lightmap_color * 0.5;
}

)";