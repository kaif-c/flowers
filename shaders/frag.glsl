#version 450 core

out vec4 o_col;
in vec2 uv;

void main() {
    o_col = vec4(uv.x, uv.y, 1, 1);
}
