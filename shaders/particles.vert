#version 450 core

struct Particle {
    vec3  pos;
    vec3  vel;
    float mass;
    float life;
    float scale;
};

layout (std430, binding = 0) buffer ParticlesBuf {
    Particle particles[];
};

layout (location = 0) in vec3 aPos;
layout (location = 1) in vec2 aUV;
uniform mat4 u_transform;
uniform mat4 u_proj;

flat out uint should_discard;
out vec2 uv;

void main() {
    Particle p = particles[gl_InstanceID];
    if (p.life <= 0) {
        should_discard = 1;
        return;
    }
    should_discard = 0;
    gl_Position = u_proj * ((u_transform * vec4(p.pos, 1)) + p.scale*vec4(aPos, 0));
    uv = aUV;
}
