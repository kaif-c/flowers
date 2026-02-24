#version 450 core

#define SIZE 256
#define HSIZE (SIZE/2)
#define LINE_THICKNESS 0

layout(local_size_x = SIZE, local_size_y = 1) in;
layout(binding = 0, rgba8) uniform image2D floor_tex;

void main() {
    int x = int(gl_GlobalInvocationID.x);
    for (int y = 0; y < SIZE; y++) {
        const ivec2 pos = ivec2(x, y);
        vec4 col;
        if (y >= HSIZE - LINE_THICKNESS &&
            y <= HSIZE + LINE_THICKNESS ||
            x >= HSIZE - LINE_THICKNESS &&
            x <= HSIZE + LINE_THICKNESS)
            col = vec4(1);
        else
            col = vec4(0, 0, 0, 1);

        imageStore(floor_tex, pos, col);
    }
}
