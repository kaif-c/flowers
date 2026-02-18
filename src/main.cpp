#include "application.hpp"
#include "objects.hpp"
#include "renderer.hpp"
#include <GL/gl.h>
#include <GL/glext.h>
#include <vector>

#define IMG_SIZE 256

int main() {
    if (CreateWindow())
        return 1;
    Texture floor_tex(IMG_SIZE, IMG_SIZE, 0);
    Texture flower_tex(IMG_SIZE, IMG_SIZE, 1);
    Program tex_generator({2}, {GL_COMPUTE_SHADER});

    tex_generator.Use();
    tex_generator.Dispatch({floor_tex, flower_tex}, {IMG_SIZE, 1, 1});
    tex_generator.FinishComputes();
    tex_generator.Destroy();

    floor_tex.GenerateMipMap();
    flower_tex.GenerateMipMap();

    vector<GLuint> quad_elems = {
        0, 1, 2, 2, 3, 0
    };

    Program floor_prog({0, 3}, {GL_VERTEX_SHADER, GL_FRAGMENT_SHADER});
    Program tex_prog({0, 3}, {GL_VERTEX_SHADER, GL_FRAGMENT_SHADER});
    vector<Vertex> tex_verts = {
        {{-.5, -.5, -2}, {0, 0}},
        {{ .5, -.5, -2}, {1, 0}},
        {{ .5,  .5, -2}, {1, 1}},
        {{-.5,  .5, -2}, {0, 1}},
    };
    Mesh tex_mesh(tex_verts, quad_elems, tex_prog);

    vector<Vertex> verts = {
        {{-250, -.5, 250}, {0, 0}},
        {{ 250, -.5, 250}, {250, 0}},
        {{ 250, -.5,-250}, {250, 250}},
        {{-250, -.5,-250}, {0, 250}},
    };
    Mesh floor_mesh(verts, quad_elems, floor_prog);

    CreateSpawners();
    while (UpdateApp()) {
        // floor_tex.Use(0);
        // floor_mesh.Draw();
        flower_tex.Use(0);
        tex_mesh.Draw();
        DrawSpawners();
    }
    DestroySpawners();
    floor_tex.Destroy();
    floor_mesh.Destroy();
    flower_tex.Destroy();
    tex_mesh.Destroy();
    CloseWindow();
    return 0;
}
