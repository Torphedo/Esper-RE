const char* diffuse_frag = R"(
#version 330 core
out vec4 fragment_rgba;

in vec2 texcoord;
in vec2 lightmap_uv;
in vec3 normal;
uniform sampler2D albedo_texture;
uniform sampler2D normal_texture;
uniform sampler2D lightmap_texture;
uniform vec3 cam_dir;
uniform int flags = 0;

void main() {
    bool render_uv_colors = (flags & 1) != 0;
    bool render_normal_colors = (flags & 2) != 0;
    bool has_normal = (flags & 4) != 0;

    vec4 color = vec4(texcoord, 0.0, 1.0);
    // I figure avoiding a texture sample is worth an if statement. - torph
    if (!render_uv_colors) {
        color = texture(albedo_texture, texcoord);
    }

    // TODO: Do alpha blending here. This is low-priority since most textures have BC1 1-bit alpha (except for a few normal maps).
    if (color.a < 0.1) {
        discard;
    }

    vec3 normal_vec = cam_dir;
    if (has_normal || render_normal_colors) {
        normal_vec = normal;
    }
    if (has_normal) {
        vec3 normal_sample = texture(normal_texture, texcoord).rgb;
        normal_sample = (normal_sample * 2.0) - 1.0;
        normal_vec += normal_sample;
    }
    const float ambient = 0.2f;
    float diffuse_factor = abs(dot(cam_dir, normal_vec)) + ambient;
    vec4 lightmap_color = vec4(texture(lightmap_texture, lightmap_uv).rgb, 1.0);
    lightmap_color = vec4(lightmap_color.rgb * lightmap_color.a, 0.0);

    fragment_rgba = (color * diffuse_factor) + lightmap_color;

    if (render_normal_colors) {
        fragment_rgba = vec4(normal, 1.0);
    }
}

)";