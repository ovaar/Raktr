# TODO

You are tasked to work on the Raktr project.
You will contribute by doing research and implementing the following requirements iteratively.

The file `plan.md` contains the TODO list for this project. 

While iterating the planned bullet points take through you thoughts. When completing a bullet point, please mark the task item as done before continuing with the next one, using `[x]`, and save the `plan.md`.
Bullet points that have been completed must be marked with `[x]`.

Please adhere to the instructions written in `.github/copilot-instructions.md`.

## Research 

* Create a plan to address the following bullet points
* The file `research.md` contains the knowledge of previous iterations, so read that carefully. 
* After new research is complete, please document it in `research.md` and safe the file.

## Implementing

* Create a plan to address the following bullet points
* Activate the .venv, see `README.md`
* Please read the `README.md` on how to setup, configure, build and test the project.
* When implementing use a TDD approach, to first write the test and then implement.
* Run the tests before continuing
* Update the `glossary.md` to reflect the used vocabulary to other contributors.
* Finally, when done implementing don't forget to commit the changes using `git commit`, avoid commiting `plan.md` and `research.md`.

## TODO list:

* [x] Do research about what would be a good directory structure for the `raktr/engine` and `raktr/renderer`
* [x] Create basic repository folder structure.
* [x] Set up conan 2.x as a package manager
* [x] Set up cmake + ninja as a build system
* [x] Do research about what would be the MVP for rendering the Stanford Bunny model in `resources/models/bunny` using OpenGL and GLM
* [x] Research how I (the maintainer) changed the directory structure in the repository, such that projects are isolated and can be built in isolation, which improves coupling and is in line with single responsibility and seperation of concerns principles. The is how the directory structure will be from now on, so please make sure you use that from now on. Write that in `research.md`.
* [x] Research how the public api for `render` should like this. The `render` project must provide an abstraction layer for various modern rendering backends like OpenGL4, WGL, Vulkan or DirectX bindings. The goal is that we will initialize the render backend in `engine` so that `engine` can successfully initialize the render backend and use it to start and render graphics.
* [x] Implement the `render` such eventually `engine` can initialize the OpenGL graphics backend successfully through the `render` abstraction layer, but also take care of exceptional scenarios, like that graphics initialization can fail. Add unit tests in `render` to verify initializing a mocked graphics backend (without window handle) and initialize a vertex and index buffer using a OBJ (WaveFront) solid like a square:
```
v 0.5773502691896258 3.5773502691896257 0.5773502691896258
v 0.5773502691896258 3.5773502691896257 -0.5773502691896258
v -0.5773502691896258 3.5773502691896257 -0.5773502691896258
v -0.5773502691896258 3.5773502691896257 0.5773502691896258
vn 0 1 0
vn 0 1 0
f 1//1 2//1 3//1
f 1//2 3//2 4//2
```
* [x] Refactor the `MockBackend` and `MockDevice` to a `Fake`, which implements real behaviour, that can be used as a software replacement for any concrete backends like OpenGL4, WGL, Vulkan or DirectX.
* [x] Address technical dept:
```
- FakeDevice uses simple flat shading (no lighting/normals yet)
- No depth buffer in FakeBackend (will add when needed)
- OBJ loader is minimal (only positions, no normals/UVs)
```
* [x] Add basic solids as written in OBJ as text files in the `resources/` dir.
* [x] Implement WaveFront (OBJ) serialization class and add unit tests. Also add integration test to verify loading solids from `resources/`
* [x] Research how to optimally create wireframes from an vertex- and index buffer.
* [x] Research where to implement the wireframe algorithm, in `engine` or in `render`.
* [x] Implement creating wireframe algorithm (Algorithm 1: Edge Hash Set (CPU, O(N)) - âœ… Recommended primary approach), as an MVP, later we can always change it to something else if required. I do want to use meshoptimizer to optimize the vertex-, index- and wireframe buffers.
* [x] Research what we still need to build before starting on the OpenGL backend
* [x] Implement asynchronous cross-platform graphics backend using [wgpu-native](https://github.com/gfx-rs/wgpu-native)
    Implementation plan:
        Phase 1: wgpu-native integration + window ✅
        Phase 2: Basic rendering (triangle) ✅
        Phase 3: Advanced features (textures, depth) - Deferred
    Dependencies:
        wgpu-native via pre-built binaries ✅
        GLFW for windowing ✅
        WGSL for shaders (native to WebGPU) ✅
* [x] Add overload of `create_window` that takes a window handler as described in:
```cpp
/*!
 * @brief Get native platform window handle.
 * 
 * Returns platform-specific handle for graphics API initialization:
 * - Windows: HWND
 * - Linux X11: Window (XID)
 * - Linux Wayland: wl_surface*
 * - macOS: NSWindow*
 * 
 * @return Opaque pointer to native window handle.
 */
[[nodiscard]] virtual void* native_handle() const = 0;
```

Because, for example when I have a .NET application that can provide me already an HWND, I want to be able to pass that window handle to Raktr so it can use that as a surface to start drawing onto.

* [x] Implement a manual test that renders a cube in 3D and spins the cube randomly in `test_visual_triangle.cpp`
* [x] Research how we can embed using the 3d right hand rule in code. I was thinking something in the direction of strictly types like `transform_types.h`
* [x] Add helpers for strict space seperation using strictly types 
* [x] Implement the ability for the User to resize the Window
* [ ] Implement functionality the ability to switch to full-screen.
* [ ] Add User Input handlers to `engine`
* [ ] Add camera to `engine`
  * [ ] Create frustum and use WASD keys for moving the camera while the move position for rotating it.  
* [ ] Scene graph and culling
  * [ ] Implement high performance multithreaded Octree
  * [ ] Implement view (frustum) culling
  * [ ] Implement occlusion culling 
  * [ ] 
* [ ] Material system
* [ ] Batching and instancing
* [ ] Post-processing effects

## Next

* [ ] Upgrade the rendering pipeline to:
```
Level 2: Frame Pipelining 🎯 Recommended Next Step
CPU can work N frames ahead of GPU (2-3 frames in flight)
Requires: Multiple command buffers, fences/semaphores
Benefit: 20-30% better throughput, smoother frame times
Effort: Medium - need frame sync primitives
```