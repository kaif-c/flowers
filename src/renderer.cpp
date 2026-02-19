#include "renderer.hpp"
#include "application.hpp"
#include "gl_func.hpp"
#include <GL/gl.h>
#include <GL/glext.h>
#include <cstddef>
#include <cstdio>
#include <cstdlib>
#include <print>
#include <vector>
#include "glm/ext/matrix_float4x4.hpp"
#include "glm/ext/matrix_transform.hpp"
#include "glm/ext/vector_float3.hpp"
#include "glm/gtc/type_ptr.hpp"
#include "logger.hpp"

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

bool shader_compile_check(GLuint shade, GLuint type)
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

bool link_check(GLuint prog)
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

Texture::Texture(const GLsizei width, const GLsizei height,
                 const int unit) {
    glGenTextures(1, &id);
    Use(unit);

    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA8,
                 width, height, 0, GL_RGBA,
                 GL_UNSIGNED_BYTE, 0);
    glTexStorage2D(GL_TEXTURE_2D, 6, GL_RGBA8, width, height);
}

Texture::Texture(const Texture &old) {
    id = old.id;
}

void Texture::Destroy() {
    glDeleteTextures(1, &id);
}

void Texture::Use(const int unit) const {
    glActiveTexture(GL_TEXTURE0 + unit);
    glBindTexture(GL_TEXTURE_2D, id);
}

void Texture::GenerateMipMap() {
    Use(0);
    glGenerateMipmap(GL_TEXTURE_2D);
}

Program::Program(vector<GLuint> ids, vector<GLuint> types) {
    if (ids.size() != types.size()) {
        ERR("Number of ids dont match "
                "types in Program Generation");
        return;
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

void Program::Use() const {
    glUseProgram(program);
}

void Program::Dispatch(const vector<Texture> textures, const ivec3 wg) {
    for (unsigned int i = 0; i < textures.size(); ++i) {
        glBindImageTexture(i, textures[i].id, 0, GL_FALSE, 0,
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

void Program::Destroy() {
    glDeleteProgram(program);
}

static void enable_attrib(GLuint ind, GLint comps, GLuint64 offset) {
    glEnableVertexAttribArray(ind);
    glVertexAttribPointer(ind, comps, GL_FLOAT, GL_FALSE, sizeof(Vertex), (void*)offset);
}

Mesh::Mesh(const vector<Vertex> &verts,
           const vector<GLuint> &elems,
           const Program &prog)
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
    program.Use();
    glBindVertexArray(id);
    mat4 mvp = mat4(1.0);
    if (!still) {
        if (!billboard)
            mvp = cam.proj;
        else 
            program.Uniform("u_proj", cam.proj);
        mvp = rotate(mvp, cam.rot.x, vec3(1, 0, 0));
        mvp = rotate(mvp, cam.rot.y, vec3(0, 1, 0));
        mvp = rotate(mvp, cam.rot.z, vec3(0, 0, 1));
        mvp = translate(mvp, cam.pos);
        mvp = translate(mvp, pos);
    }
    program.Uniform("u_transform", mvp);
}

void Mesh::Draw() const {
    UpdateProjection();
    glDrawElements(GL_TRIANGLES, GetElemCnt(),
                   GL_UNSIGNED_INT, 0);
}

GLuint Mesh::GetElemCnt() const {
    return elem_cnt;
}

void Mesh::Destroy() {
    glDeleteVertexArrays(1, &id);
    program.Destroy();
}

ParticleSystem::ParticleSystem(const Mesh &_mesh, const GLuint _max)
    : mesh(_mesh), prog({4}, {GL_COMPUTE_SHADER}), max(_max) {

    mesh.billboard = true;
    std::vector<Particle> p;
    p.insert(p.begin(), max, (Particle){
            .pos = vec4(0, 0, -1, 0),
            .vel = vec4(0),
            .mass = 0.1,
            .life = 1
            });
    glGenBuffers(1, &particle_buf);
    glBindBuffer(GL_SHADER_STORAGE_BUFFER,
                 particle_buf);
    glBufferData(GL_SHADER_STORAGE_BUFFER,
                 max * sizeof(Particle),
                 p.data(), GL_DYNAMIC_READ_ARB);

    glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 0,
                     particle_buf);


    DrawCmd cmd = {0};
    cmd.count = 6;
    glGenBuffers(1, &draw_cmd_buf);
    glBindBuffer(GL_DRAW_INDIRECT_BUFFER,
                 draw_cmd_buf);
    glBufferData(GL_DRAW_INDIRECT_BUFFER,
                 sizeof(DrawCmd),
                 &cmd, GL_DYNAMIC_READ_ARB);
    glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 1,
                     draw_cmd_buf);

    std::vector<GLint> dead_inds_def;
    dead_inds_def.reserve(max + 1);
    dead_inds_def.push_back(0); // INFO: index 0 is a float
    for (GLuint i = 0; i < max; ++i)
        dead_inds_def.push_back(i);

    glGenBuffers(1, &dead_inds_buf);
    glBindBuffer(GL_SHADER_STORAGE_BUFFER,
                 dead_inds_buf);
    glBufferData(GL_SHADER_STORAGE_BUFFER,
                 max * sizeof(GLint) + sizeof(float),
                 dead_inds_def.data(), GL_DYNAMIC_READ_ARB);

    glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 2,
                     dead_inds_buf);

    glBindBuffer(GL_SHADER_STORAGE_BUFFER,
                 0);
    glBindBuffer(GL_DRAW_INDIRECT_BUFFER,
                 0);
}

static int reps = 0;
void ParticleSystem::Update(const float dt, const vec3 *pos,
                            const float *mass,
                            const vec3 *vel,
                            const GLuint spawner_len,
                            const GLuint own_spawner,
                            const float spawn_time) {
    prog.Use();

    glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 0,
                     particle_buf);
    glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 1,
                     draw_cmd_buf);

    prog.Uniform("max_particles", max);
    prog.Uniform("dt", dt);
    prog.Uniform("spawn_time", spawn_time);
    prog.Uniform("spawner_mass", mass, spawner_len, 1);
    prog.Uniform("spawner_pos", (const float*)pos, spawner_len, 3);

    prog.Dispatch({1, 1, 1});
    prog.FinishComputes();
    if (++reps >= 60) {
        PrintParticles();
        exit(1);
    }
}

