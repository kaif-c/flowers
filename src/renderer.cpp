#include "renderer.hpp"
#include "application.hpp"
#include "gl_func.hpp"
#include <GL/gl.h>
#include <GL/glext.h>
#include <cstddef>
#include <cstdio>
#include <cstdlib>
#include <fstream>
#include <memory>
#include <print>
#include <vector>
#include "glm/ext/matrix_float4x4.hpp"
#include "glm/ext/matrix_transform.hpp"
#include "glm/ext/vector_float3.hpp"
#include "glm/gtc/type_ptr.hpp"
#include "logger.hpp"

enum {
    SSBO_PARTICLE,
    SSBO_DRAWCMD,
    SSBO_DEADINDS,
    SSBO_NUM
};

using namespace glm;
using namespace std;

struct DrawCmd {
    GLuint  count;
    GLuint  instanceCount;
    GLuint  firstIndex;
    GLint   baseVertex;
    GLuint  baseInstance;
};

string shaders_src[] = {
    {
        #embed "../shaders/vert.glsl" // 0
    },
    {
        #embed "../shaders/frag.glsl" // 1
    },
    {
        #embed "../shaders/tex_generator.glsl" // 2
    },
    {
        #embed "../shaders/tex.frag" // 3
    },
    {
        #embed "../shaders/particle.comp" // 4
    },
    {
        #embed "../shaders/particles.vert" // 5
    },
    {
        #embed "../shaders/sold.frag" // 6
    },
    {
        #embed "../shaders/particles.frag" // 7
    },
};

static bool shader_compile_check(GLuint shade, GLuint type)
{
    GLint success;
    glGetShaderiv(shade, GL_COMPILE_STATUS, &success);
    if (success != GL_TRUE)
    {
        GLsizei log_length = 0;
        GLchar message[1024];
        glGetShaderInfoLog(shade, 1024,
                           &log_length, message);
        ERR("Cannot compile {} shader\n{}", 
            type, message);
        return true;
    }

    return false;
}

static bool link_check(GLuint prog)
{
    GLint success;
    glGetProgramiv(prog, GL_LINK_STATUS, &success);
    if (success != GL_TRUE)
    {
        GLsizei log_length = 0;
        GLchar message[1024];
        glGetProgramInfoLog(prog, 1024,
                           &log_length, message);
        ERR("Cannot link shader program\n{}", 
            message);
        return true;
    }

    return false;
}

void Texture::FromPixels(const GLuint width,
                const GLuint height,
                const unsigned char *pixels,
                const GLuint unit,
                const GLuint levels) {
    this->levels = levels;
    this->width = width;
    this->height = height;

    glGenTextures(1, &id);
    Use(unit);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER,
                        (levels > 0)?
                            GL_LINEAR_MIPMAP_LINEAR : GL_LINEAR
                    );
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA8,
                 width, height, 0, GL_RGBA,
                 GL_UNSIGNED_BYTE, pixels);
    if (levels > 0)
        glTexStorage2D(GL_TEXTURE_2D, levels, GL_RGBA8, width, height);
}

Texture::Texture(const GLsizei width, const GLsizei height,
                 const int unit, const GLuint levels) {
    FromPixels(width, height, nullptr, unit, levels);
}

Texture::Texture(const char *const img, const int unit,
                 const GLuint levels) {
    GLuint width = *(unsigned int*)img;
    GLuint height = *((unsigned int*)img + 1);
    unsigned char *pixels = (unsigned char*)
        (img + sizeof(unsigned int)*3 + sizeof(char*));
    FromPixels(width, height, pixels, unit, levels);
}

Texture::Texture(const Texture &old) {
    id = old.id;
}

Texture::~Texture() {
    glDeleteTextures(1, &id);
}

void Texture::Use(const int unit) const {
    glActiveTexture(GL_TEXTURE0 + unit);
    glBindTexture(GL_TEXTURE_2D, id);
}

void Texture::GenerateMipMap() const {
    if (levels > 0) {
        Use(0);
        glGenerateMipmap(GL_TEXTURE_2D);
    }
    else {
        ERR("Cannot generate mipmap with level={}", levels);
        exit(1);
    }
}

Program::Program(vector<GLuint> ids, vector<GLuint> types) {
    if (ids.size() != types.size()) {
        ERR("Number of ids({}) and types({}) mismatch",
            ids.size(), types.size());
        exit(1);
    }

    vector<GLuint> shaders;
    shaders.reserve(ids.size());
    for (unsigned int i = 0; i < ids.size(); ++i) {
        shaders.push_back(glCreateShader(types[i]));
        const char *src = shaders_src[ids[i]].c_str();

        glShaderSource(shaders[i], 1, &src, NULL);
        glCompileShader(shaders[i]);
        if (shader_compile_check(shaders[i], types[i])) {
            fflush(stdout);
            exit(1);
        }
    }

    program = glCreateProgram();
    for (GLuint shader: shaders)
        glAttachShader(program, shader);
    glLinkProgram(program);
    if (link_check(program)) {
        fflush(stdout);
        exit(1);
    }

    glUseProgram(0);
    for (GLuint shader: shaders)
        glDeleteShader(shader);
}

