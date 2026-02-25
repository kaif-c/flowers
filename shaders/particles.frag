#version 450 core

flat in uint should_discard;
in vec2 uv;
layout(binding = 0) uniform sampler2D tex;

out vec4 o_col;

void main() {
    if (should_discard != 0)
        discard;
    o_col = texture(tex, uv);
    if (o_col.a < 1)
        discard;
}
