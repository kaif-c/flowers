#pragma once
#include "glm/ext/matrix_float4x4.hpp"
#include "glm/ext/vector_float3.hpp"
#include "glm/ext/vector_float2.hpp"
#include "glm/ext/vector_int3.hpp"
#include <GL/gl.h>
#include <GL/glext.h>
#include <memory>
#include <vector>

using namespace std;
using namespace glm;

class Texture {
public:
    /**
    * \brief Generate an empty texture
    * \param width 
    * \param height
    * \param unit the texture unit where image is stored
    */
    Texture(const GLsizei, const GLsizei, const int, const GLuint);
    /**
    * \brief Generate a texture from image
    * \param image pointer to GIMP generated image
    * \param unit the texture unit
    * \param levels Levels of MipMaps
    */
    Texture(const char *const, const int, const GLuint);
    /**
    * \brief Generate a texture from pixel data
    * \param width of texture
    * \param height of texture
    * \param pixels pixel data of the image
    * \param id pointer to texture id for OpenGL
    * \param unit the texture unit
    * \param levels Levels of MipMaps
    */
    void FromPixels(const GLuint, const GLuint, const unsigned char *,
            const GLuint, const GLuint);
    Texture(const Texture &);
    ~Texture();
    /**
    * \brief Binds the texture for render
    * \param unit the unit where the texture should bind
    */
    void Use(const int) const;
    void GenerateMipMap() const;

public:
    GLuint id;
private:
    GLuint width;
    GLuint height;
    GLuint levels;
};

class Program {
public:
    Program(vector<GLuint>, vector<GLuint>);
    Program(const Program &);
    ~Program();
    void Use() const;
    static void Dispatch(const vector<Texture*>, const ivec3);
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
    shared_ptr<Program> program;
    vec3 pos;
    GLuint id;
    bool still = false;
    bool billboard = false;
public:
    Mesh(const vector<Vertex> &,
            const vector<GLuint> &,
            const shared_ptr<Program>);
    Mesh(const Mesh &) = default;
    ~Mesh();
    void UpdateProjection() const;
    void Draw() const ;
    GLuint GetElemCnt() const;
private:
    GLuint elem_cnt;
};

class ParticleSystem {
public:
    ParticleSystem(unique_ptr<Mesh>, const GLuint);
    ~ParticleSystem();
    void Update(const float, const vec3 *, const float *, const vec3 *,
                const GLuint, const GLuint, const float);
    void Draw();
    void PrintParticles();
private:
    void BindSSBOBase(const GLuint) const;
    template<typename T> T *const MapSSBO(GLuint) const;
private:
    unique_ptr<Mesh> mesh;
    Program prog;
    GLuint ssbo[3];
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
    float scale;
    float _p3;
    float _p4;
};
