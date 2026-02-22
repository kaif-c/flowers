#version 450 core

out vec4 o_col;
flat in uint should_discard;
in vec2 uv;

void main() {
    if (should_discard != 0)
        discard;
    o_col = vec4(uv.x, uv.y, 1, 1);
}