Program::Program(const Program &old) {
    program = old.program;
}

Program::~Program() {
    glDeleteProgram(program);
}

void Program::Use() const {
    glUseProgram(program);
}

void Program::Dispatch(const vector<Texture*> textures,
                       const ivec3 wg) {
    for (unsigned int i = 0; i < textures.size(); ++i) {
        glBindImageTexture(i, textures[i]->id, 0, GL_FALSE, 0,
                           GL_READ_WRITE, GL_RGBA8);
    }
    Dispatch(wg);
}

void Program::Dispatch(const ivec3 wg) {
    glDispatchCompute(wg.x, wg.y, wg.z);
}

void Program::FinishComputes(const GLuint type) {
    glMemoryBarrier(type);
}

GLuint Program::GetUniformLoc(const char *name) const {
    int i = glGetUniformLocation(program, name);
    if (i < 0)
        ERR("Uniform with name '{}' doesnt exist", name);
    return i;
}

void Program::Uniform(const char *name, GLuint data) const {
    glUniform1ui(GetUniformLoc(name), data);
}

void Program::Uniform(const char *name, GLint data) const {
    glUniform1i(GetUniformLoc(name), data);
}

void Program::Uniform(const char *name, float data) const {
    glUniform1f(GetUniformLoc(name), data);
}

void Program::Uniform(const char *name, const float *data, GLint size,
                       GLint component_num) const {
    switch (component_num) {
        case 1:
            glUniform1fv(GetUniformLoc(name), size, data);
            break;
        case 3:
            glUniform3fv(GetUniformLoc(name), size, data);
            break;
        default:
            ERR("Component number {} not possible for unifroms",
                component_num);
            break;
    }
}


void Program::Uniform(const char *name, const mat4 data) const {
    glUniformMatrix4fv(GetUniformLoc(name),
                       1,
                       GL_FALSE,
                       value_ptr(data));
}

static void enable_attrib(GLuint ind, GLint comps, GLuint64 offset) {
    glEnableVertexAttribArray(ind);
    glVertexAttribPointer(ind, comps, GL_FLOAT, GL_FALSE, sizeof(Vertex), (void*)offset);
}

Mesh::Mesh(const vector<Vertex> &verts,
           const vector<GLuint> &elems,
           const shared_ptr<Program> prog)
    : program(prog), pos(0, 0, 0) {
    GLuint bo[2];
    glGenBuffers(2, bo);
    glBindBuffer(GL_ARRAY_BUFFER, bo[0]);
    glBufferData(GL_ARRAY_BUFFER, verts.size()*sizeof(Vertex),
                 (float*)verts.data(), GL_STATIC_DRAW);

    glGenVertexArrays(1, &id);
    glBindVertexArray(id);

    enable_attrib(0, 3, offsetof(Vertex, pos));
    enable_attrib(1, 2, offsetof(Vertex, uv));

    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, bo[1]);
    glBufferData(GL_ELEMENT_ARRAY_BUFFER, elems.size()*sizeof(GLuint),
                 elems.data(), GL_STATIC_DRAW);

    glBindVertexArray(0);
    glDeleteBuffers(2, bo);
    elem_cnt = elems.size();
}

void Mesh::UpdateProjection() const {
    program->Use();
    glBindVertexArray(id);
    mat4 mvp = mat4(1.0);
    if (!still) {
        if (!billboard)
            mvp = cam.proj;
        else 
            program->Uniform("u_proj", cam.proj);
        mvp = rotate(mvp, cam.rot.x, vec3(1, 0, 0));
        mvp = rotate(mvp, cam.rot.y, vec3(0, 1, 0));
        mvp = rotate(mvp, cam.rot.z, vec3(0, 0, 1));
        mvp = translate(mvp, cam.pos);
        mvp = translate(mvp, pos);
    }
    program->Uniform("u_transform", mvp);
}

void Mesh::Draw() const {
    UpdateProjection();
    glDrawElements(GL_TRIANGLES, GetElemCnt(),
                   GL_UNSIGNED_INT, 0);
}

GLuint Mesh::GetElemCnt() const {
    return elem_cnt;
}

Mesh::~Mesh() {
    glDeleteVertexArrays(1, &id);
}

