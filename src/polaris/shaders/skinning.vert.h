const char* skinning_vert = R"(
#version 330 core
layout (location = 0) in vec3 a_pos;
layout (location = 1) in vec2 a_texcoord;
layout (location = 2) in vec2 a_lightmap_uv;
layout (location = 3) in vec3 a_normal;
layout (location = 4) in vec2 a_blendindices;
layout (location = 5) in vec2 a_blendweights;

uniform mat4 skin_xforms[256];
uniform mat4 pvm;
uniform uint uv_divisor;
out vec2 texcoord;
out vec2 lightmap_uv;
out vec3 normal;

void main() {
    // We map the large integer value into the [0, 1] range for texture lookups
    texcoord = a_texcoord / uv_divisor;
    vec2 weights = a_blendweights / uv_divisor;
    lightmap_uv = a_lightmap_uv / uv_divisor;
    normal = a_normal / uv_divisor;

    mat4 skin1 = skin_xforms[int(a_blendindices.x)] * weights.x;
    mat4 skin2 = skin_xforms[int(a_blendindices.y)] * weights.y;
    mat4 skin = skin2 * skin1;

    gl_Position = pvm * skin * vec4(a_pos, 1.0);
}
)";
