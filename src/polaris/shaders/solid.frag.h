const char* solid_frag = R"(
#version 330 core
out vec4 fragment_rgba;

void main() {
    fragment_rgba = vec4(0, 1, 0, 1);
}
)";
