#pragma once
#include "glm/ext/matrix_float4x4.hpp"
#include "glm/ext/vector_float2.hpp"
#include "glm/ext/vector_float3.hpp"
using namespace glm;

class Camera {
public:
    mat4 proj;
    vec3 rot;
    vec3 pos;
};
extern Camera cam;

int CreateWindow();
void CloseWindow();
int UpdateApp();
bool IsKeyDown(int key);
void ToggleCursor();
vec2 GetCursorPos();
float GetTime();
