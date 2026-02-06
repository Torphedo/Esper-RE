const char* generic_vert = R"(
#version 330 core
layout (location = 0) in vec3 a_pos;
layout (location = 1) in vec2 a_texcoord;
layout (location = 2) in vec2 a_lightmap_uv;
layout (location = 3) in vec3 a_normal;

uniform mat4 pvm;
uniform uint uv_divisor;
out vec2 texcoord;
out vec2 lightmap_uv;
out vec3 normal;

void main() {
    gl_Position = pvm * vec4(a_pos, 1.0);

    // We map the large integer value into the [0, 1] range for texture lookups
    texcoord = a_texcoord / uv_divisor;
    lightmap_uv = a_lightmap_uv / uv_divisor;
    normal = a_normal / uv_divisor;
}
)";
