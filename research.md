# Research Log

## 2025-10-13 — Directory structure for engine and renderer

Context: The repository already has `raktr/engine/{public,src}` and `raktr/renderer/{public,src}`. The project guidelines in `.github/copilot-instructions.md` prefer a unified `include/raktr/` + `src/raktr/...` layout, public API under the `raktr::` namespace, DDD-ish subsystems (core, math, ecs, render, scene, io, platform, etc.), and private headers under `detail/`.

Decision: To align with both the current repo layout and the guidelines, we’ll keep `public/` as the include-root for each module for now, but mirror the recommended namespace and submodule layout within it:

- Engine (game/platform-agnostic logic):
	- public headers in `raktr/engine/<subsystem>`
	- implementation in `src/raktr/engine/<subsystem>`
	- private headers in `src/raktr/engine/<subsystem>/detail`
	- subsystems: `core`, `math`, `ecs`, `scene`, `io`, `platform` (abstractions only), `physics`, `scripting`

- Renderer (graphics backends and pipelines):
	- public headers in `raktr/renderer/<subsystem>`
	- implementation in `src/raktr/renderer/<subsystem>`
	- private headers in `src/raktr/renderer/<subsystem>/detail`
	- subsystems: `backend` (e.g., `opengl/`), `pipeline`, `resources` (meshes, textures), `window` (creation + surface, thin wrapper), `shaders` (interfaces + reflection helpers)

Rationale:
- Matches the guideline’s separation of public API vs. implementation and keeps platform-specific code isolated under `renderer/backend/*` and `engine/platform/*`.
- Keeps namespaces coherent: `raktr::engine::core::…`, `raktr::renderer::backend::opengl::…`.
- Scales well if/when we want to adopt the global `include/raktr` layout later—only the top-level include root needs to shift.
- Encourages testable, small subsystems and a clean public API surface.

Deliverable: Create the directory scaffolding with `.keep` placeholders to make them visible in version control.

## 2025-10-13 — MVP to render the Stanford Bunny with OpenGL + GLM

Goal: Define the minimal set of code and assets required to render the bunny model once, using a fixed-function-like minimal shader pipeline (vertex + fragment), GLM for matrices, and an OpenGL backend. Use one of the reconstructed meshes in `resources/models/bunny/reconstruction` (PLY format), preferably `bun_zipper_res4.ply` for fewer triangles and faster load.

Key decisions:
- Use PLY mesh loader (initially a simple, minimal parser supporting ASCII PLY with vertex positions and faces). If the file is binary, choose the smallest ASCII decimated file or implement a tiny binary reader subset. We can start with a converter step later if needed.
- Window/context: GLFW (via Conan) would be typical, but current constraints don’t show a package setup yet. As an MVP research scope, define interfaces under `renderer/window` and a stub OpenGL context creation plan (later backed by GLFW/SDL).
- Shaders: very basic MVP with a single directional light or flat color; MVP doesn’t need normals if we do flat color; to add basic lighting, parse normals if available.
- Camera: GLM perspective + lookAt; orbit or static camera.
- Render path: upload vertex buffer + index buffer, simple VAO, draw call.

Minimal components to implement:
1) `io`: PLY loader that returns positions (and optionally normals) + indices.
2) `renderer::backend::opengl`: buffer, shader, program minimal wrappers.
3) `renderer::pipeline`: a simple render function that binds buffers and draws.
4) `renderer::window`: interface to swap buffers and poll events (stub for now).
5) `scene`: camera matrices with GLM.

Acceptance for MVP: Start a window, load `bun_zipper_res4.ply`, upload buffers, render the mesh with a flat color, controllable camera or fixed view.

Observations from the bunny mesh:
- `bun_zipper_res4.ply` is `format ascii 1.0` with header:
	- `element vertex 453` and properties: `float x y z confidence intensity`.
	- `element face 948` and one property: `list uchar int vertex_indices`.
- This means we can parse positions and ignore the extra per-vertex `confidence` and `intensity` columns. Faces are likely triangles (count is 3) but parser should read the leading count per face.

PLY loader sketch:
- Read header lines until `end_header`.
- Capture counts for vertices and faces and the vertex property layout to know how many columns to skip.
- For each vertex line: read x, y, z (first three floats) and skip any remaining vertex properties.
- For each face line: read N (uchar) followed by N indices (ints). If N == 3, push a triangle; if N > 3, triangulate fan-wise as a fallback.

Rendering pipeline pseudo-code:
1. data = io::load_ply_positions_indices(path)
2. compute bounds center = (min+max)/2 and scale to fit view if desired.
3. create window + GL context (GLFW later) and load GL symbols (GLAD later).
4. create buffers (VBO, EBO) and a VAO; upload positions and indices.
5. compile simple shaders:
	 - vertex: MVP multiply and pass through position
	 - fragment: constant color
6. set up GLM matrices: projection = perspective, view = lookAt, model = translate(-center) * scale(s)
7. main loop: clear, bind program/VAO, set MVP uniform, draw elements, swap.

Minimal interfaces to define now (headers only):
- `raktr/renderer/window/window.h`: abstract `Window` with init, should_close, poll_events, swap_buffers.
- `raktr/renderer/backend/opengl/gl_types.h`: forward-declare types or wrapping handles (to be implemented later).
- `raktr/renderer/pipeline/simple_pipeline.h`: `render(mesh, camera)` style entry point.
- `raktr/engine/io/ply_loader.h`: function returning struct `{ std::vector<glm::vec3> positions; std::vector<uint32_t> indices; }`.
- `raktr/engine/scene/camera.h`: holds view/projection matrices using GLM.

Risks and mitigations:
- Context creation requires a windowing lib (GLFW/SDL). Mitigate by stubbing interfaces and postponing actual backend selection to a later increment.
- Normal data is absent in `res4`. For flat shading MVP, no normals are required. If basic lighting is desired, compute per-face or vertex normals from positions and faces.
- Endianness/binary PLY variants aren’t needed for the chosen ASCII file.

Next steps (beyond MVP research):
- Add Conan dependencies for GLFW, GLAD, and GLM; wire CMake targets per guidelines.
- Implement the PLY loader with tests (GoogleTest) using `bun_zipper_res4.ply` as fixture.
- Implement a minimal OpenGL backend sufficient for uploading buffers and drawing.

## 2025-10-28 — Project Isolation & Independent Build Architecture

### Context

The maintainer restructured the repository to support **isolated, independently buildable projects** following SOLID principles (particularly Single Responsibility and Separation of Concerns). This architectural decision improves:

1. **Decoupling** — Each subsystem can be built and tested independently
2. **Maintainability** — Clear boundaries and ownership between engine, render, and editor
3. **Flexibility** — Each project can be packaged separately via Conan if needed
4. **Testability** — Unit tests run in isolation per project

### Directory Structure Overview

```
Raktr/                              # Repository root
├── .github/                        # CI/CD workflows and copilot instructions
├── cmake/                          # Shared CMake modules (ccache, iwyu, timetrace)
│   ├── ccache.cmake
│   ├── iwyu.cmake
│   └── timetrace.cmake
├── profiles/                       # Conan profiles (MSVC, Clang, GCC)
│   ├── msvc_vs.profile
│   └── llvm_clang_vs.profile
├── resources/                      # Shared assets (models, textures)
│   └── models/bunny/
├── scripts/                        # Build and utility scripts
├── raktr/                          # **Build root** — All buildable projects
│   ├── CMakeLists.txt              # Top-level CMake (orchestrates sub-projects)
│   ├── conanfile.py                # Conan recipe (dependencies for all projects)
│   ├── engine/                     # **Engine library** (independent project)
│   │   ├── CMakeLists.txt          # Engine-specific build config
│   │   ├── public/                 # Public API headers
│   │   │   └── raktr/engine/*.h    # Namespaced headers
│   │   ├── src/                    # Engine implementation
│   │   │   ├── core/
│   │   │   ├── ecs/
│   │   │   ├── io/
│   │   │   ├── math/
│   │   │   ├── physics/
│   │   │   ├── platform/
│   │   │   ├── scene/
│   │   │   └── scripting/
│   │   └── tests/                  # GoogleTest suites for engine
│   │       ├── CMakeLists.txt
│   │       └── test_*.cpp
│   ├── render/                     # **Renderer library** (independent project)
│   │   ├── CMakeLists.txt          # Render-specific build config
│   │   ├── public/                 # Public API headers
│   │   │   └── raktr/render/*.h    # Namespaced headers
│   │   ├── src/                    # Renderer implementation
│   │   │   ├── backend/            # Graphics API backends
│   │   │   │   ├── opengl/
│   │   │   │   ├── vulkan/
│   │   │   │   └── directx12/
│   │   │   ├── pipeline/
│   │   │   ├── resources/
│   │   │   ├── shaders/
│   │   │   └── window/
│   │   └── tests/                  # GoogleTest suites for render
│   │       ├── CMakeLists.txt
│   │       └── test_*.cpp
│   └── editor/                     # **Editor executable** (application)
│       ├── CMakeLists.txt
│       └── src/main.cpp
└── build/                          # CMake build output (gitignored)
    ├── Debug/
    └── Release/
```

### Key Architectural Principles

#### 1. **Independent CMakeLists per Project**

Each subsystem (`engine/`, `render/`, `editor/`) has its own `CMakeLists.txt`:

```cmake
# raktr/engine/CMakeLists.txt
find_package(glm REQUIRED CONFIG)
find_package(spdlog REQUIRED CONFIG)
find_package(fmt REQUIRED CONFIG)

add_library(${PROJECT_NAME}_engine STATIC ${sources})

target_include_directories(
  ${PROJECT_NAME}_engine
  PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/public   # Public API
  PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/src)    # Implementation details

target_link_libraries(${PROJECT_NAME}_engine PUBLIC glm::glm spdlog::spdlog fmt::fmt)
```

**Benefits:**
- Can be built standalone: `cmake -S raktr/engine -B build/engine`
- Clear dependency declaration
- Tests are isolated per project

#### 2. **Public API Separation**

All public headers live under `<project>/public/`:

```
engine/public/raktr/engine/
├── core/
│   ├── memory.h
│   └── logging.h
├── ecs/
│   ├── entity.h
│   └── component.h
└── engine.h
```

**Include paths:**
```cpp
#include "raktr/engine/engine.h"       // Public API
#include "raktr/engine/ecs/entity.h"   // Public subsystem API
```

**Implementation details** (private headers) go under `src/` with `detail/` subdirectories:

```
engine/src/
├── core/
│   ├── detail/
│   │   └── memory_pool.h           // Private implementation
│   └── memory.cpp
└── engine.cpp
```

#### 3. **Dependency Graph**

Projects have explicit dependencies via `target_link_libraries`:

```
editor (executable)
  ↓
  ├─→ render (library)
  │     ↓
  │     └─→ engine (library)
  │           ↓
  │           └─→ glm, spdlog, fmt (Conan packages)
  └─→ engine (library)
```

```cmake
# raktr/render/CMakeLists.txt
target_link_libraries(
  ${PROJECT_NAME}_render
  PUBLIC 
    ${PROJECT_NAME}_engine    # Render depends on engine
    glm::glm
    spdlog::spdlog
    fmt::fmt)
```

#### 4. **Shared CMake Modules**

Custom modules (`ccache.cmake`, `iwyu.cmake`, `timetrace.cmake`) are centralized at repository root under `cmake/`:

```cmake
# raktr/CMakeLists.txt (top-level)
list(APPEND CMAKE_MODULE_PATH "${CMAKE_CURRENT_LIST_DIR}/../cmake")
include(ccache)      # Note: NO .cmake extension
include(iwyu)
include(timetrace)
```

**Important:** CMake automatically appends `.cmake` when searching `CMAKE_MODULE_PATH`, so use:
- ✅ `include(ccache)` 
- ❌ `include(ccache.cmake)` — will fail

#### 5. **Conan Integration**

Single `conanfile.py` at `raktr/conanfile.py` manages all dependencies:

```python
def requirements(self):
    self.requires("glm/1.0.1")
    self.requires("mimalloc/2.2.4")
    self.requires("fmt/12.0.0")
    self.requires("spdlog/1.16.0")
    self.test_requires("gtest/1.17.0")
```

**Build flow:**
```sh
# 1. Install dependencies (from repo root)
conan install raktr --output-folder=. -pr:a=profiles/llvm_clang_vs.profile -o:a='&:with_tests=True'

# 2. Configure (from raktr/ subdirectory)
cd raktr/
cmake --preset conan-release

# 3. Build all projects
cmake --build --preset conan-release
```

Conan generates:
- `build/Release/generators/conan_toolchain.cmake`
- `build/Release/generators/Find*.cmake` for each dependency

#### 6. **Testing Strategy**

Each project has isolated tests under `<project>/tests/`:

```cmake
# raktr/engine/tests/CMakeLists.txt
find_package(GTest REQUIRED CONFIG)
include(GoogleTest)

add_executable(raktr_engine_test ${test_sources})
target_link_libraries(raktr_engine_test 
  PRIVATE 
    raktr_engine
    GTest::gtest 
    GTest::gtest_main)

gtest_discover_tests(raktr_engine_test)
```

**Run tests:**
```sh
cd build/Release
ctest --output-on-failure
```

