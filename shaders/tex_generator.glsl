#version 450 core

#define SIZE 256
#define HSIZE (SIZE/2)
#define RADIUS 110
#define LINE_THICKNESS 0
#define PETAL_BORDER 3
#define FLOWER_CENTRE_RAD 10

layout(local_size_x = SIZE, local_size_y = 1) in;
layout(binding = 0, rgba8) uniform image2D floor_tex;
layout(binding = 1, rgba8) uniform image2D flower_tex;

bool is_not_flower(const ivec2 pos, const ivec2 centre) {
    bool outside_flower = false;
    if (distance(centre, pos) <= FLOWER_CENTRE_RAD)
        return outside_flower;
    if (
        (pos.y <= pos.x + PETAL_BORDER && pos.y >= pos.x - PETAL_BORDER) ||
        (pos.y <= SIZE - pos.x + PETAL_BORDER && pos.y >= SIZE - pos.x - PETAL_BORDER)
       )
        outside_flower = true;
    if (distance(pos, centre) > RADIUS)
        outside_flower = true;
    return outside_flower;
}

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

        col = vec4(1);
        const ivec2 centre = ivec2(HSIZE);
        if (is_not_flower(pos, centre))
            col = vec4(0, 0, 0, 1);
        imageStore(flower_tex, pos, col);
    }
}
