#pragma once
#include "glm/ext/matrix_float4x4.hpp"
#include "glm/ext/vector_float3.hpp"
#include "glm/ext/vector_float2.hpp"
#include "glm/ext/vector_int3.hpp"
#include <GL/gl.h>
#include <GL/glext.h>
#include <vector>

using namespace std;
using namespace glm;

class Texture {
public:
    Texture(const GLsizei, const GLsizei, const int);
    Texture(const Texture &);
    void Destroy();
    void Use(const int) const;
    void GenerateMipMap();

public:
    GLuint id;
};

class Program {
public:
    Program(vector<GLuint>, vector<GLuint>);
    Program(const Program &);
    void Use() const;
    static void Dispatch(const vector<Texture>, const ivec3);
    static void Dispatch(const ivec3);
    static void FinishComputes(const GLuint =
                               GL_SHADER_IMAGE_ACCESS_BARRIER_BIT|
                               GL_SHADER_STORAGE_BARRIER_BIT|
                               GL_VERTEX_ATTRIB_ARRAY_BARRIER_BIT);
    GLuint GetUniformLoc(const char *const) const;
    void Uniform(const char *, GLuint) const;
    void Uniform(const char *, GLint) const;
    void Uniform(const char *, float) const;
    void Uniform(const char *, const float *, GLint, GLint) const;
    void Uniform(const char *, const mat4) const;
    void Destroy();
private:
    GLuint program;
};

class Vertex {
public:
    vec3 pos;
    vec2 uv;
};

class Mesh {
public:
    Program program;
    vec3 pos;
    GLuint id;
    bool still = false;
    bool billboard = false;
public:
    Mesh(const vector<Vertex> &, const vector<GLuint> &, const Program &);
    Mesh(const Mesh &) = default;
    void UpdateProjection() const;
    void Draw() const ;
    void Destroy();
    GLuint GetElemCnt() const;
private:
    GLuint elem_cnt;
};

class ParticleSystem {
public:
    ParticleSystem(const Mesh &, const GLuint);
    ParticleSystem(const ParticleSystem&) = default;
    void Update(const float, const vec3 *, const float *, const vec3 *,
                const GLuint, const GLuint, const float);
    void Draw();
    void Destroy();
    void PrintParticles();
private:
    Mesh mesh;
    Program prog;
    GLuint particle_buf;
    GLuint draw_cmd_buf;
    GLuint dead_inds_buf;
    GLuint max;
};

// INFO: _pN are padding as according to glsl std430
struct Particle {
public:
    vec3 pos;
    float _p1;
    vec3 vel;
    float mass;
    float life;
    float _p2;
    float _p3;
    float _p4;
};