ParticleSystem::ParticleSystem(unique_ptr<Mesh> _mesh, const GLuint _max)
    : mesh(std::move(_mesh)), prog({4}, {GL_COMPUTE_SHADER}),
        max(_max) {
    mesh->billboard = true;
    glGenBuffers(SSBO_NUM, ssbo);
    glBindBuffer(GL_SHADER_STORAGE_BUFFER,
                 ssbo[SSBO_PARTICLE]);
    glBufferData(GL_SHADER_STORAGE_BUFFER,
                 max * sizeof(Particle),
                 nullptr, GL_DYNAMIC_DRAW);
    BindSSBOBase(SSBO_PARTICLE);


    DrawCmd cmd = {0};
    cmd.count = 6;
    glBindBuffer(GL_DRAW_INDIRECT_BUFFER,
                 ssbo[SSBO_DRAWCMD]);
    glBufferData(GL_DRAW_INDIRECT_BUFFER,
                 sizeof(DrawCmd),
                 &cmd, GL_DYNAMIC_DRAW);
    BindSSBOBase(SSBO_DRAWCMD);

    std::vector<GLint> is_ind_dead;
    is_ind_dead.insert(is_ind_dead.begin(), max + 1, 0);

    glBindBuffer(GL_SHADER_STORAGE_BUFFER,
                 ssbo[SSBO_DEADINDS]);
    glBufferData(GL_SHADER_STORAGE_BUFFER,
                 max * sizeof(GLint) + sizeof(float),
                 is_ind_dead.data(), GL_DYNAMIC_DRAW);
    BindSSBOBase(SSBO_DEADINDS);

    glBindBuffer(GL_SHADER_STORAGE_BUFFER,
                 0);
    glBindBuffer(GL_DRAW_INDIRECT_BUFFER,
                 0);
}

ParticleSystem::~ParticleSystem() {
    glDeleteBuffers(SSBO_NUM, ssbo);
}

void ParticleSystem::Update(const float dt, const vec3 *pos,
                            const float *mass,
                            const vec3 *vel,
                            const GLuint spawner_len,
                            const GLuint own_spawner,
                            const float spawn_time) {
    prog.Use();

    for (GLuint i = 0; i < SSBO_NUM; ++i)
        BindSSBOBase(i);

    prog.Uniform("max_particles", max);
    prog.Uniform("dt", dt);
    prog.Uniform("spawn_time", spawn_time);
    prog.Uniform("own_spawner", own_spawner);
    prog.Uniform("particle_life", 3.0f);
    prog.Uniform("spawner_mass", mass, spawner_len, 1);
    prog.Uniform("spawner_pos", (const float*)pos, spawner_len, 3);

    prog.Dispatch({max/255 + 1, 1, 1});
}

void ParticleSystem::Draw() {
    mesh->UpdateProjection();
    BindSSBOBase(SSBO_PARTICLE);
    glBindBuffer(GL_DRAW_INDIRECT_BUFFER, ssbo[SSBO_DRAWCMD]);
    glDrawElementsIndirect(GL_TRIANGLES, GL_UNSIGNED_INT, nullptr);
}

void ParticleSystem::BindSSBOBase(const GLuint id) const {
    glBindBufferBase(GL_SHADER_STORAGE_BUFFER, id,
                     ssbo[id]);
}

template<typename T>
T *const ParticleSystem::MapSSBO(GLuint ind) const {
    glBindBuffer(GL_SHADER_STORAGE_BUFFER, ssbo[ind]);
    return (T*)glMapBuffer(
                GL_SHADER_STORAGE_BUFFER,
                GL_READ_ONLY
            );
}

void ParticleSystem::PrintParticles() {
    prog.Use();
    INF("Particle Buf={}; DrawCmd Buf={}; Dead Indices Buf={}",
            ssbo[SSBO_PARTICLE],
            ssbo[SSBO_DRAWCMD],
            ssbo[SSBO_DEADINDS]);

    DrawCmd *const cmd = MapSSBO<DrawCmd>(SSBO_DRAWCMD);
    GLuint count = cmd->instanceCount;
    INF("Printing {} particles\n", count);
    glUnmapBuffer(GL_SHADER_STORAGE_BUFFER);

    GLint *const is_ind_dead = MapSSBO<GLint>(SSBO_DEADINDS);
    println("last spawn time = {}", *(float*)is_ind_dead);
    print("dead indices =\n[");
    for (GLuint i = 1; i < max + 1; ++i)
        print("{}, ", is_ind_dead[i]);
    print("\b\b]\n");

    Particle *const particles = MapSSBO<Particle>(SSBO_PARTICLE);
    for (GLuint i = 0; i < 100; ++i) {
        println("[{}]:pos=({},{},{}), vel=({},{},{}), mass={}, life={}",
                i,

                particles[i].pos.x,
                particles[i].pos.y,
                particles[i].pos.z,

                particles[i].vel.x,
                particles[i].vel.y,
                particles[i].vel.z,

                particles[i].mass,
                particles[i].life
            );
    }
    glUnmapBuffer(GL_SHADER_STORAGE_BUFFER);
    glBindBuffer(GL_SHADER_STORAGE_BUFFER, 0);
    fflush(stdout);
}
