#version 450 core

out vec4 o_col;
in vec2 uv;

layout(binding = 0) uniform sampler2D tex;

void main() {
    o_col = texture(tex, uv);
    if (o_col.a < 1)
        discard;
}