**Benefits:**
- Tests run per-project (engine tests don't run render tests)
- Fast feedback loop
- Clear test ownership

### Design Rationale

#### Why Isolate Projects?

**Before (monolithic):**
```
raktr/
├── include/raktr/         # All headers mixed
├── src/                   # All implementations mixed
└── tests/                 # All tests mixed
```

**Problems:**
- Hard to determine dependencies
- Tests take longer (everything rebuilds)
- Coupling increases over time
- Can't package subsystems independently

**After (isolated):**
```
raktr/
├── engine/               # Self-contained
├── render/               # Self-contained
└── editor/               # Self-contained
```

**Benefits:**
- Clear responsibility boundaries (SOLID)
- Faster incremental builds (only changed projects rebuild)
- Testable in isolation
- Can package `render` as standalone library if needed

#### Why `public/` vs `include/`?

The guidelines recommend `include/raktr/`, but the current structure uses `<project>/public/`:

```cmake
target_include_directories(
  ${PROJECT_NAME}_engine
  PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/public)
```

**Rationale:**
- `public/` clearly distinguishes from implementation (`src/`)
- Scales to multiple projects without global `include/` conflicts
- Easy migration path: rename `public/` → `include/` if needed

**Include usage:**
```cpp
#include "raktr/engine/engine.h"    // Works with public/raktr/engine/engine.h
```

#### Why Namespace Per Project?

```cpp
namespace raktr::engine { /* engine code */ }
namespace raktr::render { /* render code */ }
```

**Benefits:**
- Avoids symbol collisions
- Clear ownership
- Mirrors directory structure
- Aligns with C++ Core Guidelines

### Common Patterns

#### Creating a New Subsystem

1. Add directory under `<project>/src/` or `<project>/public/raktr/<project>/`:
   ```
   engine/src/audio/
   engine/public/raktr/engine/audio/
   ```

2. Add implementation and public header:
   ```cpp
   // engine/public/raktr/engine/audio/audio_system.h
   namespace raktr::engine::audio {
       class AudioSystem { /* ... */ };
   }
   
   // engine/src/audio/audio_system.cpp
   #include "raktr/engine/audio/audio_system.h"
   namespace raktr::engine::audio {
       // Implementation
   }
   ```

3. Tests go under `<project>/tests/`:
   ```cpp
   // engine/tests/test_audio_system.cpp
   #include "raktr/engine/audio/audio_system.h"
   #include <gtest/gtest.h>
   
   TEST(AudioSystem, InitializeSuccessfully) { /* ... */ }
   ```

#### Adding a Dependency

1. Add to `raktr/conanfile.py`:
   ```python
   def requirements(self):
       self.requires("new-lib/1.0.0")
   ```

2. Reinstall Conan dependencies:
   ```sh
   conan install raktr --output-folder=. -pr:a=profiles/llvm_clang_vs.profile
   ```

3. Link in project `CMakeLists.txt`:
   ```cmake
   find_package(new-lib REQUIRED CONFIG)
   target_link_libraries(${PROJECT_NAME}_engine PUBLIC new-lib::new-lib)
   ```

### Migration Path (If Needed)

To adopt global `include/` layout later:

1. Rename `public/` → `include/` in all projects
2. Update `target_include_directories` in CMakeLists
3. No code changes needed (includes remain `raktr/<project>/*.h`)

### Summary

The current directory structure prioritizes:

✅ **Isolation** — Each project builds independently  
✅ **Clear boundaries** — Public API vs implementation separation  
✅ **SOLID principles** — Single responsibility per project  
✅ **Testability** — Isolated test suites  
✅ **Maintainability** — Explicit dependencies, clear ownership  

**Key takeaway:** From now on, all new code should respect project boundaries and use the `<project>/public/` and `<project>/src/` layout with proper namespacing (`raktr::<project>::...`).

## 2025-10-28 — Render Abstraction API Design

### Goal

Design a public API for the `render` project that provides a clean abstraction layer over multiple modern graphics backends (OpenGL 4, Vulkan, DirectX 12). The API must:

1. Allow `engine` to initialize and use any backend without knowing implementation details
2. Handle initialization failures gracefully using `std::expected` (no exceptions)
3. Support core rendering operations: buffer creation, shader compilation, draw calls
4. Follow SOLID principles: Interface Segregation, Dependency Inversion
5. Be minimal yet extensible for future features

### Architecture Overview

```
┌─────────────────────────────────────────────────────────┐
│                    raktr::engine                        │
│                                                         │
│  ┌────────────────────────────────────────────────┐   │
│  │  Uses: raktr::render::RenderContext            │   │
│  │         raktr::render::Device                  │   │
│  │         raktr::render::Buffer                  │   │
│  └────────────────────────────────────────────────┘   │
└─────────────────────────────────────────────────────────┘
                          │
                          │ depends on (public API)
                          ▼
┌─────────────────────────────────────────────────────────┐
│              raktr::render (abstraction layer)          │
│                                                         │
│  Public API:                                           │
│  ├── RenderContext   (main entry point)               │
│  ├── Device          (GPU device abstraction)         │
│  ├── Buffer          (GPU buffer handle)              │
│  ├── Shader          (shader program handle)          │
│  ├── BackendType     (enum: OpenGL, Vulkan, DX12)    │
│  └── Error           (error codes via std::errc)      │
│                                                         │
│  Internal (src/):                                      │
│  └── IBackend       (interface for backends)          │
└─────────────────────────────────────────────────────────┘
                          │
                          │ implements
                          ▼
┌─────────────────────────────────────────────────────────┐
│         raktr::render::backend::*                       │
│                                                         │
│  ├── OpenGLBackend   (src/backend/opengl/)            │
│  ├── VulkanBackend   (src/backend/vulkan/)            │
│  └── DX12Backend     (src/backend/directx12/)         │
└─────────────────────────────────────────────────────────┘
```

### Core Concepts

#### 1. **RenderContext** — Main Entry Point

The `RenderContext` is the primary interface for the engine. It manages backend lifetime and provides factory methods for resources.

```cpp
// raktr/render/public/raktr/render/render_context.h
#pragma once

#include <memory>
#include <expected>
#include <system_error>
#include "raktr/render/device.h"
#include "raktr/render/buffer.h"

namespace raktr::render
{
    /*!
     * @brief Supported graphics API backends.
     */
    enum class BackendType
    {
        OpenGL,
        Vulkan,
        DirectX12
    };

    /*!
     * @brief Configuration for initializing a render context.
     */
    struct RenderConfig
    {
        BackendType backend = BackendType::OpenGL;
        bool enable_validation = false;  // Debug layers/validation
        bool enable_vsync = true;
    };

    /*!
     * @brief Main rendering context managing backend and resources.
     */
    class RenderContext
    {
    public:
        RenderContext() = default;
        ~RenderContext();

        // Non-copyable, moveable
        RenderContext(const RenderContext&) = delete;
        RenderContext& operator=(const RenderContext&) = delete;
        RenderContext(RenderContext&&) noexcept;
        RenderContext& operator=(RenderContext&&) noexcept;

        /*!
         * @brief Initialize the render context with given configuration.
         * @param config Rendering configuration.
         * @return Success or error code.
         */
        std::expected<void, std::error_code> initialize(const RenderConfig& config);

        /*!
         * @brief Shutdown the render context and free resources.
         */
        void shutdown();

        /*!
         * @brief Get the initialized device.
         * @return Reference to the device, or nullptr if not initialized.
         */
        Device* device() const;

        /*!
         * @brief Check if context is successfully initialized.
         */
        bool is_initialized() const;

    private:
        struct Impl;
        std::unique_ptr<Impl> _impl;
    };

    /*!
     * @brief Factory function to create a render context.
     * @return Unique pointer to RenderContext.
     */
    std::unique_ptr<RenderContext> create_render_context();

} // namespace raktr::render
```

**Usage from engine:**

```cpp
// In raktr::engine
#include "raktr/render/render_context.h"

auto render_ctx = raktr::render::create_render_context();

raktr::render::RenderConfig config{
    .backend = raktr::render::BackendType::OpenGL,
    .enable_validation = true,
    .enable_vsync = true
};

auto result = render_ctx->initialize(config);
if (!result) {
    spdlog::error("Failed to initialize render context: {}", result.error().message());
    return result.error();
}

// Use render_ctx->device() for operations
```

#### 2. **Device** — GPU Device Abstraction

The `Device` represents the GPU and provides resource creation methods.

```cpp
// raktr/render/public/raktr/render/device.h
#pragma once

#include <memory>
#include <expected>
#include <system_error>
#include <span>
#include "raktr/render/buffer.h"

namespace raktr::render
{
    /*!
     * @brief GPU device abstraction for creating resources.
     */
    class Device
    {
    public:
        Device() = default;
        virtual ~Device() = default;

        // Non-copyable, non-moveable (abstract interface)
        Device(const Device&) = delete;
        Device& operator=(const Device&) = delete;

        /*!
         * @brief Create a vertex buffer.
         * @param data Vertex data to upload.
         * @param size Size in bytes.
         * @return Buffer handle or error.
         */
        virtual std::expected<Buffer, std::error_code> 
        create_vertex_buffer(std::span<const std::byte> data) = 0;

        /*!
         * @brief Create an index buffer.
         * @param data Index data to upload.
         * @param size Size in bytes.
         * @return Buffer handle or error.
         */
        virtual std::expected<Buffer, std::error_code> 
        create_index_buffer(std::span<const std::byte> data) = 0;

        /*!
         * @brief Submit a simple draw call (for MVP).
         * @param vertex_buffer Vertex buffer to bind.
         * @param index_buffer Index buffer to bind.
         * @param index_count Number of indices to draw.
         * @return Success or error.
         */
        virtual std::expected<void, std::error_code>
        draw_indexed(const Buffer& vertex_buffer, 
                    const Buffer& index_buffer, 
                    uint32_t index_count) = 0;

        /*!
         * @brief Clear the current render target.
         */
        virtual void clear() = 0;

        /*!
         * @brief Present the rendered frame (swap buffers).
         */
        virtual void present() = 0;
    };

} // namespace raktr::render
```

#### 3. **Buffer** — GPU Buffer Handle

A lightweight handle representing a GPU buffer (vertex or index).

```cpp
// raktr/render/public/raktr/render/buffer.h
#pragma once

#include <cstdint>

namespace raktr::render
{
    /*!
     * @brief Type of GPU buffer.
     */
    enum class BufferType
    {
        Vertex,
        Index
    };

    /*!
     * @brief Handle to a GPU buffer resource.
     * 
     * This is a lightweight handle that can be copied.
     * The underlying resource is managed by the backend.
     */
    class Buffer
    {
    public:
        /*!
         * @brief Construct an invalid buffer handle.
         */
        Buffer() : _id(0), _type(BufferType::Vertex) {}

        /*!
         * @brief Construct a buffer handle.
         * @param id Backend-specific buffer identifier.
         * @param type Type of buffer (vertex or index).
         */
        Buffer(uint64_t id, BufferType type) 
            : _id(id), _type(type) {}

        /*!
         * @brief Check if buffer handle is valid.
         */
        bool is_valid() const { return _id != 0; }

        /*!
         * @brief Get the buffer identifier.
         */
        uint64_t id() const { return _id; }

        /*!
         * @brief Get the buffer type.
         */
        BufferType type() const { return _type; }

    private:
        uint64_t _id;
        BufferType _type;
    };

} // namespace raktr::render
```

#### 4. **Error Handling** — Custom Error Category

Following the guidelines (no exceptions), we use `std::error_code` with a custom error category.

```cpp
// raktr/render/public/raktr/render/error.h
#pragma once

#include <system_error>

namespace raktr::render
{
    /*!
     * @brief Render subsystem error codes.
     */
    enum class RenderError
    {
        Success = 0,
        BackendNotSupported,
        InitializationFailed,
        DeviceCreationFailed,
        BufferCreationFailed,
        ShaderCompilationFailed,
        InvalidOperation
    };

    /*!
     * @brief Custom error category for render errors.
     */
    class RenderErrorCategory : public std::error_category
    {
    public:
        const char* name() const noexcept override { return "raktr::render"; }

        std::string message(int ev) const override
        {
            switch (static_cast<RenderError>(ev))
            {
                case RenderError::Success: return "Success";
                case RenderError::BackendNotSupported: return "Backend not supported";
                case RenderError::InitializationFailed: return "Initialization failed";
                case RenderError::DeviceCreationFailed: return "Device creation failed";
                case RenderError::BufferCreationFailed: return "Buffer creation failed";
                case RenderError::ShaderCompilationFailed: return "Shader compilation failed";
                case RenderError::InvalidOperation: return "Invalid operation";
                default: return "Unknown error";
            }
        }
    };

    /*!
     * @brief Get the global render error category.
     */
    inline const RenderErrorCategory& render_category()
    {
        static RenderErrorCategory category;
        return category;
    }

    /*!
     * @brief Create an error_code from a RenderError.
     */
    inline std::error_code make_error_code(RenderError e)
    {
        return {static_cast<int>(e), render_category()};
    }

} // namespace raktr::render

// Enable automatic conversion to std::error_code
namespace std
{
    template<>
    struct is_error_code_enum<raktr::render::RenderError> : true_type {};
}
```

### Implementation Strategy

#### Backend Interface (Internal)

```cpp
// raktr/render/src/backend/ibackend.h (private header)
#pragma once

#include "raktr/render/device.h"
#include "raktr/render/render_context.h"
#include <memory>
#include <expected>

namespace raktr::render::backend
{
    /*!
     * @brief Internal interface for graphics backends.
     * This is not exposed in the public API.
     */
    class IBackend
    {
    public:
        virtual ~IBackend() = default;

        /*!
         * @brief Initialize the backend.
         */
        virtual std::expected<void, std::error_code> initialize(const RenderConfig& config) = 0;

        /*!
         * @brief Shutdown the backend.
         */
        virtual void shutdown() = 0;

        /*!
         * @brief Get the device abstraction.
         */
        virtual Device* device() = 0;
    };

    /*!
     * @brief Factory to create backend implementations.
     */
    std::unique_ptr<IBackend> create_backend(BackendType type);

} // namespace raktr::render::backend
```

#### OpenGL Backend Example

```cpp
// raktr/render/src/backend/opengl/opengl_backend.h (private)
#pragma once

#include "backend/ibackend.h"
#include "backend/opengl/opengl_device.h"

namespace raktr::render::backend
{
    class OpenGLBackend : public IBackend
    {
    public:
        OpenGLBackend();
        ~OpenGLBackend() override;

        std::expected<void, std::error_code> initialize(const RenderConfig& config) override;
        void shutdown() override;
        Device* device() override { return _device.get(); }

    private:
        std::unique_ptr<OpenGLDevice> _device;
        // OpenGL context, window handle, etc.
    };

} // namespace raktr::render::backend
```

### Testing Strategy (Unit Tests)

Following TDD, we'll create tests before implementing:

```cpp
// raktr/render/tests/test_render_context.cpp
#include <gtest/gtest.h>
#include "raktr/render/render_context.h"

TEST(RenderContext, CreateContext_ReturnsValidContext)
{
    // Arrange
    auto ctx = raktr::render::create_render_context();

    // Assert
    ASSERT_NE(ctx, nullptr);
    EXPECT_FALSE(ctx->is_initialized());
}

TEST(RenderContext, Initialize_WithOpenGL_Succeeds)
{
    // Arrange
    auto ctx = raktr::render::create_render_context();
    raktr::render::RenderConfig config{
        .backend = raktr::render::BackendType::OpenGL,
        .enable_validation = false
    };

    // Act
    auto result = ctx->initialize(config);

    // Assert
    EXPECT_TRUE(result.has_value());
    EXPECT_TRUE(ctx->is_initialized());
    EXPECT_NE(ctx->device(), nullptr);
}

TEST(RenderContext, Initialize_WithUnsupportedBackend_Fails)
{
    // Arrange
    auto ctx = raktr::render::create_render_context();
    raktr::render::RenderConfig config{
        .backend = static_cast<raktr::render::BackendType>(999),  // Invalid
    };

    // Act
    auto result = ctx->initialize(config);

    // Assert
    EXPECT_FALSE(result.has_value());
    EXPECT_EQ(result.error(), raktr::render::RenderError::BackendNotSupported);
}
```

```cpp
// raktr/render/tests/test_buffer_creation.cpp
#include <gtest/gtest.h>
#include "raktr/render/render_context.h"
#include <vector>

TEST(Device, CreateVertexBuffer_WithValidData_Succeeds)
{
    // Arrange
    auto ctx = raktr::render::create_render_context();
    raktr::render::RenderConfig config{.backend = raktr::render::BackendType::OpenGL};
    ASSERT_TRUE(ctx->initialize(config).has_value());

    std::vector<float> vertices = {
        0.5f,  0.5f, 0.0f,
        0.5f, -0.5f, 0.0f,
       -0.5f, -0.5f, 0.0f
    };
    auto data = std::as_bytes(std::span{vertices});

    // Act
    auto buffer_result = ctx->device()->create_vertex_buffer(data);

    // Assert
    ASSERT_TRUE(buffer_result.has_value());
    EXPECT_TRUE(buffer_result->is_valid());
    EXPECT_EQ(buffer_result->type(), raktr::render::BufferType::Vertex);
}

TEST(Device, CreateIndexBuffer_WithValidData_Succeeds)
{
    // Arrange
    auto ctx = raktr::render::create_render_context();
    raktr::render::RenderConfig config{.backend = raktr::render::BackendType::OpenGL};
    ASSERT_TRUE(ctx->initialize(config).has_value());

    std::vector<uint32_t> indices = {0, 1, 2};
    auto data = std::as_bytes(std::span{indices});

    // Act
    auto buffer_result = ctx->device()->create_index_buffer(data);

    // Assert
    ASSERT_TRUE(buffer_result.has_value());
    EXPECT_TRUE(buffer_result->is_valid());
    EXPECT_EQ(buffer_result->type(), raktr::render::BufferType::Index);
}
```

### OBJ Square Mesh for Testing

The plan requires testing with this specific OBJ square:

```obj
v 0.5773502691896258 3.5773502691896257 0.5773502691896258
v 0.5773502691896258 3.5773502691896257 -0.5773502691896258
v -0.5773502691896258 3.5773502691896257 -0.5773502691896258
v -0.5773502691896258 3.5773502691896257 0.5773502691896258
vn 0 1 0
vn 0 1 0
f 1//1 2//1 3//1
f 1//2 3//2 4//2
```

**Test fixture:**

```cpp
// raktr/render/tests/fixtures/square.obj (test asset)
// (Content above)

// raktr/render/tests/test_obj_square.cpp
#include <gtest/gtest.h>
#include "raktr/render/render_context.h"
#include <array>

TEST(Device, UploadOBJSquare_CreatesBuffersSuccessfully)
{
    // Arrange
    auto ctx = raktr::render::create_render_context();
    ASSERT_TRUE(ctx->initialize({.backend = raktr::render::BackendType::OpenGL}).has_value());

    // Vertices from OBJ (positions only for MVP)
    std::array<float, 12> vertices = {
         0.5773502691896258f,  3.5773502691896257f,  0.5773502691896258f,  // v1
         0.5773502691896258f,  3.5773502691896257f, -0.5773502691896258f,  // v2
        -0.5773502691896258f,  3.5773502691896257f, -0.5773502691896258f,  // v3
        -0.5773502691896258f,  3.5773502691896257f,  0.5773502691896258f   // v4
    };

    // Indices from faces (OBJ uses 1-based, convert to 0-based)
    std::array<uint32_t, 6> indices = {
        0, 1, 2,  // f 1//1 2//1 3//1 → 0, 1, 2
        0, 2, 3   // f 1//2 3//2 4//2 → 0, 2, 3
    };

    // Act
    auto vb_result = ctx->device()->create_vertex_buffer(std::as_bytes(std::span{vertices}));
    auto ib_result = ctx->device()->create_index_buffer(std::as_bytes(std::span{indices}));

    // Assert
    ASSERT_TRUE(vb_result.has_value());
    ASSERT_TRUE(ib_result.has_value());

    // Optional: Attempt a draw call (may require mock window/context)
    auto draw_result = ctx->device()->draw_indexed(*vb_result, *ib_result, 6);
    // For headless testing, this might fail without a window context
    // We'll handle that in the implementation
}
```

### Mock Backend for Testing

To test without actual graphics hardware:

```cpp
// raktr/render/src/backend/mock/mock_backend.h (for tests)
#pragma once

#include "backend/ibackend.h"

namespace raktr::render::backend
{
    /*!
     * @brief Mock backend for unit testing (no actual GPU operations).
     */
    class MockBackend : public IBackend
    {
    public:
        std::expected<void, std::error_code> initialize(const RenderConfig&) override;
        void shutdown() override;
        Device* device() override;

    private:
        std::unique_ptr<Device> _mock_device;
    };

} // namespace raktr::render::backend
```

### Summary

**Public API Structure:**

```
raktr/render/public/raktr/render/
├── render_context.h     # Main entry point
├── device.h             # GPU device abstraction
├── buffer.h             # Buffer handle
├── error.h              # Error codes and category
└── types.h              # Common types (BackendType, etc.)
```

**Key Design Decisions:**

1. ✅ **Factory pattern** — `create_render_context()` hides backend selection
2. ✅ **Interface Segregation** — `Device` provides only essential methods
3. ✅ **Dependency Inversion** — Engine depends on abstractions, not concrete backends
4. ✅ **Error handling via `std::expected`** — No exceptions, explicit error propagation
5. ✅ **PImpl idiom** — `RenderContext` uses PImpl to hide implementation details
6. ✅ **RAII** — Resources are managed via smart pointers and destructors

**Next Steps (Implementation Phase):**

1. Create public API headers (above)
2. Write unit tests for `RenderContext`, `Device`, `Buffer`
3. Implement `MockBackend` for headless testing
4. Implement `OpenGLBackend` with real OpenGL calls
5. Run tests and verify they pass
6. Document integration with `engine`

## 2025-10-28 — Render Abstraction Implementation (Completed)

### Summary

Successfully implemented the render abstraction layer following TDD principles. All 10 unit tests pass, validating the design and implementation.

### Files Created

**Public API (`raktr/render/public/`):**
- `render_context.h` — Main entry point for render subsystem
- `device.h` — GPU device abstraction interface
- `buffer.h` — Lightweight GPU buffer handle
- `render_error.h` — Custom error category and error codes

**Implementation (`raktr/render/src/`):**
- `render_context.cpp` — RenderContext implementation with PImpl idiom
- `backend/ibackend.h` — Internal backend interface (not public)
- `backend/backend_factory.cpp` — Factory for creating backend instances
- `backend/mock_backend.h` — Mock backend for testing
- `backend/mock_backend.cpp` — Mock backend implementation
- `backend/mock_device.h` — Mock device for testing
- `backend/mock_device.cpp` — Mock device implementation

**Tests (`raktr/render/tests/`):**
- `test_render_context.cpp` — Tests for RenderContext lifecycle
- `test_buffer_creation.cpp` — Tests for buffer creation and validation
- `test_obj_square.cpp` — Tests for OBJ square mesh upload and drawing

### Test Results

```
[==========] Running 10 tests from 3 test suites.
[  PASSED  ] 10 tests.
```

All tests pass:
- ✅ RenderContext creation and initialization
- ✅ Backend selection (Mock backend)
- ✅ Error handling for unsupported backends
- ✅ Device retrieval
- ✅ Shutdown and cleanup
- ✅ Vertex buffer creation
- ✅ Index buffer creation
- ✅ Buffer validation (empty data fails)
- ✅ OBJ square mesh upload (4 vertices, 6 indices)
- ✅ Indexed draw call with square mesh

### Key Achievements

1. **TDD Workflow** — Tests written first, implementation followed
2. **Error Handling** — All failures use `std::expected<T, std::error_code>` (no exceptions)
3. **SOLID Principles**:
   - Single Responsibility: Each class has one clear purpose
   - Interface Segregation: Device provides only essential methods
   - Dependency Inversion: Engine will depend on abstractions, not concrete backends
4. **Testability** — Mock backend enables headless unit testing
5. **OBJ Mesh Support** — Successfully tested with the specific square mesh from requirements

### Integration Example

From `engine`, the render subsystem can be initialized as follows:

```cpp
#include "render_context.h"

// Create render context
auto render_ctx = raktr::render::create_render_context();

// Configure backend
raktr::render::RenderConfig config{
    .backend = raktr::render::BackendType::OpenGL,
    .enable_validation = true,
    .enable_vsync = true
};

// Initialize
auto result = render_ctx->initialize(config);
if (!result) {
    spdlog::error("Render initialization failed: {}", result.error().message());
    return result.error();
}

// Use device to create buffers
auto device = render_ctx->device();
auto vb_result = device->create_vertex_buffer(vertex_data);
auto ib_result = device->create_index_buffer(index_data);

if (vb_result && ib_result) {
    device->draw_indexed(*vb_result, *ib_result, index_count);
}
```

### Next Steps

1. **OpenGL Backend** — Implement `OpenGLBackend` and `OpenGLDevice` with actual GL calls
2. **Window Integration** — Add window/surface creation (GLFW or SDL via Conan)
3. **Shader Support** — Add shader compilation and program management
4. **Resource Management** — Implement buffer deletion and resource cleanup
5. **Engine Integration** — Wire render context initialization into engine startup

## 2025-10-28 — Mock vs Fake: Refactoring to Software Renderer

### Context

The current `MockBackend` and `MockDevice` are minimal test doubles that validate inputs but don't implement actual rendering behavior. The task is to refactor them into `FakeBackend` and `FakeDevice` that implement real software rendering.

### Mock vs Fake (Test Double Patterns)

**Mock (current implementation):**
- Verifies that methods are called with correct parameters
- Returns success/failure based on input validation
- Does NOT store data or perform real operations
- Minimal behavior: just enough to pass tests
- Example: `create_vertex_buffer()` validates data isn't empty, returns a handle, but doesn't store the data

**Fake (target implementation):**
- Implements real working behavior using simplified mechanisms
- Actually stores and processes data
- Can replace production code in tests without external dependencies
- Example: In-memory database, software renderer, file system simulator

### Design: Fake Software Renderer

A `FakeDevice` should be a CPU-based software renderer that:

1. **Stores buffer data** — Actually keeps vertex/index data in memory
2. **Maintains framebuffer** — 2D pixel array representing rendered output
3. **Implements rasterization** — Draws triangles to the framebuffer
4. **Supports testing** — Allows reading pixels for verification

This enables:
- ✅ True end-to-end rendering tests without GPU
- ✅ Deterministic output for pixel-perfect assertions
- ✅ No window/context dependencies
- ✅ Debugging rendering logic in tests
- ✅ Cross-platform CI without graphics drivers

### FakeDevice Architecture

```cpp
class FakeDevice : public Device
{
private:
    // Buffer storage
    struct BufferData {
        std::vector<std::byte> data;
        BufferType type;
    };
    std::unordered_map<uint64_t, BufferData> _buffers;
    uint64_t _next_buffer_id = 1;
    
    // Framebuffer
    struct Framebuffer {
        uint32_t width = 800;
        uint32_t height = 600;
        std::vector<uint32_t> pixels;  // RGBA8 format
        uint32_t clear_color = 0x000000FF;  // Black, opaque
        
        Framebuffer() : pixels(width * height, clear_color) {}
    };
    Framebuffer _framebuffer;
    
    // Rendering state
    struct RenderState {
        uint64_t bound_vertex_buffer = 0;
        uint64_t bound_index_buffer = 0;
    };
    RenderState _state;
};
```

### Benefits of Fake Implementation

1. **True behavior testing** — Tests verify rendering actually works, not just that APIs are called
2. **Debugging** — Can inspect framebuffer to see what was drawn
3. **Deterministic** — Same input always produces same pixels (no GPU variance)
4. **Fast** — No GPU/window overhead, runs in CI
5. **Educational** — Implements real rendering concepts (rasterization, coordinate transforms)
6. **Portable** — Works everywhere, no drivers needed

### Implementation Plan

1. ✅ Research and document Fake design (this section)
2. Rename `MockBackend` → `FakeBackend`, `MockDevice` → `FakeDevice`
3. Implement buffer storage in `FakeDevice`
4. Implement framebuffer and clear operation
5. Implement simple triangle rasterization
6. Add pixel readback methods for testing
7. Update existing tests to use Fake and add pixel assertion tests
8. Update backend factory to use `BackendType::Fake`

### References

- **Test Double Patterns**: Martin Fowler's "Mocks Aren't Stubs"
- **Software Rasterization**: Scratchapixel 2.0 - Rasterization Algorithm
- **Triangle Rasterization**: Edge function method (barycentric coordinates)



## 2025-10-29 � Technical Debt Resolution: FakeDevice Improvements & OBJ Loader

### Context

After implementing the initial FakeBackend with basic triangle rasterization, three technical debt items remained:
1. FakeDevice used simple flat shading (no lighting/normals)
2. No depth buffer implementation
3. OBJ loader was minimal (positions only, no normals/UVs)

### Implementation: Enhanced FakeDevice

#### Depth Buffer
- Added depth buffer as std::vector<float> alongside framebuffer pixels
- Depth values: 0.0 = near plane, 1.0 = far plane
- Simple depth interpolation using triangle center z-coordinate
- API additions:
  - enable_depth_test(bool) - Toggle depth testing
  - clear_depth_buffer() - Reset all depth values to 1.0 (far plane)
  - is_depth_test_enabled() - Query depth test state

#### Lighting System
- Implemented simple directional lighting with ambient + diffuse components
- Light direction: (0, 1, 0) - pointing upward
- Lighting calculation: intensity = ambient + (1 - ambient) * max(0, dot(normal, light_dir))
- Ambient factor: 0.3 (30% base illumination)
- Flat shading: averages triangle vertex normals for uniform triangle color

#### Vertex Format Support
- Added VertexFormat enum to specify interleaved vertex layout:
  - Position (3 floats: x, y, z)
  - PositionNormal (6 floats: x, y, z, nx, ny, nz)
  - PositionUV (5 floats: x, y, z, u, v)
  - PositionNormalUV (8 floats: x, y, z, nx, ny, nz, u, v)
- API: set_vertex_format(VertexFormat) - tells device how to parse vertex data
- Separate rasterization paths:
  - 
asterize_triangle() - basic positions only
  - 
asterize_triangle_with_normals() - with lighting calculation

#### Color Application
- calculate_lighting(normal) - computes intensity from normal vector
- pply_lighting_to_color(base_color, intensity) - scales RGB by intensity, preserves alpha
- Base color: white (0xFFFFFFFF) modulated by lighting

### Implementation: Complete OBJ Loader

#### Features
- **Full WaveFront OBJ support**:
  - Vertex positions ( x y z)
  - Texture coordinates (t u v)
  - Vertex normals (n nx ny nz)
  - Face indices with all variations:
    -  v1 v2 v3 - positions only
    -  v1/vt1 v2/vt2 v3/vt3 - positions + UVs
    -  v1//vn1 v2//vn2 v3//vn3 - positions + normals
    -  v1/vt1/vn1 v2/vt2/vn2 v3/vt3/vn3 - all attributes

#### Automatic Triangulation
- Handles quads and n-gons via triangle fan algorithm
- Quad (4 vertices) ? 2 triangles: (0,1,2), (0,2,3)
- N-gon ? (n-2) triangles

#### Index Expansion
- OBJ format uses shared vertex pools with separate indices for pos/uv/normal
- Loader expands to per-vertex attributes (duplicates vertices as needed)
- Output format matches GPU expectations (one index per vertex attribute set)

#### Error Handling
- Returns std::expected<MeshData, ObjError>
- Error cases:
  - InvalidFormat - malformed face indices, out-of-bounds references
  - FileNotFound - file path doesn't exist
  - ParseError - numeric parsing failure
- Uses C++23 std::from_chars for efficient, locale-independent parsing

#### API
`cpp
namespace raktr::render::io {
    struct MeshData {
        std::vector<glm::vec3> positions;
        std::vector<glm::vec3> normals;
        std::vector<glm::vec2> uvs;
        std::vector<uint32_t> indices;
    };
    
    std::expected<MeshData, ObjError> load_obj(std::istream& stream);
    std::expected<MeshData, ObjError> load_obj_file(const std::string& filepath);
}
`

### Test Coverage

Added 11 new tests across 5 test suites:

#### OBJ Loader Tests (8 tests)
- ObjLoader_ParsePositions - Basic position-only triangles
- ObjLoader_ParseNormals - Normals with  v//vn syntax
- ObjLoader_ParseUVs - Texture coordinates with  v/vt syntax
- ObjLoader_ParseFull - All attributes with  v/vt/vn syntax
- ObjLoader_ParseSquare - Square from plan.md (2 triangles)
- ObjLoader_ParseQuad - Automatic quad triangulation
- ObjLoader_ErrorHandling - Empty mesh, invalid indices

#### FakeDevice Depth Tests (3 tests)
- EnableDepthTest_InitializesDepthBuffer - State initialization
- ClearDepthBuffer_ResetsAllDepthValues - Buffer clearing
- CloserTriangleOccludesFartherTriangle - Depth ordering verification

#### FakeDevice Lighting Tests (2 tests)
- VerticesWithNormals_ApplyLighting - Lighting calculation
- NormalsPointingAway_ProduceDarkerColor - Directional light validation

### Test Results

All 27 tests pass (up from 14):
- 11 new tests for technical debt features
- 16 existing tests still passing
- Total runtime: ~185ms

### Technical Decisions

1. **Flat shading over interpolation** - Simpler for testing, sufficient for validation
2. **Simple depth interpolation** - Uses triangle center z, not full barycentric (good enough for fake)
3. **Per-vertex expansion in OBJ loader** - Trades memory for simplicity and GPU compatibility
4. **std::from_chars for parsing** - Fast, locale-independent, C++23 standard
5. **[[maybe_unused]] for parse_float** - Reserved for future use (e.g., vertex weights)

### Compliance

? **C++23** - std::expected, std::from_chars, concepts  
? **TDD** - Tests written first, then implementation  
? **SOLID** - Single responsibility, clear interfaces  
? **Documentation** - Doxygen comments on all public APIs  
? **Error Handling** - No exceptions, explicit error types  
? **Testing** - GoogleTest with Triple-A pattern  

### Performance Notes

- OBJ parsing: O(V + F) where V = vertices, F = faces
- Depth buffer: Adds ~3MB for 800x600 (width � height � 4 bytes)
- Lighting: Negligible overhead (per-triangle calculation)
- Rasterization: Still O(pixels_in_bbox), unchanged

### Future Enhancements

Deferred to maintain MVP scope:
- Perspective-correct interpolation for normals/UVs
- Phong/Gouraud shading (per-vertex lighting)
- Multiple light sources
- Texture sampling from UVs
- OBJ material (.mtl) support
- Binary OBJ/PLY loaders


## 2025-10-30 — Wireframe Rendering from Vertex/Index Buffers

### Context

Wireframe rendering displays only the edges of polygons instead of filled faces, useful for:
- **Debugging** geometry and topology
- **Visualization** of mesh structure
- **Modeling tools** showing underlying mesh
- **Technical diagrams** in documentation
- **Performance profiling** (fewer pixels to fill)

Goal: Research optimal algorithms to create wireframe line lists from triangle mesh vertex/index buffers.

### Problem Definition

**Input:**
- Vertex buffer: Array of vertex positions (and attributes)
- Index buffer: Array of indices forming triangles (every 3 indices = 1 triangle)

**Output:**
- Line index buffer: Array of indices forming lines (every 2 indices = 1 line segment)

**Requirements:**
1. Extract unique edges from triangle mesh
2. Remove duplicate edges (shared between adjacent triangles)
3. Maintain correct vertex references
4. Efficient for real-time rendering (preprocessing acceptable)

### Challenge: Duplicate Edges

A closed triangle mesh has shared edges between adjacent faces:

```
Triangle mesh:
    1-----2
   / \   / \
  /   \ /   \
 0-----3-----4

Triangles (indices):
  T0: [0, 1, 3]
  T1: [1, 2, 3]
  T2: [2, 3, 4]

Edges (naive extraction):
  T0: (0,1), (1,3), (3,0)  <- 6 edges from 2 triangles
  T1: (1,2), (2,3), (3,1)  <- (1,3) duplicates (3,1)!
  T2: (2,3), (3,4), (4,2)  <- (2,3) duplicates (3,2)!
  
Unique edges: 
  (0,1), (0,3), (1,2), (1,3), (2,3), (2,4), (3,4)  = 7 unique
```

**Naive approach** (no deduplication): 3 edges/triangle × N triangles = 3N edges  
**Optimal** (shared edges): ~1.5N edges for closed mesh (Euler's formula: E ≈ 3V/2)

### Algorithm 1: Edge Hash Set (CPU-Based)

**Approach:** Build a set of unique edges by normalizing edge representation.

#### Pseudocode

```cpp
struct Edge {
    uint32_t v0, v1;
    
    // Normalize: always store min index first
    Edge(uint32_t a, uint32_t b) 
        : v0(std::min(a, b)), v1(std::max(a, b)) {}
    
    bool operator==(const Edge& other) const {
        return v0 == other.v0 && v1 == other.v1;
    }
};

// Hash function for std::unordered_set
struct EdgeHash {
    size_t operator()(const Edge& e) const {
        return std::hash<uint64_t>()((uint64_t(e.v0) << 32) | e.v1);
    }
};

std::vector<uint32_t> extract_wireframe_indices(
    const std::vector<uint32_t>& triangle_indices)
{
    std::unordered_set<Edge, EdgeHash> unique_edges;
    
    // Extract edges from triangles
    for (size_t i = 0; i + 2 < triangle_indices.size(); i += 3) {
        uint32_t v0 = triangle_indices[i];
        uint32_t v1 = triangle_indices[i + 1];
        uint32_t v2 = triangle_indices[i + 2];
        
        unique_edges.insert(Edge(v0, v1));
        unique_edges.insert(Edge(v1, v2));
        unique_edges.insert(Edge(v2, v0));
    }
    
    // Convert to line index buffer
    std::vector<uint32_t> line_indices;
    line_indices.reserve(unique_edges.size() * 2);
    
    for (const auto& edge : unique_edges) {
        line_indices.push_back(edge.v0);
        line_indices.push_back(edge.v1);
    }
    
    return line_indices;
}
```

#### Complexity
- **Time:** O(N) where N = number of triangles (hash insertion is O(1) average)
- **Space:** O(E) where E = number of unique edges (~1.5N for closed mesh)

#### Pros
- ✅ Simple to implement
- ✅ Handles arbitrary topology (open/closed meshes, non-manifold)
- ✅ Works with any vertex format (only needs indices)
- ✅ Efficient for one-time preprocessing

#### Cons
- ❌ CPU-based (not real-time for dynamic meshes)
- ❌ Requires copying data to CPU if mesh is GPU-resident
- ❌ Hash set allocation overhead

### Algorithm 2: Sorted Edge List (Cache-Friendly)

**Approach:** Sort edges and scan for duplicates in linear pass.

#### Pseudocode

```cpp
std::vector<uint32_t> extract_wireframe_indices_sorted(
    const std::vector<uint32_t>& triangle_indices)
{
    std::vector<Edge> edges;
    edges.reserve(triangle_indices.size()); // 3 edges per triangle
    
    // Extract all edges
    for (size_t i = 0; i + 2 < triangle_indices.size(); i += 3) {
        uint32_t v0 = triangle_indices[i];
        uint32_t v1 = triangle_indices[i + 1];
        uint32_t v2 = triangle_indices[i + 2];
        
        edges.push_back(Edge(v0, v1));
        edges.push_back(Edge(v1, v2));
        edges.push_back(Edge(v2, v0));
    }
    
    // Sort edges (normalized, so duplicates are adjacent)
    std::sort(edges.begin(), edges.end(), 
        [](const Edge& a, const Edge& b) {
            return (a.v0 < b.v0) || (a.v0 == b.v0 && a.v1 < b.v1);
        });
    
    // Remove duplicates
    edges.erase(std::unique(edges.begin(), edges.end()), edges.end());
    
    // Convert to line indices
    std::vector<uint32_t> line_indices;
    line_indices.reserve(edges.size() * 2);
    
    for (const auto& edge : edges) {
        line_indices.push_back(edge.v0);
        line_indices.push_back(edge.v1);
    }
    
    return line_indices;
}
```

#### Complexity
- **Time:** O(N log N) due to sorting, O(N) for deduplication
- **Space:** O(N) for edge array

#### Pros
- ✅ More cache-friendly than hash set (sequential access)
- ✅ Deterministic output order
- ✅ No hash collisions
- ✅ Better memory locality

#### Cons
- ❌ Slower than hash set for large meshes (O(N log N) vs O(N))
- ❌ Still CPU-based

### Algorithm 3: Geometry Shader (GPU-Based)

**Approach:** Use geometry shader to emit line primitives from triangles on GPU.

#### GLSL Geometry Shader

```glsl
#version 450 core

layout(triangles) in;
layout(line_strip, max_vertices = 6) out;  // 3 edges × 2 vertices

in VS_OUT {
    vec3 position;
} gs_in[];

out vec3 frag_position;

void main() {
    // Emit 3 edges of the triangle
    for (int i = 0; i < 3; i++) {
        int next = (i + 1) % 3;
        
        // First vertex of edge
        gl_Position = gl_in[i].gl_Position;
        frag_position = gs_in[i].position;
        EmitVertex();
        
        // Second vertex of edge
        gl_Position = gl_in[next].gl_Position;
        frag_position = gs_in[next].position;
        EmitVertex();
        
        EndPrimitive();  // Complete line segment
    }
}
```

#### Complexity
- **Time:** O(1) per triangle (parallel GPU execution)
- **Space:** O(N) edge output (3 edges per triangle, includes duplicates)

#### Pros
- ✅ GPU-accelerated (extremely fast for dynamic meshes)
- ✅ No CPU preprocessing
- ✅ Works with animated meshes in real-time
- ✅ No data upload/download overhead

#### Cons
- ❌ Draws duplicate edges (no deduplication)
- ❌ 2× overdraw for shared edges (wastes bandwidth)
- ❌ Requires geometry shader support (not available in all backends)
- ❌ Geometry shaders have performance pitfalls on some GPUs

### Algorithm 4: Adjacency Information (Advanced)

**Approach:** Build half-edge data structure for efficient edge queries.

#### Half-Edge Structure

```cpp
struct HalfEdge {
    uint32_t vertex;       // Destination vertex
    uint32_t face;         // Adjacent face
    uint32_t next;         // Next half-edge in face
    uint32_t twin;         // Opposite half-edge (or -1 if boundary)
};

struct Mesh {
    std::vector<glm::vec3> vertices;
    std::vector<HalfEdge> halfedges;
    std::vector<uint32_t> face_to_halfedge;  // One half-edge per face
};
```

#### Edge Extraction

```cpp
std::vector<uint32_t> extract_wireframe_from_halfedge(const Mesh& mesh) {
    std::vector<uint32_t> line_indices;
    std::unordered_set<uint32_t> visited;
    
    for (const auto& he : mesh.halfedges) {
        // Only process each edge once (via lower-index half-edge)
        if (he.twin == -1 || he.vertex < mesh.halfedges[he.twin].vertex) {
            if (visited.find(&he - mesh.halfedges.data()) == visited.end()) {
                uint32_t v0 = mesh.halfedges[he.next].vertex;  // Start of edge
                uint32_t v1 = he.vertex;                       // End of edge
                
                line_indices.push_back(v0);
                line_indices.push_back(v1);
                
                visited.insert(&he - mesh.halfedges.data());
                if (he.twin != -1) visited.insert(he.twin);
            }
        }
    }
    
    return line_indices;
}
```

#### Complexity
- **Build:** O(N) to construct half-edge structure
- **Query:** O(E) to extract unique edges
- **Space:** O(N) for half-edge storage

#### Pros
- ✅ Efficient for meshes requiring many edge queries
- ✅ Supports complex topology operations (subdivision, simplification)
- ✅ No duplicate edges by construction
- ✅ O(1) adjacency queries

#### Cons
- ❌ Complex to implement correctly
- ❌ High memory overhead (3× half-edges vs triangles)
- ❌ Overkill for simple wireframe extraction

### Algorithm 5: Indexed Line List (Precomputed)

**Approach:** Precompute wireframe indices and store alongside triangle indices.

#### Storage

```cpp
struct MeshBuffers {
    std::vector<glm::vec3> vertices;
    std::vector<uint32_t> triangle_indices;  // For solid rendering
    std::vector<uint32_t> line_indices;      // For wireframe rendering
};
```

#### Usage

```cpp
// Precompute once (e.g., during asset loading)
mesh.line_indices = extract_wireframe_indices(mesh.triangle_indices);

// Render solid
device->draw_indexed(mesh.triangle_indices, GL_TRIANGLES);

// Render wireframe
device->draw_indexed(mesh.line_indices, GL_LINES);
```

#### Complexity
- **Precompute:** O(N) using Algorithm 1 or 2
- **Runtime:** O(1) (just draw call)

#### Pros
- ✅ Zero runtime cost
- ✅ Supports any rendering backend
- ✅ Simple to integrate
- ✅ Efficient for static meshes

#### Cons
- ❌ Extra storage (~1.5N indices for lines)
- ❌ Requires preprocessing step
- ❌ Not suitable for dynamic meshes (need to recompute)

### Comparison Matrix

| Algorithm               | Time         | Space    | GPU  | Duplicates | Complexity | Best For                  |
|-------------------------|--------------|----------|------|------------|------------|---------------------------|
| **Edge Hash Set**       | O(N)         | O(E)     | ❌   | None       | Low        | Static meshes, one-time   |
| **Sorted Edge List**    | O(N log N)   | O(N)     | ❌   | None       | Low        | Cache-friendly, offline   |
| **Geometry Shader**     | O(N) GPU     | O(N)     | ✅   | 2× edges   | Medium     | Dynamic/animated meshes   |
| **Half-Edge**           | O(N)         | O(3N)    | ❌   | None       | High       | Complex topology ops      |
| **Precomputed**         | O(1) runtime | O(N+E)   | Both | None       | Low        | Static meshes, any backend|

### Recommended Approach for Raktr

Given Raktr's goals (learning, cross-platform, multiple backends), I recommend:

**Primary: Algorithm 1 (Edge Hash Set) with Precomputation**

Rationale:
1. ✅ **Simple implementation** — Easy to understand and debug
2. ✅ **Backend-agnostic** — Works with FakeDevice, OpenGL, Vulkan, DirectX
3. ✅ **Optimal** — O(N) time, no duplicates
4. ✅ **TDD-friendly** — Easy to unit test
5. ✅ **MVP-appropriate** — Minimal complexity, proven approach

**Secondary: Geometry Shader (for real-time dynamic meshes)**

Use geometry shader only when:
- Mesh changes every frame (skeletal animation, deformation)
- Preprocessing is impractical
- GPU bandwidth isn't constrained

### Implementation Strategy

#### Phase 1: CPU-Based Extraction (MVP)

```cpp
// raktr/render/public/raktr/render/mesh/wireframe.h
#pragma once

#include <vector>
#include <cstdint>

namespace raktr::render::mesh {

    /*!
     * @brief Extract unique edges from triangle mesh for wireframe rendering.
     * @param triangle_indices Index buffer with triangles (every 3 indices = 1 triangle).
     * @return Line index buffer (every 2 indices = 1 line segment).
     * @example
     * std::vector<uint32_t> tri_indices = {0, 1, 2, 1, 3, 2};  // 2 triangles
     * auto line_indices = extract_wireframe_indices(tri_indices);
     * // line_indices = {0,1, 1,2, 2,0, 1,3, 3,2} — 5 unique edges
     */
    std::vector<uint32_t> extract_wireframe_indices(
        const std::vector<uint32_t>& triangle_indices);

} // namespace raktr::render::mesh
```

#### Phase 2: Integration with MeshData

```cpp
// Extend MeshData from OBJ loader
struct MeshData {
    std::vector<glm::vec3> positions;
    std::vector<glm::vec3> normals;
    std::vector<glm::vec2> uvs;
    std::vector<uint32_t> indices;              // Triangle indices
    std::vector<uint32_t> wireframe_indices;    // Line indices (optional)
};

// Usage
auto mesh_result = io::load_obj_file("model.obj");
if (mesh_result) {
    auto& mesh = *mesh_result;
    mesh.wireframe_indices = mesh::extract_wireframe_indices(mesh.indices);
    
    // Upload both buffers
    auto tri_ib = device->create_index_buffer(mesh.indices);
    auto line_ib = device->create_index_buffer(mesh.wireframe_indices);
}
```

#### Phase 3: Rendering API

```cpp
// Extend Device interface
class Device {
public:
    virtual std::expected<void, std::error_code>
    draw_indexed(const Buffer& vertex_buffer, 
                const Buffer& index_buffer, 
                uint32_t index_count,
                PrimitiveType primitive = PrimitiveType::Triangles) = 0;
};

enum class PrimitiveType {
    Triangles,  // Default
    Lines,      // For wireframe
    Points      // For point clouds
};

// Render solid and wireframe
device->draw_indexed(vb, triangle_ib, tri_count, PrimitiveType::Triangles);
device->draw_indexed(vb, wireframe_ib, line_count, PrimitiveType::Lines);
```

### Testing Strategy

#### Unit Tests

```cpp
// raktr/render/tests/test_wireframe_extraction.cpp

TEST(WireframeExtraction, SingleTriangle_Returns3Edges) {
    // Arrange
    std::vector<uint32_t> tri_indices = {0, 1, 2};
    
    // Act
    auto line_indices = extract_wireframe_indices(tri_indices);
    
    // Assert
    EXPECT_EQ(line_indices.size(), 6);  // 3 edges × 2 vertices
    // Edges: (0,1), (1,2), (0,2)
}

TEST(WireframeExtraction, TwoTrianglesSharedEdge_RemovesDuplicate) {
    // Arrange
    //   1
    //  /|\
    // 0-2-3
    std::vector<uint32_t> tri_indices = {0, 1, 2,  1, 3, 2};
    
    // Act
    auto line_indices = extract_wireframe_indices(tri_indices);
    
    // Assert
    EXPECT_EQ(line_indices.size(), 10);  // 5 unique edges × 2 vertices
    // Edges: (0,1), (0,2), (1,2), (1,3), (2,3)
}

TEST(WireframeExtraction, Cube_ReturnsAllEdges) {
    // Arrange: Cube has 12 edges, 6 faces, 12 triangles
    auto cube_mesh = io::load_obj_file("resources/models/primitives/cube.obj");
    ASSERT_TRUE(cube_mesh.has_value());
    
    // Act
    auto line_indices = extract_wireframe_indices(cube_mesh->indices);
    
    // Assert
    EXPECT_EQ(line_indices.size(), 24);  // 12 edges × 2 vertices
}
```

#### Integration Tests

```cpp
TEST(FakeDevice, RenderWireframe_ProducesLines) {
    // Arrange
    auto ctx = create_render_context();
    ctx->initialize({.backend = BackendType::Fake});
    
    auto mesh = io::load_obj_file("triangle.obj");
    mesh->wireframe_indices = extract_wireframe_indices(mesh->indices);
    
    auto vb = ctx->device()->create_vertex_buffer(mesh->positions);
    auto line_ib = ctx->device()->create_index_buffer(mesh->wireframe_indices);
    
    // Act
    ctx->device()->clear();
    ctx->device()->draw_indexed(*vb, *line_ib, 6, PrimitiveType::Lines);
    
    // Assert
    auto pixels = ctx->device()->read_framebuffer();
    // Verify line pixels are set (not just triangle fill)
}
```

### Performance Considerations

#### Optimization 1: Reserve Capacity

```cpp
std::vector<uint32_t> line_indices;
line_indices.reserve(triangle_indices.size() * 2);  // Upper bound: 3 edges per tri
```

#### Optimization 2: Parallel Extraction (Future)

```cpp
#include <execution>

std::sort(std::execution::par, edges.begin(), edges.end());
```

#### Optimization 3: Spatial Hashing (Large Meshes)

For meshes with >1M triangles, consider spatial hashing to reduce collision probability:

```cpp
struct EdgeHash {
    size_t operator()(const Edge& e) const {
        // FNV-1a hash for better distribution
        size_t hash = 2166136261u;
        hash ^= e.v0; hash *= 16777619u;
        hash ^= e.v1; hash *= 16777619u;
        return hash;
    }
};
```

### Alternative Rendering Techniques

#### 1. Polygon Offset (OpenGL)

Render solid then wireframe with depth offset:

```cpp
// Render solid
glEnable(GL_DEPTH_TEST);
glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);
draw_triangles();

// Render wireframe on top
glEnable(GL_POLYGON_OFFSET_LINE);
glPolygonOffset(-1.0f, -1.0f);  // Pull towards camera
glPolygonMode(GL_FRONT_AND_BACK, GL_LINE);
draw_triangles();  // Same geometry!
```

**Pros:**
- ✅ No index buffer conversion needed
- ✅ Handles coincident geometry

**Cons:**
- ❌ OpenGL-specific
- ❌ Still draws shared edges twice
- ❌ PolygonMode not available in modern APIs (Vulkan/DirectX)

#### 2. Stencil Buffer Edges

Use stencil to mark edge pixels during solid rendering.

**Approach:**
- Draw triangles with stencil increment on front faces
- Draw again with stencil decrement on back faces
- Edges have stencil ≠ 0 (drawn once from one side)

**Verdict:** Too complex for wireframe extraction; better for silhouette detection.

### Conclusion

**Recommended implementation:**
1. **CPU-based edge hash set** (Algorithm 1) for static meshes
2. **Precompute during asset load** and store alongside triangle indices
3. **Extend Device API** with `PrimitiveType::Lines` support
4. **Unit tests** verify deduplication correctness
5. **Integration tests** ensure rendering works across backends

**Next steps:**
1. Implement `extract_wireframe_indices()` in `raktr/render/src/mesh/wireframe.cpp`
2. Add unit tests for edge extraction
3. Extend FakeDevice to support line rasterization
4. Update OBJ loader to optionally compute wireframe indices
5. Add integration tests rendering wireframe primitives

**Deferred to future iterations:**
- Geometry shader approach (when OpenGL backend implemented)
- Half-edge data structure (if topology operations needed)
- GPU-based extraction via compute shader

## 2025-10-31 â€” Wireframe Algorithm Placement: Engine vs Render

### Context

Decision needed: Should the wireframe extraction algorithm (`extract_wireframe_indices()`) be implemented in the `engine` or `render` project?

### Analysis

#### Current Architecture

**Engine Project:**
- **Purpose**: Game logic, platform-agnostic systems
- **Subsystems**: `core`, `math`, `ecs`, `scene`, `io`, `platform`, `physics`, `scripting`
- **Responsibility**: High-level game systems, entity management, physics simulation
- **Dependencies**: Does NOT depend on render (independent)

**Render Project:**
- **Purpose**: Graphics backend abstraction, rendering primitives
- **Subsystems**: `backend`, `pipeline`, `resources`, `window`, `shaders`, `io`
- **Responsibility**: GPU resource management, rendering API abstraction, mesh loading
- **Dependencies**: Depends on engine (for math utilities via GLM)
- **Current mesh handling**: `io/obj_loader.h` defines `MeshData` structure

#### Key Observations

1. **MeshData Ownership**: `MeshData` is defined in `raktr::render::io` namespace
2. **OBJ Loader Location**: Already in `render/public/io/obj_loader.h`
3. **Wireframe is Render Primitive**: Lines are a rendering primitive type (like triangles, points)
4. **GPU Buffer Interaction**: Wireframe indices become GPU index buffers via `Device::create_index_buffer()`
5. **Render Concepts**: Terminology (vertices, indices, primitives) is render-domain language

#### Architectural Principles

Following SOLID and project isolation guidelines from research.md:

**Single Responsibility Principle:**
- **Render**: Responsible for mesh data structures, GPU resource preparation, rendering primitives
- **Engine**: Responsible for game logic, entity systems, physics

**Dependency Rule:**
- Render depends on engine (can use engine utilities)
- Engine does NOT depend on render (must stay independent)
- Wireframe extraction uses `MeshData` from render â†’ should stay in render

**Cohesion:**
- Wireframe extraction is tightly coupled to mesh representation (`MeshData`)
- It transforms rendering data (triangle indices â†’ line indices)
- Result is consumed by render backend (`Device::draw_indexed(..., PrimitiveType::Lines)`)

### Comparison Matrix

| Aspect                  | Engine Placement                          | Render Placement                          |
|-------------------------|-------------------------------------------|-------------------------------------------|
| **Cohesion**            | âŒ Low (mesh data lives in render)       | âœ… High (next to OBJ loader, MeshData)   |
| **Dependencies**        | âŒ Would need MeshData from render       | âœ… Already has MeshData                   |
| **Conceptual Fit**      | âŒ Meshes not core engine concern        | âœ… Mesh processing is render concern      |
| **Future Extensibility**| âŒ Hard to add GPU-based algorithms      | âœ… Easy to add geometry shader approach   |
| **Testability**         | âœ… Can test independently                | âœ… Can test with existing render tests    |
| **Reusability**         | âš ï¸ Only if engine knows about meshes    | âœ… Available to all render backends       |

### Decision: Implement in `render` Project

**Rationale:**

1. âœ… **Natural Placement**: Mesh processing utilities belong with mesh loading (OBJ loader already in `render/io`)
2. âœ… **Data Locality**: `MeshData` is defined in `raktr::render::io` namespace
3. âœ… **Rendering Primitive**: Wireframe is a rendering concept (line primitives), not game logic
4. âœ… **Backend Integration**: Result directly consumed by `Device` interface (already in render)
5. âœ… **Future GPU Algorithms**: Easy to add compute shader or geometry shader variants when backends support them
6. âœ… **No Cross-Project Pollution**: Keeps render-specific concepts out of engine
7. âœ… **Follows Existing Patterns**: Similar to how OBJ loader transforms file data â†’ MeshData

**Specific Location:**

```
raktr/render/
â”œâ”€â”€ public/
â”‚   â””â”€â”€ mesh/                        # New subsystem for mesh utilities
â”‚       â””â”€â”€ wireframe.h              # Wireframe extraction API
â””â”€â”€ src/
    â””â”€â”€ mesh/
        â””â”€â”€ wireframe.cpp            # Implementation
```

**Alternative considered but rejected:**
- `render/io/` â€” Wireframe extraction is NOT I/O (doesn't read/write files)
- `render/resources/` â€” Too generic; mesh utilities deserve specific namespace
- `engine/` â€” Would violate separation of concerns and dependency rules

### API Design

```cpp
// raktr/render/public/mesh/wireframe.h
#pragma once

#include <vector>
#include <cstdint>

namespace raktr::render::mesh {

    /*!
     * @brief Extract unique edges from triangle mesh for wireframe rendering.
     * 
     * Uses edge hash set algorithm (O(N)) to deduplicate shared edges
     * between adjacent triangles.
     * 
     * @param triangle_indices Index buffer with triangles (every 3 indices = 1 triangle).
     * @return Line index buffer (every 2 indices = 1 line segment).
     * 
     * @example
     * std::vector<uint32_t> tri_indices = {0, 1, 2, 1, 3, 2};  // 2 triangles
     * auto line_indices = extract_wireframe_indices(tri_indices);
     * // line_indices = {0,1, 1,2, 2,0, 1,3, 3,2} â€” 5 unique edges
     */
    std::vector<uint32_t> extract_wireframe_indices(
        const std::vector<uint32_t>& triangle_indices);

} // namespace raktr::render::mesh
```

### Integration with Existing Code

#### 1. Extend MeshData (Optional)

```cpp
// In raktr/render/public/io/obj_loader.h
namespace raktr::render::io {
    struct MeshData {
        std::vector<glm::vec3> positions;
        std::vector<glm::vec3> normals;
        std::vector<glm::vec2> uvs;
        std::vector<uint32_t> indices;              // Triangle indices
        std::vector<uint32_t> wireframe_indices;    // Line indices (optional, computed on demand)
    };
}
```

#### 2. Usage Pattern

```cpp
#include "io/obj_loader.h"
#include "mesh/wireframe.h"

// Load mesh
auto mesh_result = raktr::render::io::load_obj_file("model.obj");
if (!mesh_result) return mesh_result.error();

auto& mesh = *mesh_result;

// Extract wireframe
mesh.wireframe_indices = raktr::render::mesh::extract_wireframe_indices(mesh.indices);

// Upload to GPU
auto vb = device->create_vertex_buffer(mesh.positions);
auto tri_ib = device->create_index_buffer(mesh.indices);
auto wire_ib = device->create_index_buffer(mesh.wireframe_indices);

// Render solid
device->draw_indexed(*vb, *tri_ib, mesh.indices.size(), PrimitiveType::Triangles);

// Render wireframe
device->draw_indexed(*vb, *wire_ib, mesh.wireframe_indices.size(), PrimitiveType::Lines);
```

### Benefits of This Design

1. **Encapsulation**: Mesh utilities grouped under `raktr::render::mesh` namespace
2. **Discoverability**: Easy to find alongside OBJ loader in render subsystem
3. **Extensibility**: Can add more mesh utilities (normals generation, tangent calculation, etc.)
4. **Backend Agnostic**: Algorithm is CPU-based, works with any render backend
5. **Testable**: Can unit test independently with synthetic triangle meshes
6. **No Cross-Dependencies**: Render project already has all needed types

### Future Enhancements (Deferred)

When adding more mesh processing utilities, consider:

```
raktr/render/public/mesh/
â”œâ”€â”€ wireframe.h           # Edge extraction
â”œâ”€â”€ normals.h             # Normal generation/smoothing
â”œâ”€â”€ tangents.h            # Tangent space calculation
â”œâ”€â”€ simplification.h      # Mesh LOD generation
â””â”€â”€ optimization.h        # Vertex cache optimization (meshoptimizer integration)
```

### Conclusion

**Wireframe extraction belongs in the `render` project under the `mesh` subsystem.**

This decision:
- âœ… Maintains clear separation of concerns (render handles mesh data)
- âœ… Follows existing patterns (OBJ loader already in render)
- âœ… Enables future GPU-based variants
- âœ… Keeps engine project lean and focused on game logic
- âœ… Provides natural namespace organization

**Implementation path:**
1. Create `raktr/render/public/mesh/wireframe.h`
2. Implement `raktr/render/src/mesh/wireframe.cpp`
3. Add unit tests in `raktr/render/tests/test_wireframe_extraction.cpp`
4. Optionally extend `MeshData` with `wireframe_indices` field
5. Update `Device` interface to support `PrimitiveType::Lines` (if not already done)

---

## 2025-10-31 — Prerequisites for OpenGL4 Backend Implementation

### Context

Before implementing the OpenGL4 backend, we need to assess what foundational components are missing from the current `render` project architecture. The goal is to identify gaps between our current abstraction layer and what's required for a real hardware backend.

### Current State Analysis

#### âœ… What We Have

1. **Core Abstractions**
   - `Device` interface with buffer creation and basic drawing
   - `RenderContext` with backend selection enum (`BackendType::OpenGL`)
   - `Buffer` handle abstraction
   - `RenderConfig` for initialization parameters
   - Error handling via `std::expected<T, std::error_code>`

2. **Resource Management**
   - Vertex/index buffer interfaces (`create_vertex_buffer`, `create_index_buffer`)
   - Basic `draw_indexed()` function
   - `clear()` and `present()` operations

3. **I/O & Mesh Processing**
   - OBJ loader producing `MeshData` (positions, normals, UVs, indices)
   - Wireframe extraction algorithm (edge deduplication)
   - Test primitives (cube, pyramid, sphere, plane, triangle)

4. **Testing Infrastructure**
   - `FakeBackend` and `FakeDevice` for software rendering tests
   - Depth buffer support in FakeDevice
   - Basic lighting calculations (dot product with normals)
   - 44 passing tests covering buffers, OBJ loading, wireframe extraction

5. **Build System**
   - Conan 2.x with dependencies: GLM, fmt, spdlog, mimalloc, meshoptimizer, gtest
   - CMake with independent project structure (engine/render/editor)
   - ClangCL toolset with C++23 support

#### âŒ What We're Missing

The following components are **critical gaps** that must be filled before OpenGL4 backend implementation:

##### 1. Window & Context Management ðŸ"´ CRITICAL

**Current State:**
- Empty `raktr/render/public/window/` directory
- No windowing library in Conan dependencies
- `FakeBackend` creates context without actual window

**Required:**
- Window abstraction (`Window` interface)
- Native window handles (HWND on Windows, etc.)
- OpenGL context creation and management
- Surface/swapchain abstraction
- Event polling (resize, close, input - though input may be engine concern)

**Dependencies Needed:**
- **GLFW** (recommended) or **SDL3** via Conan
- Both provide cross-platform window + OpenGL context creation

**API Design Sketch:**
```cpp
namespace raktr::render {
    struct WindowConfig {
        uint32_t width = 1280;
        uint32_t height = 720;
        const char* title = "Raktr";
        bool resizable = true;
        bool fullscreen = false;
    };

    class Window {
    public:
        virtual ~Window() = default;
        
        virtual bool should_close() const = 0;
        virtual void poll_events() = 0;
        virtual void swap_buffers() = 0;
        
        virtual uint32_t width() const = 0;
        virtual uint32_t height() const = 0;
        
        // Platform-specific handle for context creation
        virtual void* native_handle() const = 0;
    };

    std::expected<std::unique_ptr<Window>, std::error_code>
    create_window(const WindowConfig& config);
}
```

##### 2. OpenGL Function Loader ðŸ"´ CRITICAL

**Current State:**
- No OpenGL headers or loader
- Can't call any `gl*` functions

**Required:**
- Modern OpenGL function loading (OpenGL 4.5+ uses function pointers)
- Extension detection
- Debug context support (for `GL_KHR_debug`)

**Dependencies Needed:**
- **GLAD** (recommended, lightweight, customizable) or **GLEW**
- Generate via [glad.dav1d.de](https://glad.dav1d.de/) for OpenGL 4.5 Core + extensions

**Integration:**
```cpp
// After creating OpenGL context
if (!gladLoadGLLoader((GLADloadproc)glfwGetProcAddress)) {
    return std::unexpected(RenderError::OpenGLLoadFailed);
}
```

##### 3. Shader System ðŸ"´ CRITICAL

**Current State:**
- Empty `raktr/render/public/shaders/` directory
- No shader compilation/linking infrastructure
- FakeDevice uses hardcoded software "shaders"

**Required:**
- Shader compilation (vertex, fragment, geometry, compute)
- Program linking
- Uniform/attribute binding
- Shader reflection (optional but useful)
- Error reporting with line numbers

**API Design Sketch:**
```cpp
namespace raktr::render {
    enum class ShaderStage {
        Vertex,
        Fragment,
        Geometry,
        Compute
    };

    class Shader {
    public:
        virtual ~Shader() = default;
        // Backend-specific handle access
    };

    class ShaderProgram {
    public:
        virtual ~ShaderProgram() = default;
        
        virtual void bind() = 0;
        virtual void set_uniform(const char* name, const float* data, size_t count) = 0;
        virtual void set_uniform(const char* name, int value) = 0;
        // ... more uniform setters
    };

    // Device extensions
    virtual std::expected<Shader, std::error_code>
    create_shader(ShaderStage stage, const char* source) = 0;

    virtual std::expected<ShaderProgram, std::error_code>
    create_program(std::span<const Shader> shaders) = 0;
}
```

**Basic Shaders Needed:**
```glsl
// vertex.glsl
#version 450 core
layout(location = 0) in vec3 a_position;
layout(location = 1) in vec3 a_normal;

uniform mat4 u_mvp;
uniform mat4 u_model;

out vec3 v_normal;
out vec3 v_world_pos;

void main() {
    gl_Position = u_mvp * vec4(a_position, 1.0);
    v_normal = mat3(u_model) * a_normal;
    v_world_pos = (u_model * vec4(a_position, 1.0)).xyz;
}

// fragment.glsl
#version 450 core
in vec3 v_normal;
in vec3 v_world_pos;

out vec4 frag_color;

uniform vec3 u_light_dir;
uniform vec3 u_light_color;
uniform vec3 u_object_color;

void main() {
    vec3 norm = normalize(v_normal);
    float diff = max(dot(norm, -u_light_dir), 0.0);
    vec3 color = u_object_color * u_light_color * (0.2 + 0.8 * diff);
    frag_color = vec4(color, 1.0);
}
```

##### 4. Vertex Layout & Input Assembly ðŸŸ¡ IMPORTANT

**Current State:**
- `Buffer` is opaque handle
- No vertex attribute description
- FakeDevice assumes hardcoded layout (position, normal, uv)

**Required:**
- Vertex attribute specification (location, format, offset, stride)
- Vertex Array Objects (VAO) for OpenGL
- Input layout state

**API Design Sketch:**
```cpp
namespace raktr::render {
    enum class VertexFormat {
        Float,
        Float2,
        Float3,
        Float4,
        // ... more formats
    };

    struct VertexAttribute {
        uint32_t location;
        VertexFormat format;
        uint32_t offset;
    };

    struct VertexLayout {
        std::vector<VertexAttribute> attributes;
        uint32_t stride;
    };

    // Device extension
    virtual void set_vertex_layout(const VertexLayout& layout) = 0;
}
```

**For OpenGL Backend:**
- Each unique vertex layout creates a VAO
- Cache VAOs to avoid redundant state changes

##### 5. Pipeline State Management ðŸŸ¡ IMPORTANT

**Current State:**
- FakeDevice has hardcoded depth testing
- No state objects for blend/raster/depth-stencil

**Required:**
- Depth/stencil state (test enable, write mask, compare func)
- Blend state (enable, src/dst factors, blend op)
- Rasterizer state (cull mode, front face, polygon mode)
- Viewport and scissor state

**API Design Sketch:**
```cpp
namespace raktr::render {
    enum class CompareFunc { Never, Less, Equal, LessEqual, Greater, NotEqual, GreaterEqual, Always };
    enum class CullMode { None, Front, Back, FrontAndBack };
    enum class FrontFace { CW, CCW };
    enum class PolygonMode { Fill, Line, Point };

    struct DepthStencilState {
        bool depth_test_enable = true;
        bool depth_write_enable = true;
        CompareFunc depth_func = CompareFunc::Less;
        // ... stencil fields
    };

    struct RasterizerState {
        CullMode cull_mode = CullMode::Back;
        FrontFace front_face = FrontFace::CCW;
        PolygonMode polygon_mode = PolygonMode::Fill;
    };

    struct Viewport {
        float x, y, width, height;
        float min_depth = 0.0f;
        float max_depth = 1.0f;
    };

    // Device extensions
    virtual void set_depth_stencil_state(const DepthStencilState& state) = 0;
    virtual void set_rasterizer_state(const RasterizerState& state) = 0;
    virtual void set_viewport(const Viewport& viewport) = 0;
}
```

##### 6. Texture Support ðŸŸ¡ IMPORTANT

**Current State:**
- No texture creation interface
- OBJ loader provides UVs but we don't upload them
- No sampler abstraction

**Required (for MVP):**
- 2D texture creation from pixel data
- Texture binding to shader slots
- Basic sampler states (filter, wrap mode)

**API Design Sketch:**
```cpp
namespace raktr::render {
    enum class TextureFormat {
        R8_UNORM,
        RG8_UNORM,
        RGBA8_UNORM,
        RGBA8_SRGB,
        // ... more formats
    };

    enum class TextureFilter { Nearest, Linear };
    enum class TextureWrap { Repeat, ClampToEdge, MirroredRepeat };

    struct TextureConfig {
        uint32_t width;
        uint32_t height;
        TextureFormat format;
        TextureFilter min_filter = TextureFilter::Linear;
        TextureFilter mag_filter = TextureFilter::Linear;
        TextureWrap wrap_s = TextureWrap::Repeat;
        TextureWrap wrap_t = TextureWrap::Repeat;
        bool generate_mipmaps = true;
    };

    class Texture {
    public:
        virtual ~Texture() = default;
    };

    // Device extension
    virtual std::expected<Texture, std::error_code>
    create_texture_2d(const TextureConfig& config, std::span<const std::byte> pixels) = 0;

    virtual void bind_texture(uint32_t slot, const Texture& texture) = 0;
}
```

**Note:** Texture support can be deferred if we render with solid colors initially. However, it's needed for:
- Normal mapping
- Material textures (albedo, roughness, metallic)
- Environment maps
- Shadow maps

##### 7. Framebuffer Abstraction ðŸŸ¢ NICE-TO-HAVE

**Current State:**
- FakeDevice has internal framebuffer (`_pixels`, `_depth_buffer`)
- OpenGL uses default framebuffer initially

**Required (eventually):**
- Offscreen render targets
- Depth/stencil attachments
- Multisampling (MSAA)
- Multiple render targets (MRT)

**Can defer** until we need:
- Post-processing effects
- Shadow mapping
- Deferred rendering
- G-buffer for PBR

##### 8. Resource Lifecycle & Error Handling ðŸŸ¢ NICE-TO-HAVE

**Current State:**
- Buffers use `uint32_t` handles (id-based)
- No explicit destroy methods
- FakeDevice stores resources in vectors

**Required (for production):**
- RAII wrappers or explicit resource destruction
- Resource invalidation after context loss
- Debug labels for GPU objects (via `GL_KHR_debug`)
- Memory usage tracking

**Can defer** but should plan API:
```cpp
// Option 1: RAII (preferred for C++)
class Buffer {
    uint32_t _handle;
public:
    ~Buffer() { /* call device->destroy_buffer(_handle) */ }
    Buffer(const Buffer&) = delete; // non-copyable
    Buffer(Buffer&&) = default;     // moveable
};

// Option 2: Explicit (more control)
virtual void destroy_buffer(const Buffer& buffer) = 0;
virtual void destroy_texture(const Texture& texture) = 0;
```

### Dependency Priority Matrix

| Component                  | Priority | Reason                                      | Conan Package          |
|----------------------------|----------|---------------------------------------------|------------------------|
| **Window + Context**       | ðŸ"´ P0    | Can't create OpenGL context without it      | `glfw/3.4`            |
| **OpenGL Loader**          | ðŸ"´ P0    | Can't call any OpenGL functions             | GLAD (generate)       |
| **Shader System**          | ðŸ"´ P0    | OpenGL requires programmable pipeline       | Built-in (our code)   |
| **Vertex Layout**          | ðŸŸ¡ P1    | Needed for correct vertex attribute setup   | Built-in              |
| **Pipeline State**         | ðŸŸ¡ P1    | Needed for depth test, culling, wireframe   | Built-in              |
| **Texture Support**        | ðŸŸ¡ P2    | Can render solid colors initially           | stb_image (optional)  |
| **Framebuffer Abstraction**| ðŸŸ¢ P3    | Default framebuffer sufficient for MVP      | Built-in              |
| **Resource Lifecycle**     | ðŸŸ¢ P3    | Important for production, not MVP blocker   | Built-in              |

### Recommended Implementation Order

#### Phase 1: Window & Context (P0) âœ… MUST DO FIRST

1. Add GLFW to Conan dependencies
2. Create `raktr/render/public/window/window.h` interface
3. Implement `raktr/render/src/window/glfw_window.cpp`
4. Add window creation tests (can we create window? get size? poll events?)
5. Integrate with `RenderContext::initialize()`

**Files to create:**
- `raktr/render/public/window/window.h`
- `raktr/render/src/window/glfw_window.h` (private)
- `raktr/render/src/window/glfw_window.cpp`
- `raktr/render/tests/test_window.cpp`

#### Phase 2: OpenGL Loader (P0) âœ… MUST DO FIRST

1. Generate GLAD files for OpenGL 4.5 Core + KHR_debug
2. Add GLAD sources to project (typically `external/glad/`)
3. Link GLAD with render project
4. Add OpenGL context initialization after window creation

**Files to create:**
- `external/glad/glad.h`
- `external/glad/glad.c`
- Update `CMakeLists.txt` to compile GLAD

#### Phase 3: Shader System (P0) âœ… MUST DO FIRST

1. Extend `Device` interface with shader operations
2. Create `raktr/render/public/shaders/shader.h`
3. Implement OpenGL shader compilation in `opengl_device.cpp`
4. Create basic vertex + fragment shader pair
5. Add shader compilation tests

**Files to create:**
- `raktr/render/public/shaders/shader.h`
- `raktr/render/src/backend/opengl/opengl_shader.h`
- `raktr/render/src/backend/opengl/opengl_shader.cpp`
- `resources/shaders/basic.vert`
- `resources/shaders/basic.frag`
- `raktr/render/tests/test_shader_compilation.cpp`

#### Phase 4: OpenGL Backend Implementation (P0)

1. Create `raktr/render/src/backend/opengl/opengl_device.h`
2. Implement `create_vertex_buffer` with `glGenBuffers`/`glBufferData`
3. Implement `create_index_buffer` similarly
4. Implement `draw_indexed` with `glDrawElements`
5. Implement `clear` with `glClear`
6. Implement `present` delegating to window swap
7. Add OpenGL backend to `RenderContext` factory

**Files to create:**
- `raktr/render/src/backend/opengl/opengl_device.h`
- `raktr/render/src/backend/opengl/opengl_device.cpp`
- `raktr/render/src/backend/opengl/opengl_backend.h`
- `raktr/render/src/backend/opengl/opengl_backend.cpp`
- `raktr/render/tests/test_opengl_backend.cpp` (requires window!)

#### Phase 5: Vertex Layout & Pipeline State (P1)

1. Add `VertexLayout` to public API
2. Implement VAO management in OpenGL backend
3. Add pipeline state structs
4. Implement state setting functions

#### Phase 6: Texture Support (P2)

1. Add texture interface to `Device`
2. Implement OpenGL texture creation
3. Optionally integrate stb_image for loading

#### Phase 7: Advanced Features (P3+)

- Framebuffer objects
- Geometry shaders
- Compute shaders
- Multi-draw indirect
- Resource barriers

### Testing Strategy

#### Unit Tests (Can Do Now)

- âœ… Shader source parsing/validation (string manipulation)
- âœ… Vertex layout calculations (stride, alignment)
- âœ… State object construction

#### Integration Tests (Need Window)

- Window creation and destruction
- OpenGL context creation
- Shader compilation and linking
- Buffer upload and binding
- Full draw call (triangle test)

#### System Tests (Need Renderer)

- Render cube and compare against reference image
- Wireframe mode toggle
- Depth test validation
- Multi-object rendering

### Migration Path for Existing Tests

Our current `FakeBackend` tests will remain valuable:

1. Keep `FakeBackend` for **headless testing** (CI, unit tests)
2. Add `OpenGLBackend` for **visual validation** (manual testing, screenshots)
3. Both backends implement same `Device` interface
4. Tests can run against both (via test fixtures)

Example:
```cpp
class DeviceTest : public ::testing::TestWithParam<BackendType> {
    std::unique_ptr<RenderContext> context;
    
    void SetUp() override {
        context = create_render_context();
        RenderConfig config;
        config.backend = GetParam();
        ASSERT_TRUE(context->initialize(config));
    }
};

INSTANTIATE_TEST_SUITE_P(AllBackends, DeviceTest,
    ::testing::Values(BackendType::Fake, BackendType::OpenGL));
```

### Risks & Mitigations

| Risk                                  | Mitigation                                      |
|---------------------------------------|-------------------------------------------------|
| GLFW platform compatibility           | Use Conan package (handles platform specifics) |
| OpenGL context creation failures      | Robust error reporting, fallback to Fake       |
| Shader compilation errors             | Store line numbers, return detailed logs       |
| State management complexity           | Start simple (global state), refactor later    |
| Testing requires GPU                  | Keep FakeBackend for CI, OpenGL for dev only   |
| GLAD generation customization         | Document exact GLAD options used               |

### Conan Dependency Updates Needed

Add to `raktr/conanfile.py`:

```python
def requirements(self):
    # ... existing
    self.requires("glfw/3.4")              # Window + OpenGL context
    # GLAD is header-only generated, add to repo
    
    # Optional (Phase 6+)
    # self.requires("stb/cci.20230920")   # Image loading
```

### Glossary Updates Needed

Add to `glossary.md`:

- **VAO (Vertex Array Object)**: OpenGL object storing vertex attribute configuration
- **Shader Program**: Linked combination of vertex/fragment/geometry shaders
- **Uniform**: Shader constant (e.g., transformation matrix)
- **Framebuffer**: Render target (default = window, custom = offscreen)
- **Context**: OpenGL state machine instance tied to a window

### Conclusion

**To implement OpenGL4 backend, we MUST first:**

1. ðŸ"´ Add GLFW dependency + implement Window abstraction
2. ðŸ"´ Generate and integrate GLAD for OpenGL function loading
3. ðŸ"´ Design and implement Shader system (compile/link/uniforms)
4. ðŸŸ¡ Add Vertex Layout specification
5. ðŸŸ¡ Add Pipeline State management

**After these prerequisites, we can:**

- Implement `OpenGLDevice` and `OpenGLBackend`
- Port existing tests to run on real GPU
- Render primitives with actual hardware acceleration
- Begin work on advanced rendering features

**Estimated Effort:**
- Phase 1 (Window): 1-2 tasks (interface + GLFW impl)
- Phase 2 (GLAD): 1 task (generate + integrate)
- Phase 3 (Shaders): 2-3 tasks (interface + OpenGL impl + basic shaders)
- Phase 4 (OpenGL Device): 2-3 tasks (backend + device + tests)
- **Total before rendering**: ~7-10 tasks

**Next Steps:**
1. Create task breakdown for Phase 1 (Window abstraction)
2. Update Conan dependencies
3. Begin implementation following TDD methodology

---

## 2025-11-01 — Decision: WebGPU (wgpu-native) as Graphics Backend

### Context

After researching prerequisites for implementing manual OpenGL/Vulkan/DirectX backends, we evaluated whether to:
1. Implement backends manually (maximum learning, high effort)
2. Use a rendering abstraction layer (faster, production-ready)

### Graphics Abstraction Layer Comparison

#### Evaluated Options

| Library | OpenGL | Vulkan | DirectX | Metal | WebGPU | License | Maturity |
|---------|--------|--------|---------|-------|--------|---------|----------|
| **wgpu-native** | ✅ (via compat) | ✅ | ✅ (D3D12) | ✅ | ✅ | MIT/Apache | ⭐⭐⭐ |
| **bgfx** | ✅ | ✅ | ✅ (D3D9/11/12) | ✅ | ✅ | BSD | ⭐⭐⭐⭐⭐ |
| **IGL (Meta)** | ✅ | ✅ | ❌ | ✅ | ✅ | MIT | ⭐⭐⭐⭐⭐ |
| **SDL3** | ✅ (ctx) | ✅ (surface) | ⚠️ (HWND) | ✅ (ctx) | ❌ | zlib | ⭐⭐⭐⭐⭐ |
| **Manual** | ✅ | ✅ | ✅ | ✅ | ❌ | - | ⭐ (our impl) |

#### Why wgpu-native?

**Advantages:**

1. **Future-Proof Standard** ⭐⭐⭐⭐⭐
   - Based on WebGPU W3C standard (future of graphics on the web)
   - Modern API design influenced by Vulkan/Metal/DirectX12
   - Will be natively supported in all major browsers

2. **Complete Backend Coverage** ⭐⭐⭐⭐⭐
   - Vulkan (Windows, Linux, Android)
   - DirectX 12 (Windows)
   - Metal (macOS, iOS)
   - OpenGL/ES (fallback compatibility)
   - WebGPU (browser via wasm)

3. **Excellent Cross-Platform Support** ⭐⭐⭐⭐⭐
   - Windows (DX12, Vulkan, OpenGL)
   - Linux (Vulkan, OpenGL)
   - macOS (Metal, OpenGL)
   - Web (WebGPU, WebGL2)
   - Mobile (Vulkan, Metal, OpenGL ES)

4. **Modern Architecture** ⭐⭐⭐⭐⭐
   - Command buffer based (like Vulkan/Metal)
   - Explicit resource management
   - Async/async-friendly design
   - Lower overhead than OpenGL state machine

5. **Safe & Validated** ⭐⭐⭐⭐
   - Written in Rust (memory safety)
   - C API for FFI (wgpu-native)
   - Built-in validation layers
   - Excellent error reporting

6. **Active Development** ⭐⭐⭐⭐
   - Maintained by gfx-rs team
   - Used in production (Firefox, Deno)
   - Regular updates following WebGPU spec

7. **C++ Integration** ⭐⭐⭐⭐
   - C header (`webgpu.h`)
   - Can create C++ wrappers (`webgpu.hpp` style)
   - No runtime dependency on Rust after build

**Trade-offs:**

1. **Learning Curve** ⚠️
   - New API (WebGPU spec, not raw OpenGL/Vulkan)
   - Modern concepts (bind groups, pipeline layouts)
   - Different from legacy OpenGL state machine
   - **Mitigation**: Better long-term investment, cleaner abstraction

2. **Rust Build Dependency** ⚠️
   - Requires Rust toolchain to build wgpu-native
   - **Mitigation**: Can use pre-built binaries, or add Rust to CI

3. **Abstraction Layer** ⚠️
   - Can't access raw backend (e.g., can't call raw `glDrawElements`)
   - **Mitigation**: WebGPU API is expressive enough for most needs

4. **Still Evolving** ⚠️
   - WebGPU spec still being finalized (close to 1.0)
   - API may change slightly
   - **Mitigation**: Active development = bug fixes, better backends

### Comparison with Alternatives

#### vs. Manual Implementation (OpenGL/Vulkan/DirectX)

| Aspect | Manual | wgpu-native |
|--------|--------|-------------|
| **Learning Value** | ⭐⭐⭐⭐⭐ | ⭐⭐⭐⭐ |
| **Implementation Time** | 3-6 months | 2-4 weeks |
| **Backend Coverage** | 1 at a time | All 4+ simultaneously |
| **Maintenance** | High (bugs, drivers) | Low (community maintains) |
| **Modern API** | Depends | ✅ Modern by design |
| **Production Ready** | Depends on quality | ✅ Used in browsers |

**Decision**: wgpu-native provides 80% of learning value with 20% of implementation effort.

#### vs. bgfx

| Aspect | bgfx | wgpu-native |
|--------|------|-------------|
| **Maturity** | ⭐⭐⭐⭐⭐ | ⭐⭐⭐ |
| **Console Support** | ✅ (Nintendo, PlayStation, Xbox) | ❌ |
| **API Style** | C (with C++ wrapper) | C (with future C++ wrapper) |
| **Future-Proof** | Proprietary | W3C Standard |
| **DirectX Support** | DX9/11/12 | DX12 only |

**Decision**: bgfx is more battle-tested, but wgpu-native aligns with future web standard.

#### vs. IGL (Meta)

| Aspect | IGL | wgpu-native |
|--------|-----|-------------|
| **DirectX** | ❌ | ✅ |
| **Mobile Focus** | ⭐⭐⭐⭐⭐ | ⭐⭐⭐⭐ |
| **Battle-tested** | Billions of devices | Firefox, Deno |
| **API Design** | Modern | Modern (WebGPU spec) |

**Decision**: IGL lacks DirectX (deal-breaker for Windows gaming).

### Architecture Impact

#### Current Raktr Architecture

```
raktr/render/
├── public/
│   ├── device.h              // Keep abstract Device interface
│   ├── render_context.h      // Manages wgpu initialization
│   ├── buffer.h              // Maps to WGPUBuffer
│   └── backend/
│       └── wgpu/             // NEW: WebGPU backend
│           ├── wgpu_device.h
│           └── wgpu_types.h
└── src/
    └── backend/
        └── wgpu/
            ├── wgpu_device.cpp
            ├── wgpu_context.cpp
            └── wgpu_helpers.cpp
```

#### Integration Strategy

1. **Wrap wgpu-native** in our existing `Device` interface
2. **Keep FakeBackend** for unit tests (no GPU needed)
3. **Add WgpuBackend** for hardware rendering
4. **Existing tests** continue to work with FakeBackend
5. **New integration tests** use WgpuBackend

#### API Mapping

| Raktr Concept | WebGPU Concept |
|---------------|----------------|
| `Device` | `WGPUDevice` |
| `Buffer` | `WGPUBuffer` |
| `Shader` (future) | `WGPUShaderModule` |
| `ShaderProgram` | `WGPURenderPipeline` |
| `Texture` | `WGPUTexture` |
| `RenderContext` | `WGPUInstance` + `WGPUSurface` + `WGPUAdapter` |

### Implementation Plan

#### Phase 1: wgpu-native Integration

1. **Add wgpu-native dependency**
   - Option A: Build from source (requires Rust)
   - Option B: Use pre-built binaries
   - Add to CMake

2. **Create WgpuBackend**
   ```cpp
   class WgpuDevice : public Device {
       WGPUDevice _device;
       WGPUQueue _queue;
   public:
       std::expected<Buffer, std::error_code> 
       create_vertex_buffer(std::span<const std::byte> data) override;
       
       std::expected<void, std::error_code>
       draw_indexed(...) override;
   };
   ```

3. **Window Integration**
   - Use SDL3 or GLFW for window creation
   - Create `WGPUSurface` from native window handle
   - WebGPU handles swapchain creation

4. **Test Infrastructure**
   - Keep existing FakeBackend tests
   - Add WgpuBackend integration tests (requires GPU)
   - Use `BackendType::WebGPU` enum

#### Phase 2: Basic Rendering

1. Implement shader module creation (WGSL or SPIR-V)
2. Create render pipeline (vertex/fragment shaders)
3. Upload vertex/index buffers
4. Execute render pass (draw triangle)

#### Phase 3: Advanced Features

1. Texture support
2. Uniforms (bind groups)
3. Depth/stencil
4. Compute shaders (future)

### Dependencies

#### Required

- **wgpu-native** (via git submodule or Conan if available)
  - C headers: `webgpu.h`, `wgpu.h`
  - Binary: `libwgpu_native.a` or `wgpu_native.dll`

- **Window Library** (choose one):
  - SDL3 (recommended): `sdl/3.1.6` via Conan
  - GLFW: `glfw/3.4` via Conan

#### Optional

- **WGSL Shader Compiler**: Built into wgpu-native
- **SPIR-V Tools**: For shader reflection (future)

### Shader Language

WebGPU supports:

1. **WGSL** (WebGPU Shading Language) - Recommended
   - Native to WebGPU
   - Rust-like syntax
   - Cross-compiles to backend shaders

2. **SPIR-V** (Binary)
   - Compile GLSL → SPIR-V → WebGPU
   - Use `glslc` or `shaderc`

**Decision**: Start with **WGSL** (simpler), add SPIR-V support later if needed.

### Testing Strategy

#### Unit Tests (FakeBackend)
- ✅ Keep existing 44 tests
- ✅ No changes needed
- ✅ Run in CI without GPU

#### Integration Tests (WgpuBackend)
- Create headless device for automated tests (if possible)
- Render to texture, compare pixels
- Or mark as manual tests (require GPU)

#### System Tests
- Render primitives (cube, sphere)
- Save framebuffer to PNG
- Visual regression testing

### Migration from FakeBackend

**No breaking changes** - both backends coexist:

```cpp
// Existing tests continue to work
RenderConfig config;
config.backend = BackendType::Fake;  // Software rendering
auto context = create_render_context();
context->initialize(config);

// New GPU tests
RenderConfig gpu_config;
gpu_config.backend = BackendType::WebGPU;  // Hardware rendering
auto gpu_context = create_render_context();
gpu_context->initialize(gpu_config);
```

### Risks & Mitigations

| Risk | Impact | Mitigation |
|------|--------|------------|
| Rust build complexity | Medium | Use pre-built wgpu-native binaries |
| WebGPU spec changes | Low | Active community, rapid updates |
| Learning curve | Medium | WebGPU has excellent docs/tutorials |
| Platform compatibility | Low | wgpu-native handles platform details |
| Performance | Low | WebGPU designed for high performance |

### Success Criteria

**Phase 1 Complete:**
- ✅ wgpu-native integrated and building
- ✅ Window created with GPU surface
- ✅ Device and queue initialized
- ✅ Can clear screen to a color

**Phase 2 Complete:**
- ✅ Triangle renders on screen
- ✅ Vertex/index buffers working
- ✅ Basic shader pipeline functional

**Phase 3 Complete:**
- ✅ OBJ models render correctly
- ✅ Wireframe mode works
- ✅ Depth testing functional
- ✅ Ready for advanced rendering features

### Resources

- **wgpu-native**: https://github.com/gfx-rs/wgpu-native
- **WebGPU Spec**: https://www.w3.org/TR/webgpu/
- **WGSL Spec**: https://www.w3.org/TR/WGSL/
- **Learn WebGPU**: https://eliemichel.github.io/LearnWebGPU/
- **wgpu Examples**: https://github.com/gfx-rs/wgpu/tree/master/examples

### Conclusion

**Decision: Use wgpu-native (WebGPU) as the rendering backend.**

**Rationale:**
1. ✅ **All backends covered**: OpenGL, Vulkan, DirectX 12, Metal, WebGPU
2. ✅ **Future-proof**: W3C standard, will be in all browsers
3. ✅ **Modern API**: Clean, Vulkan-inspired design
4. ✅ **Production-ready**: Used in Firefox, Deno
5. ✅ **Time-efficient**: Focus on engine features, not backend bugs
6. ✅ **Still educational**: Learn modern graphics API patterns
7. ✅ **Async-friendly**: Aligns with planned async rendering pipeline

**Trade-off accepted**: Slight abstraction layer (can't access raw backends), but API is expressive enough for all rendering needs.

**Next steps:**
1. Add wgpu-native as dependency (git submodule or pre-built)
2. Integrate with CMake build system
3. Implement WgpuDevice wrapper around existing Device interface
4. Create window with GPU surface
5. Render first triangle using WebGPU API

## 2025-01-XX — Multithreaded Octree for Spatial Partitioning

### Context

We need a high-performance spatial partitioning structure for the scene graph to enable efficient culling (frustum, occlusion) and spatial queries. The Octree will manage game objects and their transforms, supporting concurrent queries from multiple threads (e.g., rendering thread queries for visible objects while game thread updates positions).

Existing architecture:
- Camera class with view/projection matrices in 
aktr/engine/public/scene/camera.h
- Strongly-typed transform system with GLM in 
aktr/render/public/math/transform_types.h
- C++23 with modern synchronization primitives available
- TDD workflow: tests first, then implementation

### Requirements

1. **Thread Safety**: Multiple threads must be able to query the Octree concurrently while one thread updates it
2. **Performance**: Cache-friendly memory layout, minimal allocations, fast queries
3. **Dynamic**: Support insertion, removal, and updates of objects as they move
4. **Spatial Queries**: Point queries, ray intersection, frustum culling
5. **Integration**: Work with existing Camera and transform system
6. **Bounded**: Finite world space (not infinite, simpler implementation)

### Design Decisions

#### 1. Node Structure

`cpp
struct OctreeNode {
    // Spatial bounds
    glm::vec3 center;
    float half_size;  // Half of side length
    
    // Children (8 octants: -x-y-z, +x-y-z, -x+y-z, ..., +x+y+z)
    // Use std::array for cache locality
    std::array<std::unique_ptr<OctreeNode>, 8> children;
    
    // Objects in this node (stored at leaf or when subdivision threshold not met)
    std::vector<ObjectId> objects;
    
    // Metadata
    uint8_t depth;
    bool is_leaf;
};
`

**Rationale**:
- glm::vec3 for center aligns with existing math types
- half_size instead of min/max simplifies octant calculations
- std::array for children is cache-friendly and fixed size
- std::unique_ptr for child ownership avoids copying
- Store object IDs (not pointers) for stability during updates
- Depth tracking for recursion control

#### 2. Subdivision Strategy

**Threshold-based subdivision**:
- Split when objects.size() > MAX_OBJECTS_PER_NODE (e.g., 8)
- Don't split beyond MAX_DEPTH (e.g., 8 levels = 256^3 smallest cells)
- Objects at node boundaries remain in parent (avoid duplication)

**Rationale**:
- Simple heuristic, predictable behavior
- Max depth prevents degenerate cases (clustered objects)
- Parent storage for boundary objects avoids complex multi-node membership

#### 3. Threading Strategy

**Read-Write Lock Approach** (simpler than lock-free for MVP):

`cpp
class Octree {
    std::shared_mutex _mutex;
    std::unique_ptr<OctreeNode> _root;
    
    // Query operations take shared_lock (multiple concurrent readers)
    std::vector<ObjectId> query_frustum(const Frustum& f) const {
        std::shared_lock lock(_mutex);
        // ...
    }
    
    // Update operations take unique_lock (exclusive writer)
    void insert(ObjectId id, const glm::vec3& pos) {
        std::unique_lock lock(_mutex);
        // ...
    }
};
`

**Rationale**:
- std::shared_mutex (C++17) allows multiple readers or one writer
- Queries are read-only and can run concurrently (common case)
- Updates are exclusive but less frequent
- Simple to reason about, hard to get wrong
- **Future optimization**: Per-node locking for finer granularity if profiling shows contention

**Alternative considered**: Lock-free with epoch-based reclamation (hazard pointers)
- **Pros**: Better scalability for write-heavy workloads
- **Cons**: Much more complex, error-prone, overkill for typical game scene (queries >> updates)
- **Decision**: Start with read-write lock, profile, optimize if needed

#### 4. Object Representation

`cpp
struct OctreeObject {
    glm::vec3 position;     // World position
    float radius;            // Bounding sphere radius
    // Optional: AABB for tighter bounds
};

// Octree stores:
std::unordered_map<ObjectId, OctreeObject> _objects;
`

**Rationale**:
- Bounding sphere is simplest for point-in-octant tests
- Radius allows for objects larger than a single cell
- Separate storage allows updates without traversing tree
- Object positions are cached for fast re-insertion after movement

#### 5. Query Operations

**Frustum Culling**:
`cpp
struct Frustum {
    std::array<glm::vec4, 6> planes;  // Left, right, top, bottom, near, far
};

std::vector<ObjectId> query_frustum(const Frustum& frustum) const;
`
- Test AABB vs frustum at each node
- Skip entire subtrees if AABB is outside frustum
- Collect all objects in visible nodes

**Point Query**:
`cpp
std::optional<ObjectId> query_point(const glm::vec3& point, float radius) const;
`
- Find node containing point
- Return nearest object within radius

**Ray Query**:
`cpp
std::optional<RayHit> query_ray(const glm::vec3& origin, const glm::vec3& direction, float max_dist) const;
`
- Traverse nodes intersected by ray (DDA-like)
- Test objects in each node for intersection
- Return nearest hit

#### 6. Memory Management

**Pool Allocation** for nodes:
`cpp
class NodePool {
    std::vector<std::array<OctreeNode, 64>> _chunks;  // 64 nodes per chunk
    std::vector<OctreeNode*> _free_list;
    
    OctreeNode* allocate();
    void deallocate(OctreeNode* node);
};
`

**Rationale**:
- Reduces allocator pressure (bulk allocation)
- Better cache locality (nodes allocated together)
- Reuse nodes instead of constant new/delete
- **Trade-off**: More complex than std::unique_ptr, but significant perf gain

**Alternative**: Start with std::unique_ptr, add pool later if profiling shows allocation hotspot

#### 7. Integration with Existing Systems

**Camera Integration**:
`cpp
// Extract frustum from Camera
Frustum Camera::frustum() const {
    // Extract planes from view * projection matrix
    // See Gribb & Hartmann method
}

// Usage:
Camera camera(...);
Octree octree(...);
auto visible = octree.query_frustum(camera.frustum());
`

**Transform Integration**:
`cpp
// Update object position when transform changes
void update_object_transform(ObjectId id, const glm::vec3& new_pos) {
    octree.remove(id);
    octree.insert(id, new_pos);
}
`

**Rationale**:
- Camera already has view + projection matrices
- Extract frustum planes once per frame
- Transform updates are explicit (no automatic tracking yet)

### API Design

Public API in 
aktr/engine/public/scene/octree.h:

`cpp
namespace raktr::engine::scene {

/*!
 * @brief Thread-safe Octree for spatial partitioning and culling.
 * 
 * Supports concurrent queries from multiple threads while allowing
 * updates from a single writer thread. Objects are identified by
 * ObjectId (uint64_t) and stored with position + radius.
 * 
 * @example
 * Octree octree(glm::vec3(0), 1000.0f, 8);  // 1000 unit radius, 8 max depth
 * 
 * // Insert objects
 * octree.insert(ObjectId{1}, glm::vec3(10, 0, 0), 5.0f);
 * 
 * // Query from render thread (thread-safe)
 * Frustum frustum = camera.frustum();
 * auto visible = octree.query_frustum(frustum);
 * 
 * // Update from game thread (exclusive access)
 * octree.update(ObjectId{1}, glm::vec3(15, 0, 0));
 */
class Octree {
public:
    using ObjectId = uint64_t;
    
    /*!
     * @brief Construct empty Octree.
     * @param center World-space center of root node.
     * @param half_size Half-size of root node (world units).
     * @param max_depth Maximum subdivision depth (default 8).
     * @param max_objects_per_node Split threshold (default 8).
     */
    Octree(const glm::vec3& center, float half_size, 
           uint8_t max_depth = 8, size_t max_objects_per_node = 8);
    
    ~Octree();
    
    // Non-copyable, movable
    Octree(const Octree&) = delete;
    Octree& operator=(const Octree&) = delete;
    Octree(Octree&&) noexcept;
    Octree& operator=(Octree&&) noexcept;
    
    /*!
     * @brief Insert object into Octree.
     * @param id Unique object identifier.
     * @param position World-space position.
     * @param radius Bounding sphere radius.
     * @return true if inserted, false if already exists.
     */
    bool insert(ObjectId id, const glm::vec3& position, float radius = 0.0f);
    
    /*!
     * @brief Remove object from Octree.
     * @param id Object identifier.
     * @return true if removed, false if not found.
     */
    bool remove(ObjectId id);
    
    /*!
     * @brief Update object position (remove + reinsert).
     * @param id Object identifier.
     * @param new_position New world-space position.
     * @return true if updated, false if not found.
     */
    bool update(ObjectId id, const glm::vec3& new_position);
    
    /*!
     * @brief Query objects inside frustum (thread-safe).
     * @param frustum View frustum from camera.
     * @return Vector of visible object IDs.
     */
    std::vector<ObjectId> query_frustum(const Frustum& frustum) const;
    
    /*!
     * @brief Query objects near point (thread-safe).
     * @param point Query center.
     * @param radius Search radius.
     * @return Vector of object IDs within radius.
     */
    std::vector<ObjectId> query_sphere(const glm::vec3& point, float radius) const;
    
    /*!
     * @brief Query nearest object along ray (thread-safe).
     * @param origin Ray origin.
     * @param direction Ray direction (normalized).
     * @param max_distance Maximum ray distance.
     * @return ObjectId and hit distance, or std::nullopt if no hit.
     */
    std::optional<std::pair<ObjectId, float>> 
        query_ray(const glm::vec3& origin, const glm::vec3& direction, 
                  float max_distance = 1000.0f) const;
    
    /*!
     * @brief Clear all objects from Octree.
     */
    void clear();
    
    /*!
     * @brief Get total number of objects.
     */
    [[nodiscard]] size_t size() const;
    
    /*!
     * @brief Get statistics (nodes, depth, objects per node).
     */
    struct Stats {
        size_t total_nodes;
        size_t leaf_nodes;
        size_t total_objects;
        uint8_t max_depth_used;
        size_t max_objects_in_node;
    };
    [[nodiscard]] Stats stats() const;
};

/*!
 * @brief View frustum for culling queries.
 */
struct Frustum {
    std::array<glm::vec4, 6> planes;  // Left, right, top, bottom, near, far
    
    /*!
     * @brief Extract frustum from view-projection matrix.
     * @param vp Combined view * projection matrix.
     * @return Frustum with normalized plane equations.
     */
    static Frustum from_matrix(const glm::mat4& vp);
    
    /*!
     * @brief Test if AABB intersects frustum.
     * @param center AABB center.
     * @param half_size AABB half-extents.
     * @return true if AABB is fully or partially inside frustum.
     */
    [[nodiscard]] bool intersects_aabb(const glm::vec3& center, 
                                       float half_size) const;
};

} // namespace raktr::engine::scene
`

### Testing Strategy

Follow TDD with Triple-A (Arrange / Act / Assert):

1. **test_octree_construction.cpp**:
   - Construct Octree with various sizes
   - Verify root node bounds
   - Test empty Octree state

2. **test_octree_insertion.cpp**:
   - Insert single object
   - Insert multiple objects
   - Trigger subdivision when threshold exceeded
   - Insert object outside root bounds (should fail or expand)
   - Insert duplicate ID (should fail)

3. **test_octree_removal.cpp**:
   - Remove existing object
   - Remove non-existent object
   - Remove all objects from subdivided node (should merge?)

4. **test_octree_queries.cpp**:
   - Query frustum with no objects
   - Query frustum with objects inside/outside
   - Query sphere with objects at various distances
   - Query ray hitting object vs missing

5. **test_octree_threading.cpp**:
   - Concurrent queries from multiple threads
   - Concurrent query + update (should not deadlock)
   - Stress test with many objects and queries

6. **test_frustum.cpp**:
   - Extract frustum from Camera
   - Test AABB intersection (inside, outside, intersecting)
   - Test sphere intersection

### Implementation Plan

1. **Phase 1**: Basic structure (single-threaded)
   - Implement OctreeNode, Octree class
   - Implement insert/remove
   - Implement subdivision logic
   - Write tests for construction and insertion

2. **Phase 2**: Query operations
   - Implement Frustum extraction from matrix
   - Implement frustum culling query
   - Implement sphere and ray queries
   - Write tests for all query types

3. **Phase 3**: Thread safety
   - Add std::shared_mutex
   - Verify lock granularity
   - Write threading tests (use std::thread, not FakeDevice)

4. **Phase 4**: Optimization (if profiling shows need)
   - Pool allocation for nodes
   - Per-node locking (if contention detected)
   - SIMD for frustum plane tests (AVX2 / NEON)

### Open Questions

1. **Object size handling**: How to handle objects larger than octree cells?
   - **Answer**: Store in parent node if object spans multiple children
   - Test with objects of various radii

2. **Dynamic world bounds**: What if objects move outside initial bounds?
   - **Option A**: Fail insertion (simpler, requires world bounds known up-front)
   - **Option B**: Expand root node (complex, requires rebuilding tree)
   - **Decision**: Option A for MVP, document world bounds requirement

3. **Update frequency**: Should updates be batched?
   - **Answer**: Not initially. Profile first, batch if update contention is high

4. **Integration with ECS**: How does ObjectId map to entities?
   - **Answer**: ObjectId is opaque uint64_t. Caller manages mapping (e.g., ECS entity ID)
   - Octree doesn't know about entity components

### References

- "Octrees for Faster Isosurface Generation" (Wilhelms & Van Gelder, 1992)
- "Real-Time Collision Detection" (Christer Ericson, 2004) - Chapter 7
- "Game Engine Architecture" (Jason Gregory, 2018) - Chapter 14.5
- Frustum extraction: Gribb & Hartmann, "Fast Extraction of Viewing Frustum Planes from the WorldView-Projection Matrix" (2001)
- C++23 synchronization: std::shared_mutex, std::shared_lock, std::unique_lock

### Deliverables

- 
aktr/engine/public/scene/octree.h - Public API
- 
aktr/engine/src/scene/octree.cpp - Implementation
- 
aktr/engine/tests/scene/test_octree_*.cpp - Test suite
- Updated glossary.md with Octree terminology
- This research document

