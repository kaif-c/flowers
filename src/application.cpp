#include "glm/ext/matrix_clip_space.hpp"
#include "glm/ext/vector_float3.hpp"
#include "glm/trigonometric.hpp"
#include "objects.hpp"
#include <GL/gl.h>

#define DEF(TYPE, NAME) TYPE NAME;
#include "gl_func.hpp"

#include <GL/glext.h>
#include "application.hpp"
#include "glm/ext/vector_float2.hpp"
#include "logger.hpp"
#include <GLFW/glfw3.h>
#include <glm/glm.hpp>

#define TITLE "Sorry - Flowers"
#define BG_COLOR 3/255.0, 182/255.0, 252/255.0, 1
#define FOV 45.0f

using namespace std;
using namespace glm;

Camera cam;
static GLFWwindow *glfw_wind = nullptr;
static vec2 wind_size;

static float aspc_ratio = 0;
static bool is_mouse_locked = false;

static float last_time = 0;
static float dt;
static bool is_cursor_locked = false;

void on_resize(GLFWwindow *t_wind, int t_width, int t_height)
{
    wind_size.x = t_width;
    wind_size.y = t_height;
    aspc_ratio = wind_size.x / (float)wind_size.y;
    glViewport(0, 0, wind_size.x, wind_size.y);
    cam.proj = perspective(radians(FOV), aspc_ratio, 0.1f, 100.0f);
}

void load_gl() {
    #define DEF(TYPE, NAME) NAME = (TYPE)glfwGetProcAddress(#NAME)
    #include "gl_func.hpp"
}

void APIENTRY gl_error_callback(GLuint source, GLuint type, GLuint id,
        GLuint severity, GLsizei length, const GLchar *message,
        const void *params) {
    (void)source;
    (void)id;
    (void)length;
    (void)params;
    if (severity == GL_DEBUG_SEVERITY_NOTIFICATION) {
        INF("GL[{:x}]: {}\n", type, message);
        return;
    }

    string severity_str;
    switch (severity) {
        case GL_DEBUG_SEVERITY_HIGH:
        severity_str = "HIGH";
        break;

        case GL_DEBUG_SEVERITY_MEDIUM:
        severity_str = "MEDIUM";
        break;

        case GL_DEBUG_SEVERITY_LOW:
        severity_str = "LOW";
        break;
    }
    ERR("OpenGL error[type = 0x{:x}, severity = {}]: {}", type, severity_str, message);
    if (severity == GL_DEBUG_SEVERITY_HIGH)
        exit(type);
}

int CreateWindow() {
    if (!glfwInit())
        THROW(1, "Cannot initialize GLFW");
    const GLFWvidmode *mode = glfwGetVideoMode(glfwGetPrimaryMonitor());
    wind_size.x = mode->width, wind_size.y = mode->height;
    
    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 4);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 5);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);
    glfw_wind = glfwCreateWindow(wind_size.x, wind_size.y, TITLE, 0, 0);

    if (!glfw_wind)
        THROW(1, "Cannot create GLFW window");

    glfwMakeContextCurrent(glfw_wind);
    glfwSetFramebufferSizeCallback(glfw_wind, &on_resize);

    glfwSwapInterval(1);

    const char *glver = (char*)glGetString(GL_VERSION);
    INF("Created GLFW window\nOpenGL: {}", glver);
    load_gl();
    glEnable(GL_DEPTH_TEST);
    glDisable(GL_BLEND);
    // glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    glDisable(GL_CULL_FACE);

    glEnable(GL_DEBUG_OUTPUT);
    glDebugMessageCallback(gl_error_callback, 0);
    glDebugMessageControl(GL_DONT_CARE, GL_DONT_CARE, GL_DONT_CARE, 0, NULL, GL_TRUE);
    ToggleCursor();

    cam.proj = mat4(1);
    cam.rot = vec3(0);
    cam.pos = vec3(0);
    return 0;
}

void CloseWindow() {
    glfwTerminate();
}

void Render() {
    glfwSwapBuffers(glfw_wind);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
    glClearColor(BG_COLOR);
}

int UpdateApp() {
    glfwPollEvents();
    float time = glfwGetTime();
    dt = time - last_time;
    last_time = time;

    UpdatePlayer(dt);
    UpdateSpawners(dt);

    Render();
    return !glfwWindowShouldClose(glfw_wind);
}

bool IsKeyDown(int key) {
    return glfwGetKey(glfw_wind, key) == GLFW_PRESS;
}

void ToggleCursor() {
    glfwSetInputMode(glfw_wind, GLFW_CURSOR, is_cursor_locked?
                     GLFW_CURSOR_NORMAL : GLFW_CURSOR_DISABLED);
    is_cursor_locked = !is_cursor_locked;
}

vec2 GetCursorPos() {
    double xpos, ypos;
    glfwGetCursorPos(glfw_wind, &xpos, &ypos);
    return vec2(xpos/wind_size.x, ypos/wind_size.y);
}

float GetTime() {
    return glfwGetTime();
}
