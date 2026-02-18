cmake_minimum_required(VERSION 3.11.0)
project(flower VERSION 0.0.1)

add_subdirectory(deps/glfw)
find_package(OpenGL REQUIRED)

set(CMAKE_CXX_STANDARD 23)

file(GLOB_RECURSE SRC src/*.cpp)
add_executable(${PROJECT_NAME} ${SRC})

target_include_directories(${PROJECT_NAME}
    PRIVATE deps/glfw
    PRIVATE deps/glm
)

target_link_libraries(${PROJECT_NAME} 
    PRIVATE glfw
    PRIVATE OpenGL::GL
)
