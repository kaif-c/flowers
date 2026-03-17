# Flowers

## Summary

A simple frame-rate independent [particle system](https://en.wikipedia.org/wiki/Particle_system) demo. Spawning and particle logic is all handled in compute shaders. Simple particle spawner logic is handled in CPU



![](demo.GIF)



## Dependencies

- OpenGL
- [GLFW](https://www.glfw.org/) dependencies

## Libraries Used (Static Linking)

- [GLFW](https://www.glfw.org/)

- [GLM](https://github.com/g-truc/glm)

## Compile (Release)

After cloning the repo and cd-ing into it

```sh
cmake -S. -Bbuild -DCMAKE_BUILD_TYPE=RELEASE
make -Cbuild
```

## Compile (Debug)

After cloning the repo and cd-ing into it

```sh
cmake -S. -Bbuild/release -DCMAKE_BUILD_TYPE=DEBUG
make -Cbuild
```

