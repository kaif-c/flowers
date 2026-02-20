#version 450 core

struct Particle {
    vec3  pos;
    vec3  vel;
    float mass;
    float life;
};

layout (std430, binding = 0) buffer ParticlesBuf {
    Particle particles[];
};

layout (location = 0) in vec3 aPos;
layout (location = 1) in vec2 aUV;
uniform mat4 u_transform;
uniform mat4 u_proj;

out vec2 uv;

void main() {
    Particle p = particles[gl_InstanceID];
    gl_Position = u_proj * ((u_transform * vec4(p.pos, 1)) +
        vec4(aPos, 1));
    uv = aPos.xy;
}
