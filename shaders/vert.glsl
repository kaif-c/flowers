#version 450 core

layout (location = 0) in vec3 a_pos;
layout (location = 1) in vec2 a_uv;
layout (location = 0) uniform mat4 u_transform;
out vec2 uv;

void main() {
    gl_Position = u_transform * vec4(a_pos.xyz, 1);
    uv = a_uv;
}