void ParticleSystem::Draw() {
    mesh.UpdateProjection();
    glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 0,
                     particle_buf);
    glBindBuffer(GL_DRAW_INDIRECT_BUFFER, draw_cmd_buf);
    glDrawElementsIndirect(GL_TRIANGLES, GL_UNSIGNED_INT, nullptr);
}

void ParticleSystem::Destroy() {
    glDeleteBuffers(1, &particle_buf);
    glDeleteBuffers(1, &draw_cmd_buf);
    glDeleteBuffers(1, &dead_inds_buf);
    prog.Destroy();
    mesh.Destroy();
}

template<typename T>
static T *const MapSSBO(GLuint buf) {
    glBindBuffer(GL_SHADER_STORAGE_BUFFER, buf);
    return (T*)glMapBuffer(
                GL_SHADER_STORAGE_BUFFER,
                GL_READ_ONLY
            );
}

void ParticleSystem::PrintParticles() {
    prog.Use();
    INF("Particle Buf={}; DrawCmd Buf={}; Dead Indices Buf={}",
            particle_buf, draw_cmd_buf, dead_inds_buf);

    DrawCmd *const cmd = MapSSBO<DrawCmd>(draw_cmd_buf);
    GLuint count = cmd->instanceCount;
    INF("Printing {} particles\n", count);
    glUnmapBuffer(GL_SHADER_STORAGE_BUFFER);

    GLint *const dead_inds = MapSSBO<GLint>(dead_inds_buf);
    println("last spawn time = {}", *(float*)dead_inds);
    print("dead indices =\n[");
    for (GLuint i = 1; i < max + 1; ++i)
        print("{}, ", dead_inds[i]);
    print("\b\b]\n");

    Particle *const particles = MapSSBO<Particle>(particle_buf);
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
