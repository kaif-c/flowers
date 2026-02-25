#include "objects.hpp"
#include "renderer.hpp"
#include "application.hpp"
#include "glm/common.hpp"
#include "glm/ext/scalar_constants.hpp"
#include "glm/ext/vector_float2.hpp"
#include "glm/ext/vector_float3.hpp"
#include "glm/geometric.hpp"
#include "renderer.hpp"
#include "flower_img.c"
#include <GL/gl.h>
#include <GL/glext.h>
#include <GLFW/glfw3.h>
#include <cmath>
#include <cstdlib>
#include <memory>
#include <vector>

#define SPEED 2
#define SENSITIVITY .7
#define MAX_DST 100
#define SPAWNER_NUM 3
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
    unique_ptr<ParticleSystem> particles[SPAWNER_NUM];
};

static bool was_space_down = false;
static vec2 last_cursor = vec2(0, 0);
static vec2 pl_front = vec2(0, 1);
static vec2 pl_right = vec2(1, 0);

static Spawners spawners;
static unique_ptr<Texture> particle_tex;
/**
 * \brief Calculate the front and right vectors of camera
*/
static void CalDir() {
    pl_front.x =  sin(-cam.rot.y);
    pl_front.y =  cos(-cam.rot.y);

    pl_right.x =  cos(-cam.rot.y);
    pl_right.y = -sin(-cam.rot.y);
}

static void UpdatePlayerRotation() {
    vec2 cursor = GetCursorPos() - last_cursor;
    last_cursor = GetCursorPos();

    cam.rot.y += cursor.x * SENSITIVITY;
    cam.rot.x += cursor.y * SENSITIVITY;

    cam.rot.x = glm::clamp(cam.rot.x, -(glm::pi<float>()/2), (glm::pi<float>()/2));
}

static void UpdatePlayerMovement(const float dt) {
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
}

/**
 * \brief Keeps the player from getting to the edge
 */
static void LoopPlayerPos() {
    if (cam.pos.x >= MAX_DST)
        cam.pos.x -= MAX_DST;
    if (cam.pos.x <= -MAX_DST)
        cam.pos.x += MAX_DST;
    if (cam.pos.z >= MAX_DST)
        cam.pos.z -= MAX_DST;
    if (cam.pos.z <= -MAX_DST)
        cam.pos.z += MAX_DST;
}

static void UpdateFPSCursor() {
    if (IsKeyDown(GLFW_KEY_SPACE)) {
        if (!was_space_down) {
            was_space_down = true;
            ToggleCursor();
        }
    }
    else if (was_space_down)
        was_space_down = false;
}

void UpdatePlayer(const float dt) {
    CalDir();
    UpdateFPSCursor();
    UpdatePlayerRotation();
    UpdatePlayerMovement(dt);
    LoopPlayerPos();

    if (IsKeyDown(GLFW_KEY_F1)) {
        for (int i = 0; i < SPAWNER_NUM; ++i)
            spawners.particles[i]->PrintParticles();
        exit(1);
    }
}

/**
* \return Returns random float value from min to max
*/
static float RandomRange(float min, float max) {
    return ((float)rand()/RAND_MAX)*(max - min) + min;
}

/**
* \return Returns random vec3 value from min to max (x,y,z calculated seperately)
*/
static vec3 RandomRange(vec3 min, vec3 max) {
    return vec3(
        RandomRange(min.x, max.x),
        RandomRange(min.y, max.y),
        RandomRange(min.z, max.z)
    );
}

static void CreateSpawner(const GLuint i, const vector<Vertex> &particle_verts,
                          const vector<GLuint> &particle_elems) {
    shared_ptr<Program> particle_prog = make_shared<Program>(
        vector<GLuint>({5, 7}),
        vector<GLuint>({GL_VERTEX_SHADER, GL_FRAGMENT_SHADER}));
    unique_ptr<Mesh> particle_mesh = make_unique<Mesh>(
            particle_verts, particle_elems, particle_prog);

    spawners.pos[i] = RandomRange(vec3(-30, 1, -30), vec3(30, 1, 30));
    spawners.vel[i] = RandomRange(vec3(-3, 1, -3), vec3(3, 1, 3));
    spawners.mass[i] = RandomRange(49, 51);
    spawners.particles[i] = make_unique<ParticleSystem>(
            std::move(particle_mesh), 1000000);
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
    // Using MipMaps here causes BUG
    particle_tex = make_unique<Texture>((char*)&flower_src, 0, 0);

    for (unsigned int i = 0; i < SPAWNER_NUM; ++i)
        CreateSpawner(i, particle_verts, particle_elems);
}

static void UpdateSpawnerVel(const uint i, const float grav_const, const float dt, const float time_const) {
    for (unsigned int j = 0; j < SPAWNER_NUM; ++j) {
        if (i == j) 
            continue;
        const float dst = distance(spawners.pos[i], spawners.pos[j]);
        float force = (grav_const*spawners.mass[j]) /
            (pow(dst, 2) + pow(EPSILON, 2));
        const vec3 dir = normalize(spawners.pos[j]-spawners.pos[i]);
        force *= dt/spawners.mass[i];
        spawners.vel[i] += dir*force;

        spawners.vel[i] -= normalize(
            vec3(spawners.pos[i].x, 2, spawners.pos[i].z)) *
            time_const;
    }
}

static void ClampV3(vec3 *v) {
    float mag = length(*v);
    if (mag > MAX_SPAWNER_VEL) {
        const vec3 dir = normalize(*v);
        mag = glm::min<float>(mag, MAX_SPAWNER_VEL);
        *v = dir*mag;
    }
}

static void UpdateSpawner(const uint i, const float dt) {
    spawners.pos[i] += spawners.vel[i] * dt;
    UpdateSpawnerVel(i, G*spawners.mass[i], dt, GRAV*dt);
    ClampV3(spawners.vel + i);

    spawners.vel[i].y = 0;
    spawners.particles[i]->Update(dt, spawners.pos, spawners.mass,
                                  spawners.vel,
                                  SPAWNER_NUM, i, 0.0005);
}

void UpdateSpawners(const float dt) {
    for (unsigned int i = 0; i < SPAWNER_NUM; ++i)
        UpdateSpawner(i, dt);
    Program::FinishComputes();
}

void DrawSpawners() {
    particle_tex->Use(0);
    for (unsigned int i = 0; i < SPAWNER_NUM; ++i) {
        spawners.particles[i]->Draw();
    }
}
