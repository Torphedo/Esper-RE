const char *box_vert = R"(
#version 330 core
uniform mat4 pvm;

struct Vertex {
    vec3 pos;
};

// Cube vertices
const vec2 unit = vec2(1, -1);
const Vertex verts[8] = Vertex[](
    // Points with positive Z
    Vertex(unit.xxx), Vertex(unit.yxx), Vertex(unit.xyx), Vertex(unit.yyx),

    // Points with negative Z
    Vertex(unit.yyy), Vertex(unit.xyy), Vertex(unit.yxy), Vertex(unit.xxy)
);

const int indices[24] = int[](
    0,1, 0,2, 3,1, 3,2, // +Z lines
    4,5, 4,6, 7,5, 7,6, // -Z lines
    0,7, 1,6, 2,5, 3,4  // Lines connecting the 2 halves
);

Vertex cubeVert(int i) {
    return verts[indices[i]];
}

void main() {
    Vertex v = cubeVert(gl_VertexID);
    gl_Position = pvm * vec4(v.pos, 1.0);
}
)";
