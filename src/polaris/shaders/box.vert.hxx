const char *box_vert = R"(
#version 330 core
uniform mat4 pvm;

struct Vertex {
    vec3 pos;
    vec3 normal;
    vec3 tangent;
    vec2 uv;
};

const Vertex verts[24] = Vertex[](
    Vertex(vec3(+1.0,+1.0,+1.0), vec3(0,0,1), vec3(0,1,0), vec2(1, 1)), // +Z
    Vertex(vec3(-1.0,+1.0,+1.0), vec3(0,0,1), vec3(0,1,0), vec2(0, 1)),
    Vertex(vec3(+1.0,-1.0,+1.0), vec3(0,0,1), vec3(0,1,0), vec2(1, 0)),
    Vertex(vec3(-1.0,-1.0,+1.0), vec3(0,0,1), vec3(0,1,0), vec2(0, 0)),
    Vertex(vec3(+1.0,+1.0,-1.0), vec3(0,0,-1), vec3(0,-1,0), vec2(1, 1)), // -Z
    Vertex(vec3(-1.0,+1.0,-1.0), vec3(0,0,-1), vec3(0,-1,0), vec2(0, 1)),
    Vertex(vec3(+1.0,-1.0,-1.0), vec3(0,0,-1), vec3(0,-1,0), vec2(1, 0)),
    Vertex(vec3(-1.0,-1.0,-1.0), vec3(0,0,-1), vec3(0,-1,0), vec2(0, 0)),
    Vertex(vec3(+1.0,+1.0,+1.0), vec3(1,0,0), vec3(0,0,1), vec2(1, 1)), // +X
    Vertex(vec3(+1.0,+1.0,-1.0), vec3(1,0,0), vec3(0,0,1), vec2(0, 1)),
    Vertex(vec3(+1.0,-1.0,+1.0), vec3(1,0,0), vec3(0,0,1), vec2(1, 0)),
    Vertex(vec3(+1.0,-1.0,-1.0), vec3(1,0,0), vec3(0,0,1), vec2(0, 0)),
    Vertex(vec3(-1.0,+1.0,+1.0), vec3(-1,0,0), vec3(0,0,-1), vec2(1, 1)), // -X
    Vertex(vec3(-1.0,+1.0,-1.0), vec3(-1,0,0), vec3(0,0,-1), vec2(0, 1)),
    Vertex(vec3(-1.0,-1.0,+1.0), vec3(-1,0,0), vec3(0,0,-1), vec2(1, 0)),
    Vertex(vec3(-1.0,-1.0,-1.0), vec3(-1,0,0), vec3(0,0,-1), vec2(0, 0)),
    Vertex(vec3(+1.0,+1.0,+1.0), vec3(0,1,0), vec3(1,0,0), vec2(1, 1)), // +Y
    Vertex(vec3(-1.0,+1.0,+1.0), vec3(0,1,0), vec3(1,0,0), vec2(0, 1)),
    Vertex(vec3(+1.0,+1.0,-1.0), vec3(0,1,0), vec3(1,0,0), vec2(1, 0)),
    Vertex(vec3(-1.0,+1.0,-1.0), vec3(0,1,0), vec3(1,0,0), vec2(0, 0)),
    Vertex(vec3(+1.0,-1.0,+1.0), vec3(0,-1,0), vec3(-1,0,0), vec2(1, 1)), // -Y
    Vertex(vec3(-1.0,-1.0,+1.0), vec3(0,-1,0), vec3(-1,0,0), vec2(0, 1)),
    Vertex(vec3(+1.0,-1.0,-1.0), vec3(0,-1,0), vec3(-1,0,0), vec2(1, 0)),
    Vertex(vec3(-1.0,-1.0,-1.0), vec3(0,-1,0), vec3(-1,0,0), vec2(0, 0))
);

const int indices[36] = int[](
    1,3,0,2,0,3,       // +Z
    4,6,5,7,5,6,       // -Z
    11,9,10,8,10,9,    // +X
    12,13,14,15,14,13, // -X
    19,17,18,16,18,17, // +Y
    20,21,22,23,22,21  // -Y
);

Vertex cubeVert(int i) {
    return verts[indices[i]];
}

out vec3 normal;
out mat3 TBN;
out vec2 uv;
void main() {
    Vertex v = cubeVert(gl_VertexID);
    gl_Position = pvm * vec4(v.pos, 1.0);

    normal = v.normal;
    uv = v.uv;
    vec3 bitangent = cross(v.normal, v.tangent);
    TBN = (mat3(v.tangent, bitangent, v.normal));
}
)";
