#include "objects.hpp"
#include "renderer.hpp"
#include "application.hpp"
#include "glm/common.hpp"
#include "glm/ext/scalar_constants.hpp"
#include "glm/ext/vector_float2.hpp"
#include "glm/ext/vector_float3.hpp"
#include "glm/geometric.hpp"
#include "renderer.hpp"
#include <GL/glext.h>
#include <GLFW/glfw3.h>
#include <cmath>
#include <cstdlib>
#include <vector>

#define SPEED 2
#define SENSITIVITY .7
#define MAX_DST 100
#define SPAWNER_NUM 4
#define FARTHEST_SPAWNER 5
#define G 6.67
#define GRAV 0.5f
#define EPSILON 50
#define MAX_SPAWNER_VEL 5

struct Spawners {
public:
    vec3 pos[SPAWNER_NUM];
    vec3 vel[SPAWNER_NUM];
    float mass[SPAWNER_NUM];
    float part_mass[SPAWNER_NUM];
    Mesh *(mesh[SPAWNER_NUM]);
    ParticleSystem *(particles[SPAWNER_NUM]);
};

static bool was_space_down = false;
static vec2 last_cursor = vec2(0, 0);
static vec2 pl_front = vec2(0, 1);
static vec2 pl_right = vec2(1, 0);

static Spawners spawners;

static void CalDir() {
    pl_front.x =  sin(-cam.rot.y);
    pl_front.y =  cos(-cam.rot.y);

    pl_right.x =  cos(-cam.rot.y);
    pl_right.y = -sin(-cam.rot.y);
}

void UpdatePlayer(const float dt) {
    if (IsKeyDown(GLFW_KEY_SPACE)) {
        if (!was_space_down) {
            was_space_down = true;
            ToggleCursor();
        }
    }
    else if (was_space_down)
        was_space_down = false;

    CalDir();
    vec2 cursor = GetCursorPos() - last_cursor;
    last_cursor = GetCursorPos();

    cam.rot.y += cursor.x * SENSITIVITY;
    cam.rot.x += cursor.y * SENSITIVITY;

    cam.rot.x = glm::clamp(cam.rot.x, -(glm::pi<float>()/2), (glm::pi<float>()/2));

    vec2 move = vec2(0);
    if (IsKeyDown(GLFW_KEY_S))
        move -= pl_front;
    if (IsKeyDown(GLFW_KEY_W))
        move += pl_front;
    if (IsKeyDown(GLFW_KEY_A))
        move += pl_right;
    if (IsKeyDown(GLFW_KEY_D))
        move -= pl_right;

    if (length(move))
        move = normalize(move);
    move *= SPEED * dt;
    cam.pos += vec3(move.x, 0, move.y);

    if (cam.pos.x >= MAX_DST)
        cam.pos.x -= MAX_DST;
    if (cam.pos.x <= -MAX_DST)
        cam.pos.x += MAX_DST;
    if (cam.pos.z >= MAX_DST)
        cam.pos.z -= MAX_DST;
    if (cam.pos.z <= -MAX_DST)
        cam.pos.z += MAX_DST;
}

float RandomRange(float min, float max) {
    return ((float)rand()/RAND_MAX)*(max - min) + min;
}

vec3 RandomRange(vec3 min, vec3 max) {
    return vec3(
        RandomRange(min.x, max.x),
        RandomRange(min.y, max.y),
        RandomRange(min.z, max.z)
    );
}

void CreateSpawners() {
    vector<GLuint> particle_elems = {
        0, 1, 2, 2, 3, 0
    };
    vector<Vertex> particle_verts = {
        {{-.1, -.1, 0}, {0, 0}},
        {{ .1, -.1, 0}, {1, 0}},
        {{ .1,  .1, 0}, {1, 1}},
        {{-.1,  .1, 0}, {0, 1}},
    };
    for (unsigned int i = 0; i < SPAWNER_NUM; ++i) {
        Program spawner_mesh_prog = Program({0, 6},
                                    {GL_VERTEX_SHADER,
                                    GL_FRAGMENT_SHADER});
        Mesh *mesh = new Mesh(particle_verts, particle_elems, spawner_mesh_prog);
        Program particle_prog({5, 7},
                              {GL_VERTEX_SHADER, GL_FRAGMENT_SHADER});
        Mesh particle_mesh(particle_verts, particle_elems,
                           particle_prog);
        ParticleSystem *sys = new ParticleSystem(particle_mesh, 100);

        spawners.pos[i] = RandomRange(vec3(-30, 1, -30), vec3(30, 1, 30));
        spawners.vel[i] = RandomRange(vec3(-3, 1, -3), vec3(3, 1, 3));
        spawners.mass[i] = RandomRange(30, 50);
        if (i > 2)
            spawners.part_mass[i] = -spawners.mass[i];
        else
            spawners.part_mass[i] = spawners.mass[i];
        spawners.mesh[i] = mesh;
        spawners.particles[i] = sys;
    }
}
static int rep = 0;
void UpdateSpawners(const float dt) {
    for (unsigned int i = 0; i < SPAWNER_NUM; ++i) {
        spawners.pos[i] += spawners.vel[i] * dt;
        ++rep;
        float grav_cnst = G*spawners.mass[i];
        for (unsigned int j = 0; j < SPAWNER_NUM; ++j) {
            if (i == j) 
                continue;
            float dst = distance(spawners.pos[i], spawners.pos[j]);
            float force = (grav_cnst*spawners.mass[j]) /
                (pow(dst, 2) + pow(EPSILON, 2));
            vec3 dir = normalize(spawners.pos[j]-spawners.pos[i]);
            vec3 old_vel = spawners.vel[i];
            force *= dt/spawners.mass[i];
            spawners.vel[i] += dir*force;

            spawners.vel[i] -= normalize(
                vec3(spawners.pos[i].x, 2, spawners.pos[i].z)) *
                (GRAV * dt);
        }
        spawners.vel[i] = clamp(spawners.vel[i], vec3(-4), vec3(4));
        float mag = length(spawners.vel[i]);
        if (mag > MAX_SPAWNER_VEL) {
            const vec3 dir = normalize(spawners.vel[i]);
            mag = glm::min<float>(mag, MAX_SPAWNER_VEL);
            spawners.vel[i] = dir*mag;
        }
        spawners.vel[i].y = 0;
        spawners.particles[i]->Update(dt, spawners.pos, spawners.mass,
                                      spawners.vel,
                                      SPAWNER_NUM, i);
    }
    Program::FinishComputes();
}

void DrawSpawners() {
    for (unsigned int i = 0; i < SPAWNER_NUM; ++i) {
        spawners.mesh[i]->pos = spawners.pos[i];
        // spawners.mesh[i]->Draw();
        spawners.particles[i]->Draw();
    }
}

void DestroySpawners() {
    for (unsigned int i = 0; i < SPAWNER_NUM; ++i) {
        spawners.particles[i]->Destroy();
        delete spawners.mesh[i];
        delete spawners.particles[i];
    }
}
