#include "application.hpp"
#include "objects.hpp"
#include "renderer.hpp"
#include "flower_img.c"
#include <GL/gl.h>
#include <GL/glext.h>
#include <memory>
#include <vector>

#define IMG_SIZE 256

void GenTextures(Texture *tex) {
    Program tex_generator({2}, {GL_COMPUTE_SHADER});
    tex_generator.Use();
    tex_generator.Dispatch({tex},
                           {IMG_SIZE, 1, 1});
    tex_generator.FinishComputes();
}

int main() {
    // Create window
    if (CreateWindow())
        return 1;
    // Generate the floor texture
    Texture floor_tex(IMG_SIZE, IMG_SIZE, 0, 3);
    GenTextures(&floor_tex);
    floor_tex.GenerateMipMap();

    // Basic shader programs 
    shared_ptr<Program> tex_prog =
        make_shared<Program>(
            vector<GLuint>({0, 3}),
            vector<GLuint>({GL_VERTEX_SHADER, GL_FRAGMENT_SHADER}));

    // Allocate data for meshes
    vector<GLuint> quad_elems = {
        0, 1, 2, 2, 3, 0
    };
    vector<Vertex> tex_verts = {
        {{-.5, -.5, -2}, {0, 0}},
        {{ .5, -.5, -2}, {1, 0}},
        {{ .5,  .5, -2}, {1, 1}},
        {{-.5,  .5, -2}, {0, 1}},
    };

    vector<Vertex> verts = {
        {{-250, -.5, 250}, {0, 0}},
        {{ 250, -.5, 250}, {250, 0}},
        {{ 250, -.5,-250}, {250, 250}},
        {{-250, -.5,-250}, {0, 250}},
    };

    // Generate meshes using the previous data
    Mesh tex_mesh(tex_verts, quad_elems, tex_prog);
    Mesh floor_mesh(verts, quad_elems, tex_prog);

    // Create Spawners
    CreateSpawners();

    // Game loop
    while (UpdateWindow()) {
        // Update physics and interactions
        const float dt = GetDT();
        UpdatePlayer(dt);
        UpdateSpawners(dt);

        // Rendering
        floor_tex.Use(0);
        floor_mesh.Draw();
        tex_mesh.Draw();
        DrawSpawners();
        Render();
    }
    // Close everything
    CloseWindow();
    return 0;
}
