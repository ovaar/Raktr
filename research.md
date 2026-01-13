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

The current `MockBackend` and `MockDevice` are minimal test doubles that validate inputs but don't implement actual rendering behavior. The task is to refactor them into `SoftBackend` and `SoftDevice` that implement real software rendering.

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

A `SoftDevice` should be a CPU-based software renderer that:

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

### SoftDevice Architecture

```cpp
class SoftDevice : public Device
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
2. Rename `MockBackend` → `SoftBackend`, `MockDevice` → `SoftDevice`
3. Implement buffer storage in `SoftDevice`
4. Implement framebuffer and clear operation
5. Implement simple triangle rasterization
6. Add pixel readback methods for testing
7. Update existing tests to use Fake and add pixel assertion tests
8. Update backend factory to use `BackendType::Fake`

### References

- **Test Double Patterns**: Martin Fowler's "Mocks Aren't Stubs"
- **Software Rasterization**: Scratchapixel 2.0 - Rasterization Algorithm
- **Triangle Rasterization**: Edge function method (barycentric coordinates)



## 2025-10-29 � Technical Debt Resolution: SoftDevice Improvements & OBJ Loader

### Context

After implementing the initial SoftBackend with basic triangle rasterization, three technical debt items remained:
1. SoftDevice used simple flat shading (no lighting/normals)
2. No depth buffer implementation
3. OBJ loader was minimal (positions only, no normals/UVs)

### Implementation: Enhanced SoftDevice

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

#### SoftDevice Depth Tests (3 tests)
- EnableDepthTest_InitializesDepthBuffer - State initialization
- ClearDepthBuffer_ResetsAllDepthValues - Buffer clearing
- CloserTriangleOccludesFartherTriangle - Depth ordering verification

#### SoftDevice Lighting Tests (2 tests)
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


## 2025-11-15 — Occlusion Culling Implementation Strategy

### Executive Summary

Research comprehensive occlusion culling techniques for Raktr's WebGPU-based architecture. **Recommendation: Implement Hierarchical Z-Buffer (Hi-Z) occlusion culling as MVP**, with Software Occlusion Culling as fallback.

### Main Occlusion Culling Techniques

#### A) Hardware Occlusion Queries (HOQ)

**How It Works:**
- Submit low-poly proxy geometry (bounding boxes) to GPU
- Query returns pixel count that passed depth test
- If count > 0, object is visible; render full detail

**WebGPU Support:** ✅ Via `WGPUQueryType_Occlusion`

**Frame Latency Issue:** ⚠️ Results available **next frame** (GPU→CPU roundtrip)
- Frame N: Submit queries
- Frame N+1: Results available, decide visibility
- Temporal lag can cause flickering with fast camera movement

**Best Use Cases:**
- ✅ Large, static occluders (buildings, terrain)
- ✅ VR/high-frame-rate scenarios
- ✅ Coarse culling (large groups of objects)
- ❌ Avoid: Per-object queries, fast-moving cameras, small occluders

#### B) Hierarchical Z-Buffer (Hi-Z) ⭐ **RECOMMENDED**

**How It Works:**
1. Build depth pyramid (mipmap chain) from depth buffer using compute shader
2. Each level is half-resolution of previous, stores maximum depth per tile
3. Test object AABBs against appropriate mip level
4. If object's min depth > Hi-Z depth, object is occluded

**WebGPU Implementation:**
- **Phase 1:** Depth pyramid generation via compute shader
- **Phase 2:** Visibility testing (GPU compute or CPU)
- No CPU-GPU sync required, no frame latency

**Performance:**
- Depth pyramid generation: ~0.5-1ms (1080p)
- Visibility testing: ~0.2-0.5ms (10,000 objects)
- Total overhead: 0.5-1.5ms

**Pros:**
- ✅ Best performance/complexity ratio
- ✅ No frame latency (results same frame)
- ✅ WebGPU native support (compute shaders)
- ✅ Scales with GPU power
- ✅ Handles 10,000+ objects efficiently

**Cons:**
- ❌ Requires compute shader support
- ❌ Initial implementation complexity (medium)

#### C) Software Occlusion Culling

**How It Works:**
- Rasterize simplified occluders on CPU (depth-only)
- Use low-resolution depth buffer (e.g., 512×512)
- Test object AABBs against CPU depth buffer
- Multi-threaded rasterization

**Intel's Library:** https://github.com/GameTechDev/OcclusionCulling
- Apache 2.0 license (compatible)
- SIMD optimized (SSE4.1, AVX2)
- ~1-2ms on modern CPUs (10,000 AABBs)

**Pros:**
- ✅ No GPU dependency
- ✅ Predictable performance
- ✅ Integrates with Octree (CPU-side)
- ✅ Great for low-end hardware

**Cons:**
- ❌ CPU overhead (1-2ms per frame)
- ❌ Lower precision than GPU depth
- ❌ Requires multi-threading

#### D) Conservative Occlusion (Bounding Volumes)

**How It Works:**
- Test object AABBs against previous frame's depth buffer
- Conservative: cull if AABB's nearest point is behind depth
- Simplest approach, minimal implementation

**Best For:**
- Static scenes with few dynamic objects
- Large occluders (buildings, mountains)
- Low object density (< 1,000 objects)

### Recommendation for Raktr

**MVP: Hierarchical Z-Buffer (Hi-Z) in `raktr::render`**

**Why:**
1. Best performance/complexity ratio (0.5-1.5ms total)
2. No frame latency (unlike HOQ)
3. WebGPU native (compute shaders well-supported)
4. Scales with GPU power
5. Fits Raktr's GPU-first architecture

**Fallback: Software Occlusion in `raktr::engine`**
- Use when compute shaders unavailable
- For CPU-bound scenarios
- Mobile/low-power GPUs

### Integration with Existing Pipeline

**Current:**
```
Camera → Frustum Culling (Octree) → Visible Objects → Render
```

**With Occlusion:**
```
Camera → Frustum Culling (Octree) → Potentially Visible Set
         ↓
    Occlusion Culling (Hi-Z) → Confirmed Visible Set
         ↓
    Instance Rendering
```

### Implementation Plan

#### Phase 1: MVP (Hi-Z Core) - 2-3 weeks

**Week 1: Depth Pyramid Generation**
- Create `DepthPyramid` class in `raktr/render/src/occlusion/`
- Compute shader: `depth_reduce.wgsl` (downsample depth buffer)
- Generate mip chain: each level = max depth of 2×2 block from previous

**Week 2: Visibility Testing**
- Create `VisibilityTest` class
- Compute shader: test AABBs against depth pyramid
- Project AABB to screen space, sample appropriate mip level
- Output: bitfield of visible objects

**Week 3: Integration with Octree**
- Integrate with existing frustum culling in `RenderSystem`
- Two-stage pipeline: Frustum (coarse) → Hi-Z (fine)
- Build instance buffer from confirmed visible objects

#### Phase 2: Advanced Optimizations - 2-4 weeks

1. **Two-Pass Rendering (Early-Z)**
   - Pass 1: Render occluders only (depth pre-pass)
   - Build Hi-Z from depth pre-pass
   - Pass 2: Test all objects, render visible

2. **GPU-Driven Rendering**
   - Keep visibility results on GPU
   - Use indirect draw commands
   - Eliminate CPU readback

3. **Temporal Coherence**
   - Track visibility across frames
   - Skip testing for objects visible in last N frames
   - Reduces overhead by 50-70%

4. **LOD Integration**
   - Occluded objects switch to lower LOD
   - Prevents pop-in when visible

#### Phase 3: Polish & Fallbacks - 1 week

- Software occlusion fallback
- Debug visualization modes
- Performance profiling

### Testing Strategy

**Correctness Tests:**
```cpp
TEST(HiZCulling, OccludedObjectBehindWall_IsCulled)
TEST(HiZCulling, PartiallyOccludedObject_IsVisible)
TEST(HiZCulling, ObjectInFrontOfOccluder_IsVisible)
```

**Performance Tests:**
```cpp
TEST(HiZCulling, Performance_10kObjects_UnderBudget) {
    // Should complete in < 1.5ms
}
```

**Visual Verification:**
- Freeze occlusion culling feature
- Render visible in green, culled in red
- Compare vs ground truth

### Performance Expectations

| Scene Type | Objects | Hi-Z Time | Culling Efficiency |
|------------|---------|-----------|-------------------|
| Small      | 1,000   | 0.3ms     | Minimal benefit   |
| Medium     | 10,000  | 0.8ms     | 60-80% culled     |
| Large      | 50,000  | 1.5ms     | 70-85% culled     |
| Massive    | 100,000+| 2.5ms     | 75-90% culled     |

**Culling Efficiency by Scene:**
- Dense urban: 60-80% culled
- Open terrain: 20-40% culled
- Indoor corridor: 80-95% culled

### Architecture Placement

**Option A: In `raktr::render` (RECOMMENDED)**
- Hi-Z tightly coupled to GPU depth buffer
- Compute shaders live in render backend
- Visibility results feed directly into instance buffer

```cpp
// raktr/render/src/occlusion/hi_z_culling.h
namespace raktr::render::occlusion {
    class HiZCulling {
        WGPUTexture _depth_pyramid;
        WGPUBuffer _visibility_buffer;
    public:
        void build_pyramid(WGPUTexture depth);
        void test_visibility(std::span<const AABB> aabbs, 
                           const glm::mat4& view_proj);
    };
}
```

**Option B: In `raktr::engine`**
- For Software Occlusion Culling
- Integrates with Octree on CPU side

### References

1. **"Hierarchical Z-buffer Visibility"** - Ned Greene et al., SIGGRAPH 1993
2. **"Masked Occlusion Culling"** - Intel, 2016
3. **Intel's Software Occlusion**: https://github.com/GameTechDev/OcclusionCulling
4. **WebGPU Occlusion Queries**: https://www.w3.org/TR/webgpu/#queries

### Summary

Implement **Hi-Z in `raktr::render::occlusion`**, targeting 1.0-1.5ms overhead for 10,000 objects. Two-stage pipeline: **Frustum (coarse) → Hi-Z (fine)** culling.

---

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
2. ✅ **Backend-agnostic** — Works with SoftDevice, OpenGL, Vulkan, DirectX
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
TEST(SoftDevice, RenderWireframe_ProducesLines) {
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
3. Extend SoftDevice to support line rasterization
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
   - `SoftBackend` and `SoftDevice` for software rendering tests
   - Depth buffer support in SoftDevice
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
- `SoftBackend` creates context without actual window

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
- SoftDevice uses hardcoded software "shaders"

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
- SoftDevice assumes hardcoded layout (position, normal, uv)

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
- SoftDevice has hardcoded depth testing
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
- SoftDevice has internal framebuffer (`_pixels`, `_depth_buffer`)
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
- SoftDevice stores resources in vectors

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

Our current `SoftBackend` tests will remain valuable:

1. Keep `SoftBackend` for **headless testing** (CI, unit tests)
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
| Testing requires GPU                  | Keep SoftBackend for CI, OpenGL for dev only   |
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
2. **Keep SoftBackend** for unit tests (no GPU needed)
3. **Add WgpuBackend** for hardware rendering
4. **Existing tests** continue to work with SoftBackend
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
   - Keep existing SoftBackend tests
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

#### Unit Tests (SoftBackend)
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

### Migration from SoftBackend

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

---

## 2025-11-07 — User Input Event Handling Architecture (Revised)

### Context

The next TODO item requires implementing user input capture and notification for keyboard and mouse events. This design follows a **callback-based architecture** with clear separation of concerns between `render` (raw capture) and `engine` (processing).

### Architectural Principles

1. **`render/window`** — Exposes raw GLFW input callbacks (minimal abstraction)
2. **`engine/input`** — Handles event processing, mapping, and game logic
3. **Callback pattern** — Consistent with existing `set_resize_callback` API
4. **No polling** — Events delivered immediately via callbacks
5. **GLFW key codes exposed** — Allow `engine` to map to its own enumerations
6. **Multithreading ready** — Engine can process events in parallel

### Current State Analysis

**What We Have:**
- ✅ GLFW window abstraction (`GLFWWindow` in `raktr/render/src/window/`)
- ✅ `Window::poll_events()` calls `glfwPollEvents()` (triggers GLFW callbacks)
- ✅ Resize callback system using `std::function<void(uint32_t, uint32_t)>`
- ✅ `Window` interface is in `raktr/render/public/window/window.h`

**What's Missing:**
- ❌ No keyboard event callbacks
- ❌ No mouse button event callbacks
- ❌ No mouse movement callbacks
- ❌ No mouse scroll callbacks
- ❌ No GLFW callback registration in Window API

### Architectural Decision: Callback-Based Split Architecture

**`render/window` Responsibilities:**
- Register GLFW callbacks
- Expose raw callback setters on `Window` interface
- Pass GLFW key codes/mouse buttons directly (no abstraction)
- Minimal overhead — just forward GLFW events

**`engine/input` Responsibilities** (future TODO):
- Subscribe to window callbacks
- Map GLFW codes → engine-specific enumerations
- Maintain input state (key down/up)
- Process events (potentially in parallel threads)
- Provide high-level API (action mapping, input contexts)

---

### Design: `render/window` — Raw Callback API

**Goal:** Expose GLFW input callbacks through the `Window` interface with minimal abstraction.

#### Callback Type Definitions

Match GLFW callback signatures exactly:

```cpp
// In raktr/render/public/window/window.h

// Key callback (GLFW: GLFWkeyfun)
// key: GLFW_KEY_* constant
// scancode: platform-specific scancode
// action: GLFW_PRESS, GLFW_RELEASE, GLFW_REPEAT
// mods: GLFW_MOD_* bitfield (Shift, Ctrl, Alt, etc.)
using KeyCallback = std::function<void(int key, int scancode, int action, int mods)>;

// Mouse button callback (GLFW: GLFWmousebuttonfun)
// button: GLFW_MOUSE_BUTTON_* constant
// action: GLFW_PRESS or GLFW_RELEASE
// mods: GLFW_MOD_* bitfield
using MouseButtonCallback = std::function<void(int button, int action, int mods)>;

// Cursor position callback (GLFW: GLFWcursorposfun)
// xpos, ypos: cursor position in screen coordinates
using CursorPosCallback = std::function<void(double xpos, double ypos)>;

// Scroll callback (GLFW: GLFWscrollfun)
// xoffset, yoffset: scroll offsets
using ScrollCallback = std::function<void(double xoffset, double yoffset)>;

// Resize callback (already exists)
using ResizeCallback = std::function<void(uint32_t width, uint32_t height)>;
```

#### `Window` Interface Extensions

Add callback setters to `Window` interface:

```cpp
// In raktr/render/public/window/window.h

class Window {
public:
    // ... existing methods ...

    // Input callbacks
    virtual void set_key_callback(KeyCallback callback) = 0;
    virtual void set_mouse_button_callback(MouseButtonCallback callback) = 0;
    virtual void set_cursor_pos_callback(CursorPosCallback callback) = 0;
    virtual void set_scroll_callback(ScrollCallback callback) = 0;

    // Resize callback (already exists)
    virtual void set_resize_callback(ResizeCallback callback) = 0;
};
```

**Design Notes:**
- Use `int` for GLFW constants (not enums) → keeps render layer agnostic
- Callbacks optional → game logic subscribes as needed
- Thread-safe → GLFW events fire on main thread, `engine` can dispatch to workers

---

### Implementation: `GLFWWindow` Callback Registration

**File:** `raktr/render/src/window/glfw_window.h`

```cpp
class GLFWWindow : public Window {
private:
    GLFWwindow* _window{nullptr};
    
    // Stored callbacks
    KeyCallback _key_callback;
    MouseButtonCallback _mouse_button_callback;
    CursorPosCallback _cursor_pos_callback;
    ScrollCallback _scroll_callback;
    ResizeCallback _resize_callback; // already exists

public:
    void set_key_callback(KeyCallback callback) override;
    void set_mouse_button_callback(MouseButtonCallback callback) override;
    void set_cursor_pos_callback(CursorPosCallback callback) override;
    void set_scroll_callback(ScrollCallback callback) override;
};
```

**File:** `raktr/render/src/window/glfw_window.cpp`

```cpp
#include "glfw_window.h"
#include <GLFW/glfw3.h>

// Static wrapper to forward GLFW callbacks to instance methods
namespace {

void glfw_key_callback_wrapper(GLFWwindow* window, int key, int scancode, int action, int mods) {
    auto* win = static_cast<GLFWWindow*>(glfwGetWindowUserPointer(window));
    if (win && win->_key_callback) {
        win->_key_callback(key, scancode, action, mods);
    }
}

void glfw_mouse_button_callback_wrapper(GLFWwindow* window, int button, int action, int mods) {
    auto* win = static_cast<GLFWWindow*>(glfwGetWindowUserPointer(window));
    if (win && win->_mouse_button_callback) {
        win->_mouse_button_callback(button, action, mods);
    }
}

void glfw_cursor_pos_callback_wrapper(GLFWwindow* window, double xpos, double ypos) {
    auto* win = static_cast<GLFWWindow*>(glfwGetWindowUserPointer(window));
    if (win && win->_cursor_pos_callback) {
        win->_cursor_pos_callback(xpos, ypos);
    }
}

void glfw_scroll_callback_wrapper(GLFWwindow* window, double xoffset, double yoffset) {
    auto* win = static_cast<GLFWWindow*>(glfwGetWindowUserPointer(window));
    if (win && win->_scroll_callback) {
        win->_scroll_callback(xoffset, yoffset);
    }
}

} // anonymous namespace

void GLFWWindow::set_key_callback(KeyCallback callback) {
    _key_callback = std::move(callback);
    glfwSetKeyCallback(_window, _key_callback ? glfw_key_callback_wrapper : nullptr);
}

void GLFWWindow::set_mouse_button_callback(MouseButtonCallback callback) {
    _mouse_button_callback = std::move(callback);
    glfwSetMouseButtonCallback(_window, _mouse_button_callback ? glfw_mouse_button_callback_wrapper : nullptr);
}

void GLFWWindow::set_cursor_pos_callback(CursorPosCallback callback) {
    _cursor_pos_callback = std::move(callback);
    glfwSetCursorPosCallback(_window, _cursor_pos_callback ? glfw_cursor_pos_callback_wrapper : nullptr);
}

void GLFWWindow::set_scroll_callback(ScrollCallback callback) {
    _scroll_callback = std::move(callback);
    glfwSetScrollCallback(_window, _scroll_callback ? glfw_scroll_callback_wrapper : nullptr);
}
```

**Key Implementation Points:**
- Store callbacks as member variables (allows unsubscribing via `nullptr`)
- Use `glfwGetWindowUserPointer` to retrieve `GLFWWindow*` instance
- Register GLFW callback only if user callback is valid
- Unregister (set to `nullptr`) if user callback is cleared

---

### Design: `engine/input` — Event Processing (Future TODO)

**Goal:** Process raw GLFW events and provide game-ready input abstractions.

#### Engine-Side Event Structures

**File:** `raktr/engine/public/input/key_event.h`

```cpp
namespace raktr::engine {

// Engine-specific key codes (mapped from GLFW)
enum class KeyCode : uint16_t {
    Unknown = 0,
    Space = 32,
    Apostrophe = 39,
    Comma = 44,
    // ... (map from GLFW_KEY_* constants)
    A = 65, B = 66, /* ... */ Z = 90,
    Escape = 256,
    Enter = 257,
    // ... complete mapping
};

enum class KeyAction : uint8_t {
    Release = 0,
    Press = 1,
    Repeat = 2
};

struct KeyModifiers {
    bool shift : 1;
    bool ctrl : 1;
    bool alt : 1;
    bool super : 1;
};

struct KeyEvent {
    KeyCode key;
    int scancode;
    KeyAction action;
    KeyModifiers mods;
};

} // namespace raktr::engine
```

**File:** `raktr/engine/public/input/mouse_event.h`

```cpp
namespace raktr::engine {

enum class MouseButton : uint8_t {
    Left = 0,
    Right = 1,
    Middle = 2,
    Button4 = 3,
    Button5 = 4,
    // ...
};

enum class MouseAction : uint8_t {
    Release = 0,
    Press = 1
};

struct MouseButtonEvent {
    MouseButton button;
    MouseAction action;
    KeyModifiers mods;
};

struct MouseMoveEvent {
    double x;
    double y;
};

struct MouseScrollEvent {
    double xoffset;
    double yoffset;
};

} // namespace raktr::engine
```

#### Input Event Variant

**File:** `raktr/engine/public/input/input_event.h`

```cpp
#include <variant>
#include "key_event.h"
#include "mouse_event.h"

namespace raktr::engine {

using InputEvent = std::variant<
    KeyEvent,
    MouseButtonEvent,
    MouseMoveEvent,
    MouseScrollEvent
>;

} // namespace raktr::engine
```

**Rationale:**
- `std::variant` → composition over inheritance ✅
- Type-safe event handling via `std::visit`
- No vtable overhead

#### Engine Input System (Conceptual)

**File:** `raktr/engine/public/input/input_system.h`

```cpp
namespace raktr::engine {

class InputSystem {
public:
    explicit InputSystem(raktr::render::Window& window);

    // Map GLFW codes → engine codes
    static KeyCode map_glfw_key(int glfw_key);
    static MouseButton map_glfw_mouse_button(int glfw_button);
    static KeyModifiers map_glfw_mods(int glfw_mods);

    // Process events (called per frame or in separate thread)
    void process_events();

    // Query input state
    bool is_key_pressed(KeyCode key) const;
    bool is_mouse_button_pressed(MouseButton button) const;
    std::pair<double, double> get_mouse_position() const;

private:
    // Callbacks registered with Window
    void on_key(int key, int scancode, int action, int mods);
    void on_mouse_button(int button, int action, int mods);
    void on_cursor_pos(double xpos, double ypos);
    void on_scroll(double xoffset, double yoffset);

    // Internal state
    std::unordered_map<KeyCode, bool> _key_states;
    std::unordered_map<MouseButton, bool> _mouse_button_states;
    double _mouse_x{0.0}, _mouse_y{0.0};

    // Event queue for deferred processing (thread-safe)
    std::mutex _event_queue_mutex;
    std::vector<InputEvent> _event_queue;
};

} // namespace raktr::engine
```

**Multithreading Considerations:**
- GLFW callbacks fire on **main thread** (window thread)
- `InputSystem` can queue events and process on **worker threads**
- Use `std::mutex` to protect event queue
- Decouple event capture (main thread) from event processing (worker threads)

---

### GLFW Key Code Reference

GLFW provides key codes as `int` constants (e.g., `GLFW_KEY_A = 65`). The `engine` layer will map these to `KeyCode` enum.

**Sample GLFW Key Codes:**
```cpp
#define GLFW_KEY_SPACE              32
#define GLFW_KEY_APOSTROPHE         39
#define GLFW_KEY_COMMA              44
#define GLFW_KEY_MINUS              45
#define GLFW_KEY_PERIOD             46
#define GLFW_KEY_SLASH              47
#define GLFW_KEY_0                  48
#define GLFW_KEY_1                  49
// ...
#define GLFW_KEY_A                  65
#define GLFW_KEY_B                  66
// ...
#define GLFW_KEY_ESCAPE             256
#define GLFW_KEY_ENTER              257
#define GLFW_KEY_TAB                258
#define GLFW_KEY_BACKSPACE          259
// ...
```

**Sample GLFW Mouse Button Codes:**
```cpp
#define GLFW_MOUSE_BUTTON_1         0
#define GLFW_MOUSE_BUTTON_2         1
#define GLFW_MOUSE_BUTTON_3         2
#define GLFW_MOUSE_BUTTON_LEFT      GLFW_MOUSE_BUTTON_1
#define GLFW_MOUSE_BUTTON_RIGHT     GLFW_MOUSE_BUTTON_2
#define GLFW_MOUSE_BUTTON_MIDDLE    GLFW_MOUSE_BUTTON_3
```

## 2025-11-18 — Object-Oriented Render Pass Architecture

### Context

After researching the nanite-webgpu reference implementation and OOP design patterns, we need to design a clean, maintainable architecture for render pass management that supports:

1. **Temporal Hi-Z occlusion culling** (current frame uses previous frame's depth pyramid)
2. **Multiple render passes** with explicit load/store operations
3. **Frame resource management** (double/triple buffering)
4. **Pass composition** via render graph
5. **Type safety** and **testability**

### Key Architectural Patterns

#### 1. **Pass Object Pattern (Command)**

Each rendering technique is encapsulated in a self-contained pass object:

```cpp
namespace raktr::render {

class IRenderPass {
public:
    virtual ~IRenderPass() = default;
    
    // Execute the pass with given context
    virtual void execute(PassContext& ctx) = 0;
    
    // Lifecycle hooks
    virtual void on_viewport_resize() = 0;
    virtual std::string_view name() const = 0;
};

} // namespace raktr::render
```

**Benefits:**
- Single Responsibility: Each pass does ONE thing
- Open/Closed: Add new passes without modifying existing code
- Testable: Mock passes for unit tests

#### 2. **PassContext Pattern (Data Carrier)**

Instead of scattered parameters, use a context object that carries frame state:

```cpp
namespace raktr::render {

struct PassContext {
    // Frame state
    uint32_t frame_index;
    
    // Command recording
    WGPUCommandEncoder command_encoder;
    
    // Render targets (current frame)
    WGPUTextureView color_target;
    WGPUTextureView depth_target;
    
    // Previous frame resources (for temporal techniques like Hi-Z)
    WGPUTextureView prev_frame_hi_z_pyramid;
    
    // Shared resources
    UniformBuffer& global_uniforms;
    Scene& scene;
    
    // GPU device access
    WgpuDevice& device;
    
    // Profiler (optional)
    GpuProfiler* profiler = nullptr;
};

} // namespace raktr::render
```

**Benefits:**
- Reduces parameter lists
- Easy to extend without breaking existing code
- Clear data ownership

#### 3. **RenderPassBuilder (Fluent API)**

For flexible render pass construction with method chaining:

```cpp
namespace raktr::render {

class RenderPassBuilder {
public:
    RenderPassBuilder& color_attachment(
        WGPUTextureView target,
        WGPULoadOp load_op = WGPULoadOp_Clear,
        std::array<float, 4> clear_color = {0.0f, 0.0f, 0.0f, 1.0f}
    );
    
    RenderPassBuilder& depth_attachment(
        WGPUTextureView target,
        WGPULoadOp load_op = WGPULoadOp_Clear,
        float clear_depth = 1.0f
    );
    
    RenderPassBuilder& label(std::string_view name);
    
    // Execute and return encoder
    WGPURenderPassEncoder begin(WGPUCommandEncoder encoder);
    
private:
    struct AttachmentDesc {
        WGPUTextureView view;
        WGPULoadOp load_op;
        std::array<float, 4> clear_color;
    };
    
    std::optional<AttachmentDesc> _color;
    std::optional<AttachmentDesc> _depth;
    std::string _label;
};

// Usage example:
auto render_pass = RenderPassBuilder()
    .color_attachment(hdr_target, WGPULoadOp_Load)  // Don't clear
    .depth_attachment(depth_target, WGPULoadOp_Load) // Don't clear
    .label("GeometryPass")
    .begin(cmd_encoder);

} // namespace raktr::render
```

**Benefits:**
- Type-safe configuration
- Self-documenting code
- Prevents invalid state

#### 4. **Concrete Pass Classes (Strategy Pattern)**

Each rendering technique is its own class implementing `IRenderPass`:

```cpp
namespace raktr::render {

// Occlusion culling pass
class HiZOcclusionPass : public IRenderPass {
public:
    explicit HiZOcclusionPass(WgpuDevice& device);
    
    void execute(PassContext& ctx) override;
    void on_viewport_resize() override;
    std::string_view name() const override { return "HiZOcclusionPass"; }
    
private:
    std::unique_ptr<HiZBuffer> _hi_z_buffer;
};

// Geometry rendering pass
class GeometryPass : public IRenderPass {
public:
    GeometryPass(WgpuDevice& device, WGPUTextureFormat color_format);
    
    void execute(PassContext& ctx) override;
    void on_viewport_resize() override;
    std::string_view name() const override { return "GeometryPass"; }
    
private:
    WGPURenderPipeline _pipeline;
    BindingsCache _bindings_cache;
};

// Hi-Z pyramid building pass
class HiZPyramidPass : public IRenderPass {
public:
    explicit HiZPyramidPass(WgpuDevice& device);
    
    void execute(PassContext& ctx) override;
    void on_viewport_resize() override;
    std::string_view name() const override { return "HiZPyramidPass"; }
    
private:
    std::unique_ptr<HiZBuffer> _hi_z_buffer;
};

} // namespace raktr::render
```

#### 5. **RenderGraph (Composite Pattern)**

Manages pass execution order and dependencies:

```cpp
namespace raktr::render {

class RenderGraph {
public:
    RenderGraph& add_pass(std::unique_ptr<IRenderPass> pass);
    
    void execute(PassContext& ctx);
    void on_viewport_resize();
    
private:
    std::vector<std::unique_ptr<IRenderPass>> _passes;
};

} // namespace raktr::render
```

#### 6. **FrameResources (Resource Manager)**

Manages per-frame resources with double/triple buffering for temporal techniques:

```cpp
namespace raktr::render {

class FrameResources {
public:
    struct Frame {
        WGPUTexture color_texture;
        WGPUTextureView color_view;
        WGPUTexture depth_texture;
        WGPUTextureView depth_view;
        WGPUTexture hi_z_pyramid;
        WGPUTextureView hi_z_pyramid_view;
    };
    
    explicit FrameResources(WgpuDevice& device, uint32_t num_frames = 2);
    
    // Get current frame resources
    Frame& current();
    
    // Get previous frame resources (for temporal occlusion)
    Frame& previous();
    
    // Advance to next frame
    void advance();
    
    void resize(uint32_t width, uint32_t height);
    
private:
    std::vector<Frame> _frames;
    uint32_t _current_index = 0;
};

} // namespace raktr::render
```

### Complete Integration

```cpp
namespace raktr::render {

class Renderer {
public:
    Renderer(WgpuDevice& device, uint32_t width, uint32_t height);
    
    void render(Scene& scene);
    void resize(uint32_t width, uint32_t height);
    
private:
    void setup_render_graph();
    
    WgpuDevice& _device;
    FrameResources _frame_resources;
    UniformBuffer _global_uniforms;
    RenderGraph _render_graph;
    uint32_t _frame_index = 0;
};

} // namespace raktr::render
```

### Design Principles Applied

| Principle | Application |
|-----------|-------------|
| **Single Responsibility** | Each pass does ONE thing |
| **Open/Closed** | Add new passes without modifying existing code |
| **Liskov Substitution** | All passes implement IRenderPass interface |
| **Interface Segregation** | PassContext provides only what passes need |
| **Dependency Inversion** | Renderer depends on IRenderPass abstraction |
| **Command Pattern** | Passes encapsulate operations |
| **Strategy Pattern** | Different rendering strategies as pass implementations |
| **Composite Pattern** | RenderGraph composes passes |
| **Builder Pattern** | RenderPassBuilder for flexible construction |

### Benefits

✅ **Testable** - Mock passes for unit tests  
✅ **Flexible** - Reorder/replace passes easily

---

## 2025-11-19 — PassContext Backend Isolation Research

### Problem Statement

**Concern**: PassContext currently contains backend-specific types (e.g., `WGPUCommandEncoder`, `WGPUTextureView` from WebGPU). This violates architectural principles:

1. **Abstraction leakage** - Backend details exposed in public API
2. **Engine coupling** - Engine code would depend on backend-specific types
3. **Multi-backend support** - Different backends need different context data (OpenGL has different primitives than WebGPU)

**Current PassContext** (problematic):
```cpp
struct PassContext {
    uint32_t frame_index = 0;
    
    // ❌ Backend-specific types exposed
    WGPUCommandEncoder command_encoder = nullptr;
    WGPUTextureView color_target = nullptr;
    WGPUTextureView depth_target = nullptr;
    WGPUTextureView prev_frame_hi_z_pyramid = nullptr;
    
    uint32_t viewport_width  = 0;
    uint32_t viewport_height = 0;
    
    // ❌ Backend-specific device type
    backend::wgpu::WgpuDevice* device = nullptr;
};
```

### Architectural Analysis

#### Current Architecture

The codebase already uses **type erasure** successfully in two places:

1. **Device** (public API) - Type-erases concrete backend devices (WgpuDevice, SoftDevice, etc.)
2. **Backend interface** (internal) - Hides implementation details via IBackend

**Key Insight**: RenderGraph and render passes are **internal implementation details**, NOT public API that the engine uses directly.

#### Layer Separation

```
┌─────────────────────────────────────────────────────┐
│         raktr::engine (PUBLIC API CONSUMER)         │
│                                                     │
│  Uses: Device, Buffer, RenderContext                │
│  Does NOT use: RenderGraph, PassContext, passes    │
└─────────────────────────────────────────────────────┘
                          │
                          │ uses
                          ▼
┌─────────────────────────────────────────────────────┐
│       raktr::render PUBLIC API (ABSTRACTION)        │
│                                                     │
│  • Device (type-erased)                             │
│  • Buffer (handle)                                  │
│  • RenderContext (factory)                          │
│  • RenderConfig                                     │
│                                                     │
│  ❌ NOT exposed: RenderGraph, PassContext, passes  │
└─────────────────────────────────────────────────────┘
                          │
                          │ implements
                          ▼
┌─────────────────────────────────────────────────────┐
│     raktr::render INTERNAL (IMPLEMENTATION)         │
│                                                     │
│  Backend-specific (src/):                           │
│  • WgpuDevice, SoftDevice                           │
│  • WgpuBackend, SoftBackend                         │
│  • RenderGraph + PassContext ← HERE                 │
│  • Render passes (HiZOcclusionPass, etc.)          │
│                                                     │
│  ✅ Backend-specific types OK here                 │
└─────────────────────────────────────────────────────┘
```

### Solution Options

#### Option 1: Keep PassContext Backend-Specific (RECOMMENDED)

**Rationale**: PassContext is INTERNAL to the render subsystem. It's never exposed to the engine.

**Architecture**:
```cpp
// raktr/render/src/pass_context.h (INTERNAL - not in public/)
namespace raktr::render {

// Forward declarations for backend-specific types
using WGPUCommandEncoder = struct WGPUCommandEncoderImpl*;
using WGPUTextureView    = struct WGPUTextureViewImpl*;

struct PassContext {
    // Frame state (backend-agnostic)
    uint32_t frame_index = 0;
    uint32_t viewport_width  = 0;
    uint32_t viewport_height = 0;
    
    // Backend-specific resources
    // ✅ This is OK because PassContext is internal implementation
    WGPUCommandEncoder command_encoder = nullptr;
    WGPUTextureView color_target = nullptr;
    WGPUTextureView depth_target = nullptr;
    WGPUTextureView prev_frame_hi_z_pyramid = nullptr;
    
    // Backend device access
    backend::wgpu::WgpuDevice* device = nullptr;
};

} // namespace raktr::render
```

**File placement**:
- ✅ `raktr/render/src/pass_context.h` - Internal implementation
- ❌ NOT in `raktr/render/public/` - Never exposed to engine

**Who uses PassContext**:
- `RenderGraph::execute()` - Creates PassContext, passes to render passes
- Render passes (`HiZOcclusionPass`, `GeometryPass`, etc.) - Receive PassContext in execute()
- Backend implementation - Populates PassContext with backend-specific data

**Who does NOT use PassContext**:
- Engine code - Never sees PassContext
- Public Device API - Abstracts away implementation details

**Benefits**:
- ✅ Simple - No additional abstraction needed
- ✅ Performant - Direct access to backend resources
- ✅ Type-safe - Compile-time backend coupling within render subsystem
- ✅ Maintainable - Backend-specific logic stays in backend code

**Trade-offs**:
- ⚠️ RenderGraph is WebGPU-specific (but that's OK - it's internal)
- ⚠️ Would need OpenGLPassContext for OpenGL backend (handled by separate RenderGraph implementation)

#### Option 2: Type-Erase PassContext (OVER-ENGINEERING)

**Rationale**: If PassContext were PUBLIC API (it's not), we'd need type erasure.

**Architecture**:
```cpp
// raktr/render/public/pass_context.h
namespace raktr::render {

struct PassContext {
    uint32_t frame_index = 0;
    uint32_t viewport_width  = 0;
    uint32_t viewport_height = 0;
    
    // Type-erased backend data
    std::any backend_data;
    
    // Type-erased device
    Device* device = nullptr;
};

} // namespace raktr::render

// Backend-specific context (internal)
namespace raktr::render::backend {

struct WgpuPassContext {
    WGPUCommandEncoder command_encoder;
    WGPUTextureView color_target;
    WGPUTextureView depth_target;
    WGPUTextureView prev_frame_hi_z_pyramid;
};

} // namespace raktr::render::backend
```

**Problems**:
- ❌ Complexity - Need casting and type checking
- ❌ Runtime overhead - `std::any` has performance cost
- ❌ Error-prone - Easy to cast to wrong type
- ❌ **Not needed** - PassContext is already internal!

#### Option 3: Per-Backend RenderGraph (FUTURE-PROOF)

**Rationale**: Each backend has its own RenderGraph + PassContext implementation.

**Architecture**:
```cpp
// raktr/render/src/backend/wgpu/wgpu_render_graph.h
namespace raktr::render::backend {

struct WgpuPassContext {
    uint32_t frame_index;
    WGPUCommandEncoder command_encoder;
    WGPUTextureView color_target;
    // ... WebGPU-specific data
};

class WgpuRenderGraph {
public:
    void execute(WgpuPassContext& ctx);
    // ... WebGPU-specific implementation
};

} // namespace raktr::render::backend

// raktr/render/src/backend/opengl/opengl_render_graph.h
namespace raktr::render::backend {

struct OpenGLPassContext {
    uint32_t frame_index;
    GLuint framebuffer;
    GLuint depth_texture;
    // ... OpenGL-specific data
};

class OpenGLRenderGraph {
public:
    void execute(OpenGLPassContext& ctx);
    // ... OpenGL-specific implementation
};

} // namespace raktr::render::backend
```

**Benefits**:
- ✅ Complete backend isolation
- ✅ Backend-specific optimizations possible
- ✅ No shared abstractions needed

**Trade-offs**:
- ⚠️ Code duplication across backends
- ⚠️ More complex to maintain multiple implementations
- ⚠️ Overkill for current needs (only WebGPU backend exists)

---

## 2025-11-19 — Implementation Plan: Per-Backend RenderGraph Architecture

### Decision Rationale

**Selected**: Option 3 - Per-Backend RenderGraph (FUTURE-PROOF)

**Why**: Preparing for multi-backend support (OpenGL, Vulkan, DirectX) requires backend-isolated RenderGraph implementations. This prevents large refactors when adding new backends.

### High-Level Strategy

Each backend will have:
1. **Backend-specific PassContext** - Contains backend-native types (WGPUCommandEncoder vs GLuint)
2. **Backend-specific RenderGraph** - Manages pass execution with backend context
3. **Backend-specific RenderPass implementations** - Each pass optimized for backend
4. **Shared RenderPass interface** - Common abstraction for pass behavior

### Directory Structure (Target)

```
raktr/render/
├── public/
│   ├── device.h                    # Type-erased device (unchanged)
│   ├── buffer.h                    # Buffer handle (unchanged)
│   ├── render_context.h            # Factory (unchanged)
│   └── render_pass.h               # ✨ NEW: Abstract pass interface (backend-agnostic)
│
└── src/
    ├── backend/
    │   ├── wgpu/
    │   │   ├── wgpu_device.h
    │   │   ├── wgpu_backend.h
    │   │   ├── wgpu_pass_context.h   # ✨ NEW: WebGPU-specific context
    │   │   ├── wgpu_render_graph.h   # ✨ NEW: WebGPU graph implementation
    │   │   ├── wgpu_render_graph.cpp
    │   │   └── passes/               # ✨ NEW: WebGPU-specific passes
    │   │       ├── wgpu_hi_z_occlusion_pass.h
    │   │       ├── wgpu_hi_z_occlusion_pass.cpp
    │   │       ├── wgpu_geometry_pass.h
    │   │       ├── wgpu_geometry_pass.cpp
    │   │       ├── wgpu_instanced_geometry_pass.h
    │   │       ├── wgpu_instanced_geometry_pass.cpp
    │   │       ├── wgpu_hi_z_pyramid_pass.h
    │   │       └── wgpu_hi_z_pyramid_pass.cpp
    │   │
    │   └── fake/
    │       ├── fake_device.h
    │       ├── fake_backend.h
    │       ├── fake_pass_context.h   # ✨ NEW: Fake backend context
    │       ├── fake_render_graph.h   # ✨ NEW: Fake graph (no-op or simple)
    │       └── fake_render_graph.cpp
    │
    ├── render_graph.h                # ❌ REMOVE: No longer shared
    ├── render_graph.cpp              # ❌ REMOVE
    ├── pass_context.h                # ❌ REMOVE: No longer shared
    └── passes/                       # ❌ REMOVE: Move to backend-specific
        ├── hi_z_occlusion_pass.h     # → backend/wgpu/passes/
        ├── hi_z_occlusion_pass.cpp
        ├── geometry_pass.h
        ├── geometry_pass.cpp
        ├── instanced_geometry_pass.h
        ├── instanced_geometry_pass.cpp
        ├── hi_z_pyramid_pass.h
        └── hi_z_pyramid_pass.cpp
```

### Step-by-Step Implementation Plan

---

#### **Phase 1: Create Backend-Agnostic Abstraction Layer**

**Goal**: Define common interfaces that all backends will implement.

##### Step 1.1: Create Abstract RenderPass Interface (Public API)

**File**: `raktr/render/public/render_pass.h`

**Action**: Move existing `IRenderPass` to public API and make it truly backend-agnostic.

**Code**:
```cpp
namespace raktr::render {

/*!
 * @brief Backend-agnostic render pass interface.
 * 
 * Concrete implementations are backend-specific and live in backend/*/passes/.
 */
class IRenderPass {
public:
    virtual ~IRenderPass() = default;
    
    /*!
     * @brief Get pass name for debugging/profiling.
     */
    [[nodiscard]] virtual std::string_view name() const = 0;
    
    /*!
     * @brief Handle viewport resize event.
     */
    virtual void on_viewport_resize(uint32_t width, uint32_t height) = 0;
};

} // namespace raktr::render
```

**Note**: No `execute()` method - backend-specific contexts make this impossible at this level.

##### Step 1.2: Create Backend-Specific PassContext Type Trait

**File**: `raktr/render/src/backend/backend_traits.h` (NEW)

**Action**: Define compile-time traits for backend types.

**Code**:
```cpp
namespace raktr::render::backend {

// Forward declarations
namespace wgpu { struct WgpuPassContext; class WgpuRenderGraph; }
namespace fake { struct SoftPassContext; class SoftRenderGraph; }

/*!
 * @brief Compile-time mapping of backend types.
 * 
 * Each backend specializes this trait to define its PassContext and RenderGraph types.
 */
template<typename BackendDevice>
struct BackendTraits;

// Specialization for WgpuDevice
template<>
struct BackendTraits<wgpu::WgpuDevice> {
    using PassContext = wgpu::WgpuPassContext;
    using RenderGraph = wgpu::WgpuRenderGraph;
};

// Specialization for SoftDevice
template<>
struct BackendTraits<soft::SoftDevice> {
    using PassContext = soft::SoftPassContext;
    using RenderGraph = soft::SoftRenderGraph;
};

} // namespace raktr::render::backend
```

**Benefits**:
- ✅ Compile-time type safety
- ✅ No runtime overhead
- ✅ Easy to add new backends (just add specialization)

---

#### **Phase 2: Implement WebGPU Backend-Specific Components**

**Goal**: Create WebGPU-specific PassContext, RenderGraph, and move existing passes.

##### Step 2.1: Create WgpuPassContext

**File**: `raktr/render/src/backend/wgpu/wgpu_pass_context.h` (NEW)

**Action**: Move current PassContext into WebGPU backend.

**Code**:
```cpp
namespace raktr::render::backend::wgpu {

// Forward declarations
class WgpuDevice;

/*!
 * @brief WebGPU-specific pass execution context.
 * 
 * Contains WebGPU command encoder, texture views, and frame state.
 */
struct WgpuPassContext {
    // Frame state
    uint32_t frame_index = 0;
    uint32_t viewport_width = 0;
    uint32_t viewport_height = 0;
    
    // WebGPU command recording
    WGPUCommandEncoder command_encoder = nullptr;
    
    // WebGPU render targets
    WGPUTextureView color_target = nullptr;
    WGPUTextureView depth_target = nullptr;
    
    // Temporal resources (for Hi-Z occlusion)
    WGPUTextureView prev_frame_hi_z_pyramid = nullptr;
    
    // Device access
    WgpuDevice* device = nullptr;
    
    // Renderer configuration
    const RendererConfig* config = nullptr;
};

} // namespace raktr::render::backend::wgpu
```

##### Step 2.2: Create WgpuRenderGraph

**File**: `raktr/render/src/backend/wgpu/wgpu_render_graph.h` (NEW)

**Action**: Create WebGPU-specific render graph.

**Code**:
```cpp
namespace raktr::render::backend::wgpu {

// Forward declarations
class WgpuRenderPass;

/*!
 * @brief WebGPU-specific render graph.
 * 
 * Manages execution order of WebGPU render passes.
 */
class WgpuRenderGraph {
public:
    explicit WgpuRenderGraph(WgpuDevice* device);
    
    /*!
     * @brief Add a WebGPU render pass to the graph.
     */
    WgpuRenderGraph& add_pass(std::unique_ptr<WgpuRenderPass> pass);
    
    /*!
     * @brief Execute all passes with WebGPU context.
     */
    void execute(WgpuPassContext& ctx);
    
    /*!
     * @brief Notify passes of viewport resize.
     */
    void on_viewport_resize(uint32_t width, uint32_t height);
    
    /*!
     * @brief Get number of passes.
     */
    [[nodiscard]] size_t pass_count() const { return _passes.size(); }
    
    /*!
     * @brief Clear all passes.
     */
    void clear();
    
    /*!
     * @brief Get injected device.
     */
    [[nodiscard]] WgpuDevice* device() const { return _device; }
    
private:
    WgpuDevice* _device{nullptr};
    std::vector<std::unique_ptr<WgpuRenderPass>> _passes;
};

} // namespace raktr::render::backend::wgpu
```

**File**: `raktr/render/src/backend/wgpu/wgpu_render_graph.cpp` (NEW)

**Implementation**:
```cpp
namespace raktr::render::backend::wgpu {

WgpuRenderGraph::WgpuRenderGraph(WgpuDevice* device)
    : _device(device)
{
}

WgpuRenderGraph& WgpuRenderGraph::add_pass(std::unique_ptr<WgpuRenderPass> pass) {
    _passes.push_back(std::move(pass));
    return *this;
}

void WgpuRenderGraph::execute(WgpuPassContext& ctx) {
    for (auto& pass : _passes) {
        pass->execute(ctx);
    }
}

void WgpuRenderGraph::on_viewport_resize(uint32_t width, uint32_t height) {
    for (auto& pass : _passes) {
        pass->on_viewport_resize(width, height);
    }
}

void WgpuRenderGraph::clear() {
    _passes.clear();
}

} // namespace raktr::render::backend::wgpu
```

##### Step 2.3: Create WgpuRenderPass Base Class

**File**: `raktr/render/src/backend/wgpu/wgpu_render_pass.h` (NEW)

**Action**: Backend-specific pass interface.

**Code**:
```cpp
namespace raktr::render::backend::wgpu {

/*!
 * @brief WebGPU-specific render pass interface.
 * 
 * Extends IRenderPass with WebGPU execute() signature.
 */
class WgpuRenderPass : public IRenderPass {
public:
    virtual ~WgpuRenderPass() = default;
    
    /*!
     * @brief Execute pass with WebGPU context.
     */
    virtual void execute(WgpuPassContext& ctx) = 0;
};

} // namespace raktr::render::backend::wgpu
```

##### Step 2.4: Move Existing Passes to WebGPU Backend

**Action**: Rename and move existing pass files.

**File Moves**:
```
src/passes/hi_z_occlusion_pass.h     → src/backend/wgpu/passes/wgpu_hi_z_occlusion_pass.h
src/passes/hi_z_occlusion_pass.cpp   → src/backend/wgpu/passes/wgpu_hi_z_occlusion_pass.cpp
src/passes/geometry_pass.h           → src/backend/wgpu/passes/wgpu_geometry_pass.h
src/passes/geometry_pass.cpp         → src/backend/wgpu/passes/wgpu_geometry_pass.cpp
src/passes/instanced_geometry_pass.h → src/backend/wgpu/passes/wgpu_instanced_geometry_pass.h
src/passes/instanced_geometry_pass.cpp → src/backend/wgpu/passes/wgpu_instanced_geometry_pass.cpp
src/passes/hi_z_pyramid_pass.h       → src/backend/wgpu/passes/wgpu_hi_z_pyramid_pass.h
src/passes/hi_z_pyramid_pass.cpp     → src/backend/wgpu/passes/wgpu_hi_z_pyramid_pass.cpp
```

**Code Changes** (Example: HiZOcclusionPass):

**Before** (`src/passes/hi_z_occlusion_pass.h`):
```cpp
namespace raktr::render {
    class HiZOcclusionPass final : public IRenderPass {
        void execute(PassContext& ctx) override;
    };
}
```

**After** (`src/backend/wgpu/passes/wgpu_hi_z_occlusion_pass.h`):
```cpp
namespace raktr::render::backend::wgpu {
    class WgpuHiZOcclusionPass final : public WgpuRenderPass {
        void execute(WgpuPassContext& ctx) override;
    };
}
```

**Update Namespaces**: Change `raktr::render` → `raktr::render::backend::wgpu` in all moved files.

---

#### **Phase 3: Implement Fake Backend Components**

**Goal**: Create minimal SoftBackend render graph for testing.

##### Step 3.1: Create SoftPassContext

**File**: `raktr/render/src/backend/soft/fake_pass_context.h` (NEW)

**Code**:
```cpp
namespace raktr::render::backend::fake {

class SoftDevice;

/*!
 * @brief Fake backend pass context (minimal for testing).
 */
struct SoftPassContext {
    uint32_t frame_index = 0;
    uint32_t viewport_width = 0;
    uint32_t viewport_height = 0;
    
    SoftDevice* device = nullptr;
    const RendererConfig* config = nullptr;
};

} // namespace raktr::render::backend::fake
```

##### Step 3.2: Create SoftRenderGraph

**File**: `raktr/render/src/backend/soft/fake_render_graph.h` (NEW)

**Code**:
```cpp
namespace raktr::render::backend::fake {

class SoftRenderPass;

/*!
 * @brief Fake render graph (no-op or minimal implementation).
 */
class SoftRenderGraph {
public:
    explicit SoftRenderGraph(SoftDevice* device);
    
    SoftRenderGraph& add_pass(std::unique_ptr<SoftRenderPass> pass);
    void execute(SoftPassContext& ctx);
    void on_viewport_resize(uint32_t width, uint32_t height);
    
    [[nodiscard]] size_t pass_count() const { return _passes.size(); }
    void clear();
    [[nodiscard]] SoftDevice* device() const { return _device; }
    
private:
    SoftDevice* _device{nullptr};
    std::vector<std::unique_ptr<SoftRenderPass>> _passes;
};

} // namespace raktr::render::backend::fake
```

**File**: `raktr/render/src/backend/soft/fake_render_graph.cpp` (NEW)

**Implementation**: Similar to WgpuRenderGraph but for SoftBackend.

##### Step 3.3: Create SoftRenderPass Base Class

**File**: `raktr/render/src/backend/soft/fake_render_pass.h` (NEW)

**Code**:
```cpp
namespace raktr::render::backend::fake {

class SoftRenderPass : public IRenderPass {
public:
    virtual ~SoftRenderPass() = default;
    virtual void execute(SoftPassContext& ctx) = 0;
};

} // namespace raktr::render::backend::fake
```

---

#### **Phase 4: Update Device Integration**

**Goal**: Connect backend-specific RenderGraphs to Device implementations.

##### Step 4.1: Add RenderGraph to WgpuDevice

**File**: `raktr/render/src/backend/wgpu/wgpu_device.h`

**Changes**:
```cpp
#include "wgpu_render_graph.h"

class WgpuDevice {
public:
    // ... existing methods ...
    
    /*!
     * @brief Get WebGPU render graph for this device.
     */
    [[nodiscard]] WgpuRenderGraph& render_graph() { return _render_graph; }
    [[nodiscard]] const WgpuRenderGraph& render_graph() const { return _render_graph; }
    
private:
    WgpuRenderGraph _render_graph{this};  // Initialized with device pointer
    // ... existing members ...
};
```

##### Step 4.2: Add RenderGraph to SoftDevice (Optional)

**File**: `raktr/render/src/backend/soft/fake_device.h`

**Changes**: Similar to WgpuDevice, add SoftRenderGraph member.

---

#### **Phase 5: Update Tests**

**Goal**: Migrate existing tests to use backend-specific components.

##### Step 5.1: Update RenderGraph Tests

**File**: `raktr/render/tests/test_render_graph.cpp`

**Changes**:
- Replace `#include "render_graph.h"` with `#include "backend/wgpu/wgpu_render_graph.h"`
- Replace `RenderGraph` with `wgpu::WgpuRenderGraph`
- Replace `PassContext` with `wgpu::WgpuPassContext`
- Update mock passes to use `WgpuRenderPass`

##### Step 5.2: Update Temporal Occlusion Integration Tests

**File**: `raktr/render/tests/test_temporal_occlusion_integration.cpp`

**Changes**: Similar namespace and type updates.

##### Step 5.3: Update Visual Demo

**File**: `raktr/editor/tests/test_visual_triangle.cpp`

**Changes**:
```cpp
// Before
#include "render_graph.h"
RenderGraph temporal_occlusion_graph(device);

// After
#include "backend/wgpu/wgpu_render_graph.h"
using namespace raktr::render::backend::wgpu;
WgpuRenderGraph temporal_occlusion_graph(device);
```

---

#### **Phase 6: Remove Shared Components**

**Goal**: Delete old shared RenderGraph/PassContext files.

##### Step 6.1: Remove Old Files

**Delete**:
- `raktr/render/src/render_graph.h`
- `raktr/render/src/render_graph.cpp`
- `raktr/render/public/pass_context.h`
- `raktr/render/src/passes/` (entire directory)

##### Step 6.2: Update CMakeLists.txt

**File**: `raktr/render/src/CMakeLists.txt`

**Changes**:
```cmake
# Remove old sources
# render_graph.cpp  # DELETE
# passes/*.cpp      # DELETE

# Add backend-specific sources
backend/wgpu/wgpu_render_graph.cpp
backend/wgpu/passes/wgpu_hi_z_occlusion_pass.cpp
backend/wgpu/passes/wgpu_geometry_pass.cpp
backend/wgpu/passes/wgpu_instanced_geometry_pass.cpp
backend/wgpu/passes/wgpu_hi_z_pyramid_pass.cpp

backend/soft/fake_render_graph.cpp
```

---

#### **Phase 7: Build and Test**

##### Step 7.1: Incremental Build Verification

1. Build after each phase to catch errors early
2. Fix compilation errors immediately
3. Run tests after each phase

##### Step 7.2: Full Test Suite

Run all tests to ensure no regressions:
```powershell
cmake --build build/Release --target render_tests
.\build\Release\render\tests\render_tests.exe
```

Expected: All 307+ tests passing.

---

## 2025-11-19 — Klaus Iglberger's Type Erasure Pattern for RenderPass

### Context

The current implementation plan (Phase 1) proposes creating an `IRenderPass` interface for backend-agnostic render passes:

```cpp
class IRenderPass {
public:
    virtual ~IRenderPass() = default;
    virtual std::string_view name() const = 0;
    virtual void on_viewport_resize(uint32_t width, uint32_t height) = 0;
    // Note: No execute() method - backend-specific PassContext makes this impossible
};
```

**Problem**: This is the traditional OOP approach with **inheritance-based polymorphism**, which has several drawbacks:

1. **Intrusive** - Render passes must inherit from `IRenderPass`
2. **Virtual dispatch overhead** - Every call goes through vtable
3. **Rigid interface** - All passes must implement the same methods
4. **Not idiomatic modern C++** - Violates "prefer composition over inheritance"

**Solution**: Use **Klaus Iglberger's Type Erasure Pattern** (also called **External Polymorphism** or **Concept-Model-Object** pattern).

### What is Type Erasure?

Type erasure is a technique that provides runtime polymorphism **without requiring inheritance**. It bridges static polymorphism (templates) with dynamic polymorphism (virtual functions) by:

1. Accepting any type that satisfies compile-time requirements (duck typing)
2. Wrapping it in a type-erased container
3. Providing a uniform interface without the wrapped type needing to know about it

**Key Insight**: The polymorphic behavior is achieved through an **internal implementation detail**, not through the public API.

### Klaus Iglberger's Pattern

The pattern uses three components:

1. **Concept** (internal) - Abstract interface with pure virtual functions
2. **Model<T>** (internal) - Template class that wraps concrete types
3. **Object** (public) - Type-erased wrapper that holds `unique_ptr<Concept>`

**Architecture**:

```
┌─────────────────────────────────────────────────┐
│           RenderPass (public API)               │
│  - Non-template, type-erased wrapper            │
│  - Holds unique_ptr<Concept>                    │
│  - Provides public interface: name(), execute() │
└─────────────────────────────────────────────────┘
                    │
                    │ holds
                    ▼
┌─────────────────────────────────────────────────┐
│         Concept (internal interface)            │
│  - Pure virtual interface                       │
│  - virtual name() = 0                          │
│  - virtual execute(...) = 0                    │
└─────────────────────────────────────────────────┘
                    ▲
                    │ implements
                    │
┌─────────────────────────────────────────────────┐
│       Model<ConcretePass> (internal)            │
│  - Template wrapping any pass type              │
│  - Forwards calls to wrapped pass               │
│  - ConcretePass doesn't know about Concept      │
└─────────────────────────────────────────────────┘
                    │
                    │ wraps
                    ▼
┌─────────────────────────────────────────────────┐
│     Concrete Pass (e.g., WgpuGeometryPass)      │
│  - NO INHERITANCE required                      │
│  - Just implements: name(), execute()           │
│  - Completely independent                       │
└─────────────────────────────────────────────────┘
```

### Implementation

#### Current Device Implementation (Reference)

The codebase **already uses this pattern** for `Device`:

```cpp
// raktr/render/public/device.h
class Device {
public:
    template <typename T>
    Device(T device_impl)
        : _impl(std::make_unique<Model<T>>(std::move(device_impl)))
    {}

    // Public interface
    bool supports<Capability>() const;
    Capability capability<Capability>() const;

private:
    // Internal Concept interface
    struct Concept {
        virtual ~Concept() = default;
        virtual bool supports(std::type_index) const = 0;
        // ... more virtuals
    };

    // Internal Model wrapper
    template<typename T>
    struct Model : Concept {
        Model(T impl) : _impl(std::move(impl)) {}
        
        bool supports(std::type_index idx) const override {
            return _impl.supports(idx); // Forward to concrete impl
        }
        
    private:
        T _impl; // Wrapped concrete device (WgpuDevice, SoftDevice, etc.)
    };

    std::unique_ptr<Concept> _impl; // Type-erased storage
};
```

**Key Points**:
- ✅ `WgpuDevice`, `SoftDevice` do NOT inherit from anything
- ✅ `Concept` and `Model` are **internal implementation details**
- ✅ Public API is clean: `Device device = WgpuDevice{...};`
- ✅ No virtual dispatch in concrete device code

#### Proposed RenderPass Implementation

Following the same pattern:

```cpp
// raktr/render/public/render_pass.h
#pragma once

#include <memory>
#include <string_view>

namespace raktr::render {

// Forward declarations (backend-specific contexts are internal)
namespace backend::wgpu { struct WgpuPassContext; }
namespace backend::soft { struct SoftPassContext; }

/*!
 * @brief Type-erased render pass using external polymorphism.
 *
 * This class can wrap any concrete pass type (WgpuGeometryPass, SoftGeometryPass, etc.)
 * without requiring them to inherit from a common base class.
 *
 * Concrete passes only need to implement:
 * - std::string_view name() const
 * - void execute(PassContext& ctx)
 * - void on_viewport_resize(uint32_t width, uint32_t height)
 *
 * @example
 * // Concrete pass (no inheritance!)
 * class WgpuGeometryPass {
 * public:
 *     std::string_view name() const { return "Geometry Pass"; }
 *     void execute(WgpuPassContext& ctx) { /* render */ }
 *     void on_viewport_resize(uint32_t w, uint32_t h) { /* resize */ }
 * };
 *
 * // Usage
 * RenderPass pass = WgpuGeometryPass{};
 * pass.name(); // "Geometry Pass"
 */
class RenderPass {
public:
    /*!
     * @brief Construct from any concrete pass type.
     * @tparam PassType Concrete pass type (e.g., WgpuGeometryPass).
     * @param pass Concrete pass instance.
     *
     * Requirements:
     * - PassType must have: std::string_view name() const
     * - PassType must have: void execute(PassContextType& ctx)
     * - PassType must have: void on_viewport_resize(uint32_t, uint32_t)
     */
    template <typename PassType>
    RenderPass(PassType pass)
        : _impl(std::make_unique<Model<PassType>>(std::move(pass)))
    {}

    // Non-copyable (some passes may hold GPU resources)
    RenderPass(const RenderPass&) = delete;
    RenderPass& operator=(const RenderPass&) = delete;

    // Movable
    RenderPass(RenderPass&&) noexcept = default;
    RenderPass& operator=(RenderPass&&) noexcept = default;

    ~RenderPass() = default;

    /*!
     * @brief Get the human-readable pass name.
     */
    [[nodiscard]] std::string_view name() const {
        return _impl->do_name();
    }

    /*!
     * @brief Notify pass of viewport resize.
     */
    void on_viewport_resize(uint32_t width, uint32_t height) {
        _impl->do_on_viewport_resize(width, height);
    }

    /*!
     * @brief Execute pass with backend-specific context.
     * @tparam PassContextType Backend-specific context (WgpuPassContext, SoftPassContext).
     * @param ctx Backend pass context.
     *
     * Note: This is a template method because PassContext type varies per backend.
     */
    template <typename PassContextType>
    void execute(PassContextType& ctx) {
        _impl->do_execute(&ctx);
    }

private:
    /*!
     * @brief Internal polymorphic interface (Concept).
     *
     * This is the abstract base class that enables polymorphism.
     * It is an **implementation detail** not exposed to users.
     */
    struct Concept {
        virtual ~Concept() = default;
        virtual std::string_view do_name() const = 0;
        virtual void do_on_viewport_resize(uint32_t width, uint32_t height) = 0;
        virtual void do_execute(void* ctx) = 0; // Type-erased context
    };

    /*!
     * @brief Internal wrapper for concrete pass types (Model).
     *
     * This template class wraps any concrete pass type and forwards
     * calls to it. The concrete pass does NOT need to inherit from anything.
     */
    template <typename PassType>
    struct Model : Concept {
        explicit Model(PassType pass) : _pass(std::move(pass)) {}

        std::string_view do_name() const override {
            return _pass.name();
        }

        void do_on_viewport_resize(uint32_t width, uint32_t height) override {
            _pass.on_viewport_resize(width, height);
        }

        void do_execute(void* ctx) override {
            // Deduce context type from pass's execute() signature
            using ContextType = typename PassTraits<PassType>::ContextType;
            _pass.execute(*static_cast<ContextType*>(ctx));
        }

    private:
        PassType _pass; // Wrapped concrete pass
    };

    /*!
     * @brief Type trait to deduce PassContext type from pass's execute() method.
     */
    template <typename PassType>
    struct PassTraits {
        // Deduced by looking at execute(ContextType& ctx) signature
        // This uses SFINAE / concepts to extract context type
        using ContextType = typename PassType::ContextType;
    };

    std::unique_ptr<Concept> _impl; // Type-erased storage
};

} // namespace raktr::render
```

#### Concrete Pass Implementation (No Inheritance!)

With type erasure, concrete passes are **completely independent**:

```cpp
// raktr/render/src/backend/wgpu/passes/wgpu_geometry_pass.h
#pragma once

#include "backend/wgpu/wgpu_pass_context.h"
#include <string_view>

namespace raktr::render::backend::wgpu {

/*!
 * @brief WebGPU geometry rendering pass.
 *
 * Note: Does NOT inherit from any base class!
 * Just implements the required duck-typed interface.
 */
class WgpuGeometryPass {
public:
    using ContextType = WgpuPassContext; // Type trait for context deduction

    WgpuGeometryPass() = default;

    /*!
     * @brief Get pass name.
     */
    [[nodiscard]] std::string_view name() const {
        return "WebGPU Geometry Pass";
    }

    /*!
     * @brief Execute geometry rendering.
     * @param ctx WebGPU-specific pass context.
     */
    void execute(WgpuPassContext& ctx) {
        // Actual rendering logic
        WGPURenderPassEncoder pass = wgpuCommandEncoderBeginRenderPass(
            ctx.command_encoder, &render_pass_desc);
        
        // Draw calls...
        wgpuRenderPassEncoderEnd(pass);
    }

    /*!
     * @brief Handle viewport resize.
     */
    void on_viewport_resize(uint32_t width, uint32_t height) {
        _viewport_width = width;
        _viewport_height = height;
    }

private:
    uint32_t _viewport_width = 800;
    uint32_t _viewport_height = 600;
};

} // namespace raktr::render::backend::wgpu
```

**Benefits**:
- ✅ **No inheritance** - `WgpuGeometryPass` is a plain class
- ✅ **No virtual functions** - All methods are regular functions
- ✅ **Type-safe** - `execute()` takes concrete `WgpuPassContext&`
- ✅ **Non-intrusive** - Pass doesn't know about `RenderPass` wrapper
- ✅ **Zero runtime overhead** in concrete pass code

### Comparison: Traditional OOP vs Type Erasure

| Aspect | Traditional OOP (IRenderPass) | Type Erasure (Concept-Model-Object) |
|--------|-------------------------------|-------------------------------------|
| **Inheritance** | ✗ Required (`class Pass : public IRenderPass`) | ✅ Not needed |
| **Virtual dispatch** | ✗ In concrete pass code | ✅ Only in wrapper (internal) |
| **Intrusiveness** | ✗ Pass must know about base class | ✅ Pass is independent |
| **Type safety** | ✗ `execute(void* ctx)` requires cast | ✅ `execute(ConcreteContext& ctx)` |
| **Flexibility** | ✗ All passes must match interface | ✅ Duck typing - any type works |
| **Testability** | ✗ Need mocks inheriting from base | ✅ Any test double works |
| **Modernness** | ✗ Old-style OOP | ✅ Modern C++ idiom |

### Real-World Example: std::function

The C++ standard library uses type erasure extensively:

```cpp
// std::function is type-erased!
std::function<int(int, int)> op;

// Can hold lambda (no inheritance!)
op = [](int a, int b) { return a + b; };

// Can hold function pointer
int multiply(int a, int b) { return a * b; }
op = multiply;

// Can hold functor (no inheritance!)
struct Divider {
    int operator()(int a, int b) { return a / b; }
};
op = Divider{};

// All work through same interface
int result = op(10, 5);
```

**How it works**: Internally, `std::function` uses the **exact same** Concept-Model-Object pattern!

### Integration with RenderGraph

**Before (with IRenderPass interface)**:
```cpp
class RenderGraph {
public:
    void add_pass(std::unique_ptr<IRenderPass> pass) {
        _passes.push_back(std::move(pass));
    }

    void execute(PassContext& ctx) {
        for (auto& pass : _passes) {
            pass->execute(ctx); // ❌ Can't do this - PassContext is backend-specific!
        }
    }

private:
    std::vector<std::unique_ptr<IRenderPass>> _passes;
};
```

**After (with Type Erasure)**:
```cpp
// raktr/render/src/backend/wgpu/wgpu_render_graph.h
class WgpuRenderGraph {
public:
    /*!
     * @brief Add render pass (accepts any type).
     * @tparam PassType Concrete pass type (WgpuGeometryPass, WgpuHiZOcclusionPass, etc.).
     */
    template <typename PassType>
    void add_pass(PassType pass) {
        _passes.emplace_back(std::move(pass));
    }

    /*!
     * @brief Execute all passes with WebGPU context.
     */
    void execute(WgpuPassContext& ctx) {
        for (auto& pass : _passes) {
            pass.execute(ctx); // ✅ Type-safe, concrete context
        }
    }

private:
    std::vector<RenderPass> _passes; // Type-erased storage
};
```

**Key Improvements**:
1. ✅ `add_pass()` is a **template** - accepts any pass type
2. ✅ `execute()` takes **concrete** `WgpuPassContext&` - type-safe!
3. ✅ No need for `std::unique_ptr` - `RenderPass` is movable
4. ✅ Clean, modern C++ API

### Benefits Summary

**Why Type Erasure is Better**:

1. **Non-Intrusive**
   - Passes don't need to know about any base class
   - Can use third-party pass implementations unchanged
   - Easier to test (no mock inheritance needed)

2. **Type Safety**
   - `execute(WgpuPassContext&)` instead of `execute(void*)`
   - Compile-time errors for missing methods
   - No runtime casts

3. **Performance**
   - Virtual dispatch only in **wrapper code** (internal)
   - Concrete pass code has **zero virtual overhead**
   - Compiler can inline pass methods

4. **Flexibility**
   - Passes can have different interfaces (duck typing)
   - Easy to add new pass types
   - No rigid base class contract

5. **Modern C++**
   - Follows "prefer composition over inheritance"
   - Same pattern as `std::function`, `std::any`
   - Aligns with Copilot guidelines (SOLID, DDD)

### Implementation Plan (Revised Phase 1)

**Phase 1: Replace IRenderPass with Type-Erased RenderPass**

1. **Create** `raktr/render/public/render_pass.h` (type-erased wrapper)
2. **Delete** old `IRenderPass` interface (not needed)
3. **Update** `WgpuGeometryPass` - remove inheritance, add `ContextType` typedef
4. **Update** `WgpuHiZOcclusionPass` - remove inheritance
5. **Update** `WgpuInstancedGeometryPass` - remove inheritance
6. **Update** `WgpuHiZPyramidPass` - remove inheritance
7. **Update** `WgpuRenderGraph` - use `std::vector<RenderPass>` instead of `std::vector<std::unique_ptr<IRenderPass>>`
8. **Build and test** - should compile cleanly

**Migration Example**:

**Before**:
```cpp
class WgpuGeometryPass : public IRenderPass {
public:
    std::string_view name() const override;
    void on_viewport_resize(uint32_t w, uint32_t h) override;
    // Can't have execute() - PassContext is backend-specific!
};
```

**After**:
```cpp
class WgpuGeometryPass {
public:
    using ContextType = WgpuPassContext;
    
    std::string_view name() const;
    void execute(WgpuPassContext& ctx);
    void on_viewport_resize(uint32_t w, uint32_t h);
};
```

**Result**: Clean, modern, type-safe, non-intrusive render pass architecture!

### References

1. **Klaus Iglberger's Talks**:
   - CppCon 2021: "Breaking Dependencies: Type Erasure"
   - CppCon 2022: "Back to Basics: Polymorphism"

2. **Articles**:
   - Rainer Grimm: "C++ Core Guidelines: Type Erasure with Templates"
   - https://www.modernescpp.com/index.php/c-core-guidelines-type-erasure-with-templates/

3. **Books**:
   - Klaus Iglberger: "C++ Software Design" (2022)
   - Chapter on Type Erasure and External Polymorphism

4. **Standard Library Examples**:
   - `std::function` - Type-erased callable
   - `std::any` - Type-erased storage
   - `std::shared_ptr` - Type-erased deleter

---

### Migration Checklist

**Phase 1: Abstraction Layer**
- [ ] Create `public/render_pass.h` with backend-agnostic IRenderPass
- [ ] Create `src/backend/backend_traits.h` with type traits
- [ ] Build and verify compilation

**Phase 2: WebGPU Backend**
- [ ] Create `backend/wgpu/wgpu_pass_context.h`
- [ ] Create `backend/wgpu/wgpu_render_graph.h/.cpp`
- [ ] Create `backend/wgpu/wgpu_render_pass.h`
- [ ] Move passes to `backend/wgpu/passes/`
- [ ] Rename pass classes (add `Wgpu` prefix)
- [ ] Update namespaces in all moved files
- [ ] Build and verify WebGPU backend

**Phase 3: Fake Backend**
- [ ] Create `backend/soft/fake_pass_context.h`
- [ ] Create `backend/soft/fake_render_graph.h/.cpp`
- [ ] Create `backend/soft/fake_render_pass.h`
- [ ] Build and verify Fake backend

**Phase 4: Device Integration**
- [ ] Add render_graph() to WgpuDevice
- [ ] Add render_graph() to SoftDevice (optional)
- [ ] Build and verify integration

**Phase 5: Update Tests**
- [ ] Update `test_render_graph.cpp`
- [ ] Update `test_temporal_occlusion_integration.cpp`
- [ ] Update `test_visual_triangle.cpp` (OcclusionCullingDemo)
- [ ] Build and run tests (should pass)

**Phase 6: Cleanup**
- [ ] Delete `src/render_graph.h/.cpp`
- [ ] Delete `public/pass_context.h`
- [ ] Delete `src/passes/` directory
- [ ] Update CMakeLists.txt
- [ ] Build and verify no references to old files

**Phase 7: Final Verification**
- [ ] Build entire project clean
- [ ] Run all render tests (307+ passing)
- [ ] Run visual demo (should work unchanged)
- [ ] Verify no compilation warnings
- [ ] Review code for consistency

---

### Future Backend Addition Guide

When adding a new backend (e.g., OpenGL), follow this pattern:

1. **Create backend directory**: `src/backend/opengl/`

2. **Define PassContext**:
```cpp
// backend/opengl/opengl_pass_context.h
struct OpenGLPassContext {
    uint32_t frame_index;
    GLuint framebuffer;
    GLuint depth_texture;
    OpenGLDevice* device;
    const RendererConfig* config;
};
```

3. **Define RenderGraph**:
```cpp
// backend/opengl/opengl_render_graph.h
class OpenGLRenderGraph {
    void execute(OpenGLPassContext& ctx);
};
```

4. **Define RenderPass**:
```cpp
// backend/opengl/opengl_render_pass.h
class OpenGLRenderPass : public IRenderPass {
    virtual void execute(OpenGLPassContext& ctx) = 0;
};
```

5. **Add BackendTraits specialization**:
```cpp
// backend/backend_traits.h
template<>
struct BackendTraits<opengl::OpenGLDevice> {
    using PassContext = opengl::OpenGLPassContext;
    using RenderGraph = opengl::OpenGLRenderGraph;
};
```

6. **Implement passes**: Create OpenGL-specific passes in `backend/opengl/passes/`

---

### Benefits of This Architecture

✅ **Complete Backend Isolation** - Each backend is self-contained
✅ **No Abstraction Overhead** - Direct use of backend types
✅ **Easy to Add Backends** - Just add new backend directory and traits
✅ **Compile-Time Safety** - Type traits enforce correct usage
✅ **Testable** - Each backend can be tested independently
✅ **Maintainable** - Clear separation, no shared state
✅ **Optimizable** - Backend-specific optimizations possible

---

### Estimated Effort

- **Phase 1**: 1-2 hours (abstraction layer)
- **Phase 2**: 3-4 hours (WebGPU backend migration)
- **Phase 3**: 1-2 hours (Fake backend)
- **Phase 4**: 1 hour (device integration)
- **Phase 5**: 2-3 hours (test updates)
- **Phase 6**: 1 hour (cleanup)
- **Phase 7**: 1-2 hours (verification)

**Total**: ~10-15 hours

---

### Risk Mitigation

| Risk | Mitigation |
|------|------------|
| Breaking existing tests | Build/test after each phase |
| Namespace confusion | Use consistent naming (Wgpu prefix) |
| Missing includes | Incremental compilation catches early |
| CMake issues | Update CMakeLists.txt incrementally |
| Merge conflicts | Work in feature branch, frequent commits |

---

### Success Criteria

**Phase Complete When**:
- ✅ All 307+ tests passing
- ✅ Visual demo works unchanged
- ✅ No compilation warnings
- ✅ Clean separation between backends
- ✅ Easy to add new backend (documented pattern)
- ✅ No references to old shared render_graph files

### Recommended Solution

**Use Option 1: Keep PassContext Backend-Specific (Internal)**

**Justification**:

1. **PassContext is NOT public API** - It lives in `raktr/render/src/`, never exposed to engine
2. **Current architecture already separates layers properly**:
   - Engine → Device (type-erased public API) ✅
   - Device → Backend (internal implementation with RenderGraph/PassContext) ✅
3. **No abstraction needed** - RenderGraph and passes are implementation details of the backend
4. **Performance** - Direct access to backend resources without indirection
5. **Simplicity** - Fewer abstractions = easier to understand and maintain

**File Structure**:
```
raktr/render/
├── public/              # Public API - Engine uses this
│   ├── device.h         # Type-erased device (✅ backend-agnostic)
│   ├── buffer.h         # Buffer handle (✅ backend-agnostic)
│   ├── render_context.h # Factory (✅ backend-agnostic)
│   └── render_error.h   # Error codes (✅ backend-agnostic)
│
└── src/                 # Internal implementation
    ├── backend/
    │   ├── wgpu/
    │   │   ├── wgpu_device.h          # WebGPU device impl
    │   │   ├── wgpu_backend.h         # WebGPU backend
    │   │   └── wgpu_pass_context.h    # ✅ Backend-specific OK
    │   └── fake/
    │       ├── fake_device.h
    │       └── fake_backend.h
    ├── render_graph.h    # Uses PassContext (internal)
    ├── render_graph.cpp
    ├── pass_context.h    # ✅ Backend-specific types OK (internal)
    └── passes/           # Render passes (internal)
        ├── hi_z_occlusion_pass.h
        ├── geometry_pass.h
        └── hi_z_pyramid_pass.h
```

**Key Points**:

1. **PassContext location**: `raktr/render/src/pass_context.h` (NOT in public/)
2. **Backend-specific types**: Allowed in src/, forbidden in public/
3. **Engine isolation**: Engine never includes src/ headers, only public/
4. **Future backends**: Each backend (OpenGL, Vulkan) implements its own PassContext
5. **No breaking changes**: Current architecture already correct

### Implementation Guidelines

**DO**:
- ✅ Keep PassContext in `src/` directory
- ✅ Use backend-specific types in PassContext (WGPUCommandEncoder, etc.)
- ✅ Have RenderGraph use PassContext internally
- ✅ Let passes receive PassContext in execute()
- ✅ Use Device (type-erased) in public API

**DON'T**:
- ❌ Expose PassContext in public/ headers
- ❌ Let engine code include pass_context.h
- ❌ Try to make PassContext backend-agnostic (not needed)
- ❌ Over-engineer with std::any or complex type erasure

### Current Status

**Already Correct**:
- ✅ PassContext is in `raktr/render/public/pass_context.h` with forward declarations
- ✅ Backend-specific types are forward-declared, not exposing full WebGPU headers
- ✅ Device is type-erased in public API
- ✅ Engine uses Device, not PassContext

**Minor Issue**:
- ⚠️ PassContext is in `public/` but should probably be in `src/` since it's internal
- ⚠️ Forward declarations hide the dependency, but ideally PassContext shouldn't be public

**Action Items**:
1. **If PassContext is used by engine**: Keep it in public/ with forward declarations ✅ (current)
2. **If PassContext is only used by render subsystem**: Move to src/ (better isolation)
3. **Document** that PassContext is backend-specific by design

### Conclusion

**PassContext containing backend-specific types is acceptable and correct** because:

1. It's part of the render subsystem's internal implementation
2. Engine code uses Device (type-erased), not PassContext
3. Separation of concerns is maintained through directory structure
4. Forward declarations prevent header pollution in public API
5. No additional abstraction is needed - current architecture is sound

The concern about backend-specific types in PassContext is valid for **public API**, but PassContext is **internal implementation**. The existing architecture already handles this correctly through the Device abstraction layer.

---

## 2025-11-19 — Real-Time Renderer Configuration from GUI/Editor

### Problem Statement

**Goal**: Enable GUI/Editor to toggle renderer features in real-time (e.g., occlusion culling on/off, wireframe mode, debug visualization) without exposing internal render subsystem implementation details.

**Requirements**:
1. **Layer isolation** - GUI/Editor should not depend on render subsystem internals
2. **Type-safe** - Use enums/flags, not magic strings
3. **Real-time** - Changes take effect immediately (same frame or next frame)
4. **Extensible** - Easy to add new toggleable features
5. **Thread-safe** - GUI runs on main thread, render may use worker threads

### Architectural Options

#### Option 1: Renderer Configuration Object (RECOMMENDED)

**Rationale**: Expose a configuration object through the Device API that GUI can modify.

**Architecture**:
```cpp
// raktr/render/public/renderer_config.h
namespace raktr::render {

/*!
 * @brief Runtime configuration for renderer features.
 * 
 * Thread-safe configuration object that can be modified by GUI/Editor
 * and consumed by render subsystem. Changes take effect on next frame.
 */
struct RendererConfig {
    // Culling options
    bool enable_frustum_culling = true;
    bool enable_occlusion_culling = true;
    
    // Debug visualization
    bool show_wireframe = false;
    bool show_bounding_boxes = false;
    bool show_occlusion_buffer = false;
    
    // Performance options
    bool enable_vsync = true;
    bool enable_multithreading = true;
    uint32_t target_fps = 60;
    
    // Quality settings
    enum class ShadowQuality { Off, Low, Medium, High, Ultra };
    ShadowQuality shadow_quality = ShadowQuality::High;
    
    enum class AntiAliasing { None, FXAA, TAA, MSAA_2x, MSAA_4x, MSAA_8x };
    AntiAliasing anti_aliasing = AntiAliasing::TAA;
    
    // Hi-Z occlusion settings
    uint32_t hi_z_mip_levels = 0;  // 0 = auto-calculate
    bool hi_z_conservative = true;
};

} // namespace raktr::render
```

**Device API Extension**:
```cpp
// raktr/render/public/device.h
namespace raktr::render {

class Device {
public:
    // ... existing methods ...
    
    /*!
     * @brief Get current renderer configuration.
     * @return Reference to configuration (thread-safe).
     */
    [[nodiscard]] const RendererConfig& config() const;
    
    /*!
     * @brief Update renderer configuration.
     * Changes take effect on next frame.
     * @param config New configuration.
     */
    void set_config(const RendererConfig& config);
    
    /*!
     * @brief Update a specific config field.
     * @param updater Callback to modify config.
     * @example
     * device->update_config([](RendererConfig& cfg) {
     *     cfg.enable_occlusion_culling = !cfg.enable_occlusion_culling;
     * });
     */
    void update_config(std::function<void(RendererConfig&)> updater);
    
private:
    // Thread-safe config storage (uses mutex or atomic operations)
    mutable std::mutex _config_mutex;
    RendererConfig _config;
};

} // namespace raktr::render
```

**GUI/Editor Usage**:
```cpp
// raktr/editor/src/renderer_panel.cpp
namespace raktr::editor {

class RendererPanel {
public:
    void on_gui(Device* device) {
        ImGui::Begin("Renderer Settings");
        
        // Get current config
        auto config = device->config();
        
        // Toggle occlusion culling
        if (ImGui::Checkbox("Occlusion Culling", &config.enable_occlusion_culling)) {
            device->set_config(config);
        }
        
        // Toggle wireframe
        if (ImGui::Checkbox("Wireframe", &config.show_wireframe)) {
            device->set_config(config);
        }
        
        // Shadow quality dropdown
        const char* shadow_items[] = { "Off", "Low", "Medium", "High", "Ultra" };
        int shadow_idx = static_cast<int>(config.shadow_quality);
        if (ImGui::Combo("Shadow Quality", &shadow_idx, shadow_items, 5)) {
            config.shadow_quality = static_cast<RendererConfig::ShadowQuality>(shadow_idx);
            device->set_config(config);
        }
        
        ImGui::End();
    }
};

} // namespace raktr::editor
```

**Render Subsystem Consumption**:
```cpp
// raktr/render/src/render_graph.cpp
namespace raktr::render {

void RenderGraph::execute(PassContext& ctx) {
    // Get current config from device
    const auto& config = _device->config();
    
    for (auto& pass : _passes) {
        // Skip occlusion pass if disabled
        if (pass->name() == "HiZOcclusionPass" && !config.enable_occlusion_culling) {
            continue;
        }
        
        pass->execute(ctx);
    }
}

} // namespace raktr::render

// raktr/render/src/passes/hi_z_occlusion_pass.cpp
void HiZOcclusionPass::execute(PassContext& ctx) {
    const auto& config = ctx.device->config();
    
    if (!config.enable_occlusion_culling) {
        // Mark all objects visible
        _visibility_results.assign(_aabbs.size(), true);
        return;
    }
    
    // Perform occlusion culling...
}
```

**Benefits**:
- ✅ **Type-safe** - Enums and booleans, no magic strings
- ✅ **Thread-safe** - Mutex protects config updates
- ✅ **Discoverable** - IDE autocomplete shows all options
- ✅ **Extensible** - Add new fields without breaking existing code
- ✅ **Layer separation** - GUI uses Device API, not internal types
- ✅ **Real-time** - Changes take effect on next frame
- ✅ **Testable** - Easy to unit test with different configs

**Thread Safety**:
```cpp
// Thread-safe implementation
const RendererConfig& Device::config() const {
    std::lock_guard lock(_config_mutex);
    return _config;
}

void Device::set_config(const RendererConfig& config) {
    std::lock_guard lock(_config_mutex);
    _config = config;
}

void Device::update_config(std::function<void(RendererConfig&)> updater) {
    std::lock_guard lock(_config_mutex);
    updater(_config);
}
```

#### Option 2: Command Pattern (Event-Driven)

**Rationale**: GUI sends commands to renderer, which processes them asynchronously.

**Architecture**:
```cpp
// raktr/render/public/renderer_command.h
namespace raktr::render {

enum class RendererCommandType {
    ToggleOcclusionCulling,
    ToggleWireframe,
    SetShadowQuality,
    SetAntiAliasing,
    // ... more commands
};

struct RendererCommand {
    RendererCommandType type;
    std::variant<bool, int, float, std::string> payload;
};

class Device {
public:
    void submit_command(RendererCommand cmd);
    
private:
    std::queue<RendererCommand> _command_queue;
    std::mutex _queue_mutex;
};

} // namespace raktr::render
```

**GUI Usage**:
```cpp
// Toggle occlusion culling
device->submit_command({
    .type = RendererCommandType::ToggleOcclusionCulling,
    .payload = true
});
```

**Render Subsystem**:
```cpp
void Device::process_commands() {
    std::lock_guard lock(_queue_mutex);
    
    while (!_command_queue.empty()) {
        auto cmd = _command_queue.front();
        _command_queue.pop();
        
        switch (cmd.type) {
            case RendererCommandType::ToggleOcclusionCulling:
                _config.enable_occlusion_culling = std::get<bool>(cmd.payload);
                break;
            // ... handle other commands
        }
    }
}
```

**Trade-offs**:
- ✅ Decoupled - Commands can be queued and batched
- ✅ Async-friendly - Commands processed on render thread
- ❌ Less type-safe - Variant payload can be error-prone
- ❌ Harder to query current state - Need separate getter API
- ❌ More boilerplate - Command creation/dispatch code

#### Option 3: Direct Pass Configuration (NOT RECOMMENDED)

**Rationale**: GUI directly configures render passes.

**Problem**: Violates layer separation - GUI would depend on internal render passes.

```cpp
// ❌ BAD - Exposes internal implementation
class Device {
public:
    HiZOcclusionPass* get_occlusion_pass();  // ❌ Leaks internals
};

// GUI code
auto occlusion_pass = device->get_occlusion_pass();
occlusion_pass->set_enabled(false);  // ❌ Direct coupling
```

**Why This is Bad**:
- ❌ Tight coupling between GUI and render internals
- ❌ GUI depends on specific pass implementations
- ❌ Hard to refactor render subsystem
- ❌ Violates dependency inversion principle

### Recommended Implementation Plan

**Phase 1: Basic Configuration**
1. Create `RendererConfig` struct in `raktr/render/public/`
2. Add config getters/setters to Device API
3. Implement thread-safe config storage in Device
4. Update render passes to read config from `ctx.device->config()`

**Phase 2: GUI Integration**
1. Create `RendererPanel` in editor
2. Use ImGui checkboxes/combos to modify config
3. Call `device->set_config()` on changes
4. Show real-time stats (FPS, culling efficiency)

**Phase 3: Advanced Features**
1. Add config presets (Low/Medium/High/Ultra)
2. Save/load config from JSON
3. Add config validation (e.g., MSAA not supported on some devices)
4. Add change listeners for config updates

### Example: Complete Integration

**1. Config Definition**:
```cpp
// raktr/render/public/renderer_config.h
struct RendererConfig {
    // Culling
    bool enable_frustum_culling = true;
    bool enable_occlusion_culling = true;
    
    // Debug
    bool show_wireframe = false;
    bool show_bounding_boxes = false;
};
```

**2. Device API**:
```cpp
// raktr/render/public/device.h
class Device {
public:
    const RendererConfig& config() const;
    void set_config(const RendererConfig& config);
};
```

**3. Backend Implementation**:
```cpp
// raktr/render/src/backend/wgpu/wgpu_device.cpp
const RendererConfig& WgpuDevice::config() const {
    std::lock_guard lock(_config_mutex);
    return _config;
}

void WgpuDevice::set_config(const RendererConfig& config) {
    std::lock_guard lock(_config_mutex);
    _config = config;
    
    // Optionally log changes
    spdlog::info("Renderer config updated: occlusion_culling={}", 
                 config.enable_occlusion_culling);
}
```

**4. Render Pass Usage**:
```cpp
// raktr/render/src/passes/hi_z_occlusion_pass.cpp
void HiZOcclusionPass::execute(PassContext& ctx) {
    const auto& config = ctx.device->config();
    
    if (!config.enable_occlusion_culling) {
        _visibility_results.assign(_aabbs.size(), true);
        return;
    }
    
    // Perform culling...
}
```

**5. GUI Panel**:
```cpp
// raktr/editor/src/panels/renderer_panel.cpp
void RendererPanel::render(Device* device) {
    ImGui::Begin("Renderer");
    
    auto config = device->config();
    bool changed = false;
    
    if (ImGui::Checkbox("Occlusion Culling", &config.enable_occlusion_culling)) {
        changed = true;
    }
    
    if (ImGui::Checkbox("Wireframe", &config.show_wireframe)) {
        changed = true;
    }
    
    if (changed) {
        device->set_config(config);
    }
    
    ImGui::End();
}
```

**6. Visual Demo Integration** (Already Working!):
```cpp
// raktr/editor/tests/test_visual_triangle.cpp (OcclusionCullingDemo)
bool occlusion_culling_enabled = true;

while (!window->should_close()) {
    auto input_state = input_system.process_events();
    
    // Toggle with 'O' key
    if (input_state.keys[KeyCode::O] && !last_o_pressed) {
        occlusion_culling_enabled = !occlusion_culling_enabled;
        
        // Option 1: Direct toggle (current implementation) ✅
        spdlog::info("Occlusion Culling: {}", 
                     occlusion_culling_enabled ? "ON" : "OFF");
        
        // Option 2: Use Device config (future enhancement)
        device->update_config([&](RendererConfig& cfg) {
            cfg.enable_occlusion_culling = occlusion_culling_enabled;
        });
    }
    
    // Use config in render logic
    if (occlusion_culling_enabled && hi_z_buffer) {
        // Perform culling...
    }
}
```

### Performance Considerations

**Config Access Frequency**:
```cpp
// ❌ BAD - Lock contention on every frame
void execute(PassContext& ctx) {
    auto config = ctx.device->config();  // Mutex lock
    if (config.enable_culling) {
        // ...
    }
}

// ✅ GOOD - Cache config at frame start
struct PassContext {
    RendererConfig config;  // Copy at frame start
};

void RenderGraph::execute(PassContext& ctx) {
    ctx.config = _device->config();  // Single lock
    
    for (auto& pass : _passes) {
        pass->execute(ctx);  // Passes use cached config
    }
}
```

**Atomic Config Updates** (Lock-Free Alternative):
```cpp
// For simple boolean flags, use atomics instead of mutex
struct RendererConfig {
    std::atomic<bool> enable_occlusion_culling{true};
    std::atomic<bool> show_wireframe{false};
};

// No mutex needed for reads/writes
bool culling_enabled = device->config().enable_occlusion_culling.load();
device->config().enable_occlusion_culling.store(false);
```

### Testing Strategy

**Unit Tests**:
```cpp
TEST(Device, ConfigUpdate_ImmediatelyVisible) {
    auto device = create_wgpu_device();
    
    RendererConfig config;
    config.enable_occlusion_culling = false;
    device->set_config(config);
    
    auto retrieved = device->config();
    EXPECT_FALSE(retrieved.enable_occlusion_culling);
}

TEST(Device, ConfigUpdate_ThreadSafe) {
    auto device = create_wgpu_device();
    
    // Spawn multiple threads updating config
    std::vector<std::thread> threads;
    for (int i = 0; i < 10; ++i) {
        threads.emplace_back([&]() {
            for (int j = 0; j < 1000; ++j) {
                device->update_config([](RendererConfig& cfg) {
                    cfg.enable_occlusion_culling = !cfg.enable_occlusion_culling;
                });
            }
        });
    }
    
    for (auto& t : threads) t.join();
    
    // Should not crash or corrupt data
}
```

**Integration Tests**:
```cpp
TEST(OcclusionCulling, ConfigToggle_AffectsVisibility) {
    auto device = create_wgpu_device();
    auto graph = RenderGraph(device);
    
    // Add occlusion pass
    std::vector<bool> visibility;
    graph.add_pass(std::make_unique<HiZOcclusionPass>(
        hi_z_buffer, aabbs, vp, visibility));
    
    // All visible when disabled
    device->update_config([](auto& cfg) { 
        cfg.enable_occlusion_culling = false; 
    });
    
    PassContext ctx;
    graph.execute(ctx);
    EXPECT_EQ(visibility, std::vector<bool>(aabbs.size(), true));
    
    // Culling when enabled
    device->update_config([](auto& cfg) { 
        cfg.enable_occlusion_culling = true; 
    });
    
    graph.execute(ctx);
    EXPECT_LT(std::count(visibility.begin(), visibility.end(), true),
              aabbs.size());
}
```

### Benefits Summary

| Aspect | Benefit |
|--------|---------|
| **Layer Separation** | GUI uses Device API, never touches render internals |
| **Type Safety** | Enums and structs, compiler-checked |
| **Thread Safety** | Mutex or atomics prevent race conditions |
| **Real-Time** | Changes visible on next frame |
| **Extensibility** | Add new config fields without breaking API |
| **Testability** | Easy to unit test with different configs |
| **Discoverability** | IDE autocomplete shows all options |
| **Performance** | Config cached per-frame, minimal overhead |

### Migration Path

**Current State** (Manual Toggle):
```cpp
// Local variable in demo
bool occlusion_culling_enabled = true;

if (input.key_pressed('O')) {
    occlusion_culling_enabled = !occlusion_culling_enabled;
}

if (occlusion_culling_enabled) {
    // Perform culling
}
```

**Future State** (Config-Based):
```cpp
// Device holds config
device->update_config([](auto& cfg) {
    cfg.enable_occlusion_culling = !cfg.enable_occlusion_culling;
});

// Render passes read config
void HiZOcclusionPass::execute(PassContext& ctx) {
    if (!ctx.config.enable_occlusion_culling) return;
    // Perform culling
}
```

**No Breaking Changes**:
- Current demos continue to work
- Config system is additive enhancement
- Can be adopted incrementally  
✅ **Maintainable** - Each pass is independent  
✅ **Debuggable** - Clear execution flow  
✅ **Reusable** - Passes work with any scene  
✅ **Type-safe** - Compile-time pass validation  
✅ **Profiler-friendly** - Per-pass timing  
✅ **Temporal-ready** - Frame resources built-in  

### Implementation Plan

#### Phase 1: Core Infrastructure (This Session)
1. Create `IRenderPass` interface
2. Create `PassContext` struct
3. Create `RenderPassBuilder` class
4. Create `FrameResources` class
5. Create `RenderGraph` class

#### Phase 2: Concrete Passes
1. Implement `HiZOcclusionPass`
2. Implement `GeometryPass`
3. Implement `HiZPyramidPass`

#### Phase 3: Integration
1. Update `Renderer` to use render graph
2. Wire up temporal occlusion (previous frame pyramid)
3. Update visual tests to use new architecture

### File Structure

```
raktr/render/
├── public/
│   └── raktr/render/
│       ├── pass_context.h
│       ├── render_pass.h
│       ├── render_pass_builder.h
│       ├── render_graph.h
│       └── frame_resources.h
└── src/
    ├── render_pass_builder.cpp
    ├── render_graph.cpp
    ├── frame_resources.cpp
    └── passes/
        ├── hi_z_occlusion_pass.h
        ├── hi_z_occlusion_pass.cpp
        ├── geometry_pass.h
        ├── geometry_pass.cpp
        ├── hi_z_pyramid_pass.h
        └── hi_z_pyramid_pass.cpp
```

### Next Steps

1. Implement core infrastructure classes
2. Add unit tests for `RenderPassBuilder` and `RenderGraph`
3. Migrate existing Hi-Z code to pass-based architecture
4. Update `OcclusionCullingDemo` test to use new system
5. Verify temporal occlusion works correctly (60-80% culling rate)

**Sample GLFW Action Codes:**
```cpp
#define GLFW_RELEASE                0
#define GLFW_PRESS                  1
#define GLFW_REPEAT                 2
```

**Sample GLFW Modifier Bitfield:**
```cpp
#define GLFW_MOD_SHIFT           0x0001
#define GLFW_MOD_CONTROL         0x0002
#define GLFW_MOD_ALT             0x0004
#define GLFW_MOD_SUPER           0x0008
```

---

### Implementation Plan

#### Phase 1: `render/window` — Raw Callback API ✅ (Next TODO)

**Files to modify:**
- `raktr/render/public/window/window.h` — Add callback setters
- `raktr/render/src/window/glfw_window.h` — Add callback storage
- `raktr/render/src/window/glfw_window.cpp` — Implement GLFW callback registration

**Testing:**
- Add unit tests verifying callbacks are invoked with correct GLFW values
- Mock GLFW window and trigger events manually
- Verify callback unsubscription (set to `nullptr`)

**Success Criteria:**
- `Window::set_key_callback(...)` triggers on GLFW key events
- `Window::set_mouse_button_callback(...)` triggers on GLFW mouse events
- `Window::set_cursor_pos_callback(...)` triggers on GLFW cursor movement
- `Window::set_scroll_callback(...)` triggers on GLFW scroll events
- All GLFW key codes, mouse buttons, actions, and mods passed unchanged

---

#### Phase 2: `engine/input` — Event Processing (Future TODO)

**Files to create:**
- `raktr/engine/public/input/key_event.h` — `KeyCode`, `KeyEvent`, `KeyAction`, `KeyModifiers`
- `raktr/engine/public/input/mouse_event.h` — `MouseButton`, `MouseButtonEvent`, `MouseMoveEvent`, `MouseScrollEvent`
- `raktr/engine/public/input/input_event.h` — `InputEvent` variant
- `raktr/engine/public/input/input_system.h` — `InputSystem` class
- `raktr/engine/src/input/input_system.cpp` — Implementation

**Implementation Details:**
1. Create `InputSystem` constructor that subscribes to `Window` callbacks
2. Implement GLFW → Engine code mapping functions
3. Store input state (key pressed/released, mouse position, etc.)
4. Provide query API (`is_key_pressed`, `get_mouse_position`)
5. Add thread-safe event queue for multithreaded processing

**Testing:**
- Unit tests for GLFW → Engine code mapping
- Integration tests simulating key presses and verifying state updates
- Multithreading tests ensuring thread-safe event queue access

**Success Criteria:**
- GLFW key codes correctly mapped to `KeyCode` enum
- GLFW mouse buttons correctly mapped to `MouseButton` enum
- Input state accurately reflects keyboard/mouse events
- Event queue supports concurrent access from multiple threads

---

#### Phase 3: High-Level Input Features (Future TODO)

**Features:**
- Action mapping (e.g., "Jump" → Space or Gamepad A)
- Input contexts (e.g., "Menu", "Gameplay", "Dialogue")
- Chord detection (e.g., Ctrl+S)
- Dead zones for analog input (future gamepad support)

**Files to create:**
- `raktr/engine/public/input/action_map.h`
- `raktr/engine/public/input/input_context.h`

**Success Criteria:**
- Define actions in configuration file (JSON/YAML)
- Bind multiple inputs to single action
- Switch input contexts dynamically
- Detect modifier key chords

---

### Thread Safety Considerations

**GLFW Threading Model:**
- GLFW is **not thread-safe** — all GLFW calls must be on **main thread**
- `glfwPollEvents()` triggers callbacks on **main thread**
- Window callbacks fire synchronously during `poll_events()`

**Multithreaded Engine Design:**

1. **Main Thread (Render/Window):**
   - Calls `window.poll_events()` (triggers GLFW callbacks)
   - GLFW callbacks invoke `InputSystem` callbacks
   - `InputSystem` **enqueues** events to thread-safe queue

2. **Worker Thread (Engine/Input Processing):**
   - `InputSystem::process_events()` dequeues events
   - Maps GLFW codes → Engine codes
   - Updates input state
   - Dispatches to game logic

**Thread-Safe Event Queue Implementation:**

```cpp
// In raktr/engine/src/input/input_system.cpp

void InputSystem::on_key(int key, int scancode, int action, int mods) {
    // Called on main thread (GLFW callback)
    KeyEvent event{
        .key = map_glfw_key(key),
        .scancode = scancode,
        .action = static_cast<KeyAction>(action),
        .mods = map_glfw_mods(mods)
    };

    std::lock_guard<std::mutex> lock(_event_queue_mutex);
    _event_queue.emplace_back(std::move(event));
}

void InputSystem::process_events() {
    // Called on worker thread (or main thread if single-threaded)
    std::vector<InputEvent> events;
    {
        std::lock_guard<std::mutex> lock(_event_queue_mutex);
        events = std::move(_event_queue);
        _event_queue.clear();
    }

    for (const auto& event : events) {
        std::visit([this](const auto& e) {
            handle_event(e);
        }, event);
    }
}
```

**Key Points:**
- Minimize time holding lock (only copy/move queue)
- Avoid blocking main thread in callbacks
- Process events in batch on worker thread

---

### Testing Strategy

#### Unit Tests (`raktr/render/tests/window/`)

**Test File:** `test_glfw_window_input_callbacks.cpp`

```cpp
#include "gtest/gtest.h"
#include "gmock/gmock.h"
#include "raktr/render/window/glfw_window.h"

using namespace raktr::render;

TEST(GLFWWindow_set_key_callback, invokes_callback_on_key_press) {
    // Arrange
    GLFWWindow window{800, 600, "Test Window"};
    int captured_key = -1;
    int captured_action = -1;

    window.set_key_callback([&](int key, int scancode, int action, int mods) {
        captured_key = key;
        captured_action = action;
    });

    // Act
    // Simulate GLFW key event (requires GLFW mock or integration test)
    // For unit test: trigger callback manually via GLFWWindow test interface
    // window.simulate_key_event(GLFW_KEY_A, 0, GLFW_PRESS, 0);

    // Assert
    // EXPECT_EQ(captured_key, GLFW_KEY_A);
    // EXPECT_EQ(captured_action, GLFW_PRESS);
}

TEST(GLFWWindow_set_key_callback, unsubscribes_when_set_to_nullptr) {
    GLFWWindow window{800, 600, "Test Window"};
    bool callback_invoked = false;

    window.set_key_callback([&](int, int, int, int) {
        callback_invoked = true;
    });

    window.set_key_callback(nullptr);

    // Simulate GLFW key event
    // window.simulate_key_event(GLFW_KEY_A, 0, GLFW_PRESS, 0);

    EXPECT_FALSE(callback_invoked);
}
```

**Testing Challenges:**
- GLFW requires OpenGL context → integration test preferred
- Unit tests need GLFW mock or test harness
- Alternatively: expose `trigger_key_callback(...)` for testing (conditional compile)

#### Integration Tests (`raktr/render/tests/integration/`)

**Test File:** `test_window_input_integration.cpp`

```cpp
TEST(WindowInput_integration, key_callback_receives_glfw_events) {
    // Arrange
    GLFWWindow window{800, 600, "Integration Test"};
    int received_key = -1;

    window.set_key_callback([&](int key, int, int, int) {
        received_key = key;
    });

    // Act
    // Programmatically send key event to GLFW window
    // (Requires GLFW test utilities or manual testing)

    // Assert
    // EXPECT_EQ(received_key, GLFW_KEY_SPACE);
}
```

---

### Success Criteria Summary

**`render/window` (Phase 1):**
- ✅ `Window` interface exposes `set_key_callback`, `set_mouse_button_callback`, `set_cursor_pos_callback`, `set_scroll_callback`
- ✅ `GLFWWindow` registers GLFW callbacks and forwards events unchanged
- ✅ Callbacks pass raw GLFW `int` codes (no abstraction)
- ✅ Callbacks can be unsubscribed via `nullptr`
- ✅ Unit tests verify callback invocation

**`engine/input` (Phase 2 — Future):**
- ✅ `KeyCode`, `MouseButton` enums defined in `engine/input`
- ✅ GLFW → Engine code mapping functions implemented
- ✅ `InputEvent` variant supports all event types
- ✅ `InputSystem` subscribes to `Window` callbacks
- ✅ Thread-safe event queue supports multithreaded processing
- ✅ Input state query API (`is_key_pressed`, etc.) works correctly

**Phase 3 (Future):**
- ✅ Action mapping system allows binding multiple inputs to actions
- ✅ Input contexts enable switching between different input schemes
- ✅ Chord detection supports modifier key combinations

---

### Next Steps

1. **Implement Phase 1** — Add callback API to `Window` and `GLFWWindow`
2. **Write tests** — Unit tests for callback registration and invocation
3. **Document API** — Add Doxygen comments to public callback setters
4. **Update TODO** — Mark "Research input event handling" as complete
5. **Begin Phase 2** — Create `engine/input` subsystem (next TODO)

---

### References

- **GLFW Input Guide:** https://www.glfw.org/docs/latest/input_guide.html
- **GLFW Callback Reference:** https://www.glfw.org/docs/latest/group__input.html
- **C++23 `std::variant`:** https://en.cppreference.com/w/cpp/utility/variant
- **Raktr Coding Standards:** `.github/copilot-instructions.md`

---

## 2025-11-07 — Camera Class with Frustum and Input Control

### Context

Need to implement a `Camera` class in `raktr::engine::scene` that:
1. Uses perspective projection (frustum-based)
2. Integrates with the existing `InputSystem` (WASD + mouse)
3. Works with the type-safe transform system (`View`, `Perspective` from `raktr::render::math`)
4. Follows TDD approach with unit tests first
5. Updates the visual test `DISABLED_SpinningCubeTypeSafe` to use camera controls

### Design Decisions

**Location:**  
Place in `raktr/engine/public/scene/camera.h` and `raktr/engine/src/scene/camera.cpp`

**API Design:**
```cpp
namespace raktr::engine::scene {
    class Camera {
    public:
        // Construction
        Camera(const glm::vec3& position, 
               float fov_degrees, 
               float aspect_ratio,
               float near_plane = 0.1f, 
               float far_plane = 1000.0f);
        
        // Input processing
        void process_input(const InputState& input, float delta_time);
        
        // Camera control
        void set_position(const glm::vec3& pos);
        void set_rotation(float yaw, float pitch); // Euler angles
        void set_movement_speed(float speed);
        void set_mouse_sensitivity(float sensitivity);
        
        // Getters
        [[nodiscard]] glm::vec3 position() const;
        [[nodiscard]] glm::vec3 forward() const;
        [[nodiscard]] glm::vec3 right() const;
        [[nodiscard]] glm::vec3 up() const;
        [[nodiscard]] float yaw() const;
        [[nodiscard]] float pitch() const;
        
        // Matrix generation
        [[nodiscard]] raktr::render::math::View view() const;
        [[nodiscard]] raktr::render::math::Perspective projection() const;
        
        // Projection updates
        void set_aspect_ratio(float aspect);
        void set_fov(float fov_degrees);
    };
}
```

**Camera Controls:**
- **WASD** — Forward/Left/Back/Right movement (relative to camera orientation)
  - W: Forward (+Z in view space = -forward in world space)
  - S: Backward
  - A: Strafe left
  - D: Strafe right
- **Mouse** — Look around (FPS-style)
  - Mouse X delta: Yaw (rotate around Y-axis)
  - Mouse Y delta: Pitch (rotate around X-axis, clamped to prevent gimbal lock)
- **Space/Shift** (optional future): Up/down movement

**Coordinate System:**
- Right-handed coordinate system (matches Raktr convention)
- Camera forward = -Z in view space
- Yaw = rotation around Y-axis (0° = looking -Z, 90° = looking +X)
- Pitch = rotation around X-axis (clamped to [-89°, 89°] to avoid gimbal lock)

**Input Integration:**
```cpp
// In game loop:
InputState input = input_system.process_events();
camera.process_input(input, delta_time);
View view = camera.view();
Perspective proj = camera.projection();
ModelViewProjection mvp = proj * view * model;
```

**Implementation Details:**

1. **State Management:**
   - Store position (glm::vec3)
   - Store yaw/pitch (float, in degrees)
   - Calculate forward/right/up vectors from yaw/pitch
   - Store FOV, aspect ratio, near/far planes

2. **Input Processing:**
   - Check InputState for WASD keys
   - Calculate movement vector in camera space
   - Transform to world space using right/forward vectors
   - Update position based on movement speed and delta time
   - Track mouse position delta for look rotation
   - Update yaw/pitch, clamp pitch to [-89, 89]

3. **View Matrix:**
   - Use `View::look_at(position, position + forward, up)`
   - Recompute when position or orientation changes

4. **Projection Matrix:**
   - Use `Perspective::from_fov_degrees(fov, aspect, near, far)`
   - Recreate when FOV or aspect ratio changes

**Test Strategy:**

Unit tests in `test_camera.cpp`:
1. **Construction** — Verify default values, initial position/orientation
2. **Movement** — Test WASD keys update position correctly
3. **Rotation** — Test mouse input updates yaw/pitch
4. **Matrix Generation** — Verify view/projection matrices match expected GLM results
5. **Clamping** — Verify pitch is clamped to [-89, 89]
6. **Edge Cases** — Zero delta time, no input, extreme values

Integration test:
- Update `DISABLED_SpinningCubeTypeSafe` to create Camera and process InputSystem events
- Verify camera can orbit/move around the cube

**Dependencies:**
- `raktr::engine::input::InputSystem` and `InputState`
- `raktr::render::math::View` and `Perspective`
- `glm::vec3`, `glm::mat4`
- `<cmath>` for sin/cos/radians

**Implementation Plan:**

1. Write test cases first (TDD)
2. Implement Camera class to pass tests
3. Add to CMakeLists.txt
4. Integrate with visual test
5. Manual verification of controls

**Potential Enhancements (Future):**
- Smooth camera movement (interpolation/damping)
- Multiple camera modes (orbit, fly, first-person)
- Zoom support
- Camera serialization
- Frustum culling helpers

---

### References

- **GLM lookAt:** https://glm.g-truc.net/0.9.9/api/a00668.html
- **GLM perspective:** https://glm.g-truc.net/0.9.9/api/a00243.html
- **Camera Tutorial:** https://learnopengl.com/Getting-started/Camera
- **Euler Angles:** https://en.wikipedia.org/wiki/Euler_angles

---

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
   - Write threading tests (use std::thread, not SoftDevice)

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



---

## 2025-11-14 � WebGPU Instance Rendering Implementation

### Overview

Implemented complete instance rendering support for the WebGPU (WGPU) backend, enabling efficient rendering of hundreds of objects with a single draw call. This was critical for the FrustumCullingDemo which previously could only render 1 cube due to surface acquisition limitations.

### Problem Statement

**Original Issue**: `wgpuSurfaceGetCurrentTexture()` can only be called once per frame, limiting rendering to a single object. Attempted workaround of creating multiple windows was impractical and violated the API contract.

**Solution**: Instance rendering allows drawing multiple copies of the same geometry with different per-instance data (transforms, colors) in a single draw call.

### Architecture & Implementation

#### Phase 1: API Layer (Public Interface)

**1. Buffer Type Extension** (`buffer.h`)
- Added `BufferType::Instance` enum value
- GPU buffer type for storing per-instance data

**2. Instance Data Structure** (`instance_data.h`)
- `glm::mat4 model_matrix` (64 bytes)
- `glm::vec4 color` (16 bytes)
- Total: 80 bytes per instance
- Helper method: `to_bytes()` for GPU upload

**3. Capability Interface** (`device_capabilities.h`)
- New `InstancingOps` capability struct
- Functions: `create_instance_buffer`, `update_instance_buffer`, `draw_indexed_instanced`
- Type-erased via `std::function` for device abstraction

**4. Device Integration** (`device.h`)
- Added `do_capability_instancingops()` virtual method
- Implemented capability detection in `Model<T>` wrapper
- Added convenience methods forwarding to capability interface

#### Phase 2: Backend Implementation (WebGPU)

**1. Instance Buffer Creation** (`wgpu_device.cpp` lines 706-738)
- Creates buffer with `WGPUBufferUsage_Vertex | WGPUBufferUsage_CopyDst`
- Writes data via `wgpuQueueWriteBuffer`
- Returns `Buffer` handle with `BufferType::Instance`

**2. WGSL Shader Updates** (`wgpu_device.cpp` lines 275-305)
- Added `InstanceInput` struct with 5 vertex attributes
  - `@location(1-4)`: mat4 decomposed into 4 � vec4
  - `@location(5)`: vec4 color
- Reconstructs matrix in shader: `mat4x4<f32>(instance.model_matrix_0, ...)`
- **Backward compatibility**: Alpha channel as mode selector
  - `alpha == 0.0`: Compute color from vertex position
  - `alpha > 0.0`: Use per-instance color

**3. Pipeline Configuration** (`wgpu_device.cpp` lines 430-480)
- Two vertex buffer layouts:
  - Slot 0: Geometry (stepMode=Vertex, stride=12 bytes)
  - Slot 1: Instances (stepMode=Instance, stride=80 bytes)
- 5 instance attributes (@location 1-5)

**4. Draw Call Implementation** (`wgpu_device.cpp` lines 761-941)
- Binds geometry buffer to slot 0
- Binds instance buffer to slot 1
- Calls `wgpuRenderPassEncoderDrawIndexed(pass, index_count, instance_count, 0, 0, 0)`

**5. Backward Compatibility** (`wgpu_device.cpp` lines 340-360)
- Default instance buffer with identity matrix and `(0,0,0,0)` color
- `draw_indexed()` automatically binds default instance buffer
- Move constructor/assignment updated to transfer default buffer

#### Phase 3: Testing

**1. SoftDevice Extension**
- `create_instance_buffer()` - stores instance data
- `update_instance_buffer()` - updates stored data
- `draw_indexed_instanced()` - simplified implementation (calls draw_indexed N times)

**2. Comprehensive Unit Tests** (`test_instance_rendering.cpp` - 19 tests)
- Buffer creation: valid data, multiple instances, empty data error
- Buffer updates: valid updates, invalid buffer, wrong buffer type
- Draw calls: valid buffers, invalid buffers, zero counts
- Capability interface: availability, create/update/draw
- InstanceData helpers: byte conversion, size, data preservation

**3. Visual Testing** (`test_visual_triangle.cpp`)
- Updated `FrustumCullingDemo` with 500-cube instance rendering
- Per-frame frustum culling via octree
- Dynamic instance data building for visible cubes
- Per-instance rotation and color gradient

**Results**:
- ? All 250 render tests passing (19 new + 231 existing)
- ? FrustumCullingDemo: 500 cubes with dynamic culling (26-61 visible)
- ? 60 FPS sustained performance
- ? Backward compatibility verified

### Key Technical Decisions

**1. Instance Data Layout**: 80 bytes = mat4 (64) + vec4 (16)
- Rationale: GPU-aligned, simple, extensible
- Alternative rejected: Compressed transforms (too complex)

**2. Capability Pattern**: Type-erased interface via `std::function`
- Rationale: Maintains Device abstraction, runtime capability detection
- Benefit: SoftDevice can support instancing for testing

**3. Backward Compatibility**: Default instance buffer + alpha-based mode
- Rationale: Single pipeline for both instanced and non-instanced
- Alternative rejected: Separate pipelines (complexity)

**4. Buffer Update Pattern**: Rebuild instance data per frame
- Rationale: Simple, correct, sufficient for 500 instances at 60 FPS
- Future: Persistent buffers, partial updates, double-buffering

### Performance Characteristics

**Before**: 1 cube max per frame, surface acquisition limitation
**After**: 500 cubes, 1 draw call, 26-61 visible via frustum culling, 60 FPS

**Bottleneck Analysis**:
- CPU: Octree query (~1ms), instance data build (~0.5ms)
- GPU: Minimal overhead, efficient per-instance transform processing
- Memory: 80 bytes � 500 = 40 KB per frame (negligible)

### Integration Points

- **Octree**: `query_frustum()` ? filter visible ? build instance data
- **Camera**: MVP matrix as uniform, per-instance model matrices
- **Future ECS**: Entity transforms ? InstanceData conversion

### Lessons Learned

1. WebGPU surface is single-use per frame - instance rendering is the solution
2. WGSL requires mat4 decomposition to 4 � vec4 in vertex input
3. Single pipeline with conditional logic cheaper than separate pipelines
4. SoftDevice simplified testing enabled comprehensive coverage

### Future Work

**Optimization**:
- Persistent instance buffers (avoid realloc)
- Partial buffer updates
- SIMD for instance data construction

**Features**:
- Additional per-instance data (UV offsets, animation state)
- Indirect drawing (GPU-driven culling)
- Multi-draw indirect (batch multiple meshes)

### Deliverables

? API Layer: buffer.h, instance_data.h, device_capabilities.h, device.h
? Backend: wgpu_device.h/cpp (create/update/draw methods, shader, pipeline)
? Testing: fake_device.h/cpp, test_instance_rendering.cpp (19 tests)
? Visual: test_visual_triangle.cpp (FrustumCullingDemo with 500 cubes)
? Validation: 250 tests passing, 60 FPS with frustum culling
? Documentation: This research document, inline comments

---

## 2025-01-18 — Temporal Hi-Z Occlusion Culling with OOP Render Pass Architecture

### Problem Statement

**Issue**: Original Hi-Z occlusion culling implementation had **0% culling rate** due to circular dependency:
- Built depth pyramid from ALL geometry (including occluded objects)
- Used same frame's pyramid for occlusion testing
- Result: Pyramid contains occluded objects' depth → they pass occlusion test → not culled

**Goal**: Implement temporal occlusion culling using **previous frame's depth pyramid** to break circular dependency and achieve 60-80% culling rate in test scenes.

### Research: nanite-webgpu Architecture Analysis

**Key Findings from nanite-webgpu Reference Implementation**:

1. **Temporal Occlusion Strategy**
   - Uses PREVIOUS frame's Hi-Z pyramid for current frame's culling
   - No two-pass required within single frame
   - First frame: all objects visible (no pyramid yet)
   - Subsequent frames: use N-1 pyramid for frame N

2. **Pass Object Pattern** (Command Pattern)
   - Each rendering technique encapsulated in `IRenderPass` interface
   - `execute(PassContext&)` method performs work
   - Enables composition, testing, profiling

3. **PassContext Data Carrier**
   - Struct carrying all per-frame state
   - Replaces scattered function parameters
   - Fields: command_encoder, color/depth targets, previous pyramid, viewport dims

4. **Explicit LoadOp Control**
   - Key feature: `WGPULoadOp_Clear` vs `WGPULoadOp_Load`
   - Enables multiple render passes to same target
   - Builder pattern for type-safe pass construction

5. **FrameResources for Temporal Techniques**
   - Double/triple buffering of GPU resources
   - `current()` / `previous()` accessors
   - `advance_frame()` moves ring buffer forward

### OOP Design Patterns Applied

**1. Command Pattern** (`IRenderPass` interface)
```cpp
class IRenderPass {
    virtual void execute(PassContext& ctx) = 0;
    virtual void on_viewport_resize() = 0;
    virtual std::string_view name() const = 0;
};
```
- Encapsulates rendering action
- Uniform interface for all passes
- Supports undo/redo, logging, profiling

**2. Builder Pattern** (`RenderPassBuilder`)
```cpp
builder.color_attachment(view, WGPULoadOp_Clear, {0.1f, 0.1f, 0.15f, 1.0f})
       .depth_attachment(view, WGPULoadOp_Clear, 1.0f)
       .label("MyPass")
       .begin(encoder);
```
- Fluent API for render pass construction
- Type-safe, readable, extensible
- Prevents invalid configurations

**3. Composite Pattern** (`RenderGraph`)
```cpp
RenderGraph graph;
graph.add_pass(std::make_unique<HiZOcclusionPass>(...))
     .add_pass(std::make_unique<GeometryPass>(...))
     .add_pass(std::make_unique<HiZPyramidPass>(...));
graph.execute(ctx);
```
- Manages pass execution order
- Treats single pass and graph uniformly
- Supports nesting, conditional passes

**4. Strategy Pattern** (Concrete Pass Implementations)
- Different culling strategies: frustum, Hi-Z, portal
- Different pyramid build strategies: compute, raster
- Runtime swap without changing client code

### Implementation

#### Phase 1: Core Infrastructure

**Created Files**:

1. **`render_pass.h`** (58 lines)
   - `IRenderPass` interface with `execute()`, `on_viewport_resize()`, `name()`
   - Pure virtual, virtual destructor
   - Base class for all render passes

2. **`pass_context.h`** (56 lines)
   - `PassContext` struct with frame state
   - Members: frame_index, command_encoder, color/depth targets, prev_frame_hi_z_pyramid, viewport dims, device
   - Forward declared WGPU types (avoids exposing webgpu.h in public API)
   - All members have default values

3. **`render_pass_builder.h/cpp`** (98 + 110 lines)
   - Fluent API: `color_attachment()`, `depth_attachment()`, `label()`, `begin()`
   - Returns `WGPURenderPassEncoder`
   - Uses `optional<>` for attachment descriptors
   - Handles `WGPUStringView` assignment correctly

4. **`render_graph.h/cpp`** (69 + 43 lines)
   - Composite pattern implementation
   - Methods: `add_pass()`, `execute()`, `on_viewport_resize()`, `clear()`
   - Stores `vector<unique_ptr<IRenderPass>>`
   - Iterates passes in order, hooks for profiler

5. **`frame_resources.h/cpp`** (169 + 139 lines)
   - Manages double/triple buffering
   - Methods: `current()`, `previous()`, `advance_frame()`, `recreate_resources()`, `wait_idle()`
   - Creates Hi-Z pyramid textures with mip levels
   - Ring buffer logic: `(current + size - 1) % size`

**Design Decisions**:
- Non-copyable, non-movable passes (due to reference members)
- Forward declarations minimize header dependencies
- `[[maybe_unused]]` for unused parameters (clean warnings)
- `std::string_view` return type for `name()` (no allocation)

#### Phase 2: Concrete Pass Implementations

**Created Files**:

1. **`hi_z_occlusion_pass.h/cpp`** (77 + 93 lines)
   - Performs GPU frustum + occlusion culling
   - Uses **previous frame's pyramid** via `ctx.prev_frame_hi_z_pyramid`
   - Outputs `vector<bool>& visibility_results` (filled by execute)
   - Logs culling statistics: visible count, culling rate, test time
   - Fallback: marks all visible if Hi-Z unavailable

2. **`geometry_pass.h/cpp`** (84 + 105 lines)
   - Renders visible geometry to color + depth
   - Uses visibility results from occlusion pass
   - Per-instance drawing: `wgpuRenderPassEncoderDrawIndexed(pass, indices, 1, 0, 0, i)`
   - Skips occluded instances (no GPU work wasted)
   - Uses `RenderPassBuilder` for pass construction
   - Logs draw statistics: drawn count, culling rate

3. **`hi_z_pyramid_pass.h/cpp`** (69 + 64 lines)
   - Builds Hi-Z pyramid for **next frame**
   - Uses compute shader via `hi_z_buffer->build_pyramid(depth_texture)`
   - Stores maximum depth at each mip level
   - Runs AFTER geometry pass completes
   - Logs pyramid statistics: dimensions, mip levels, build time

**Architecture Flow**:
```
Frame N-1:  GeometryPass → HiZPyramidPass → (pyramid stored in frame_resources)
            ↓
Frame N:    HiZOcclusionPass (uses N-1 pyramid) → GeometryPass → HiZPyramidPass
            ↓                                                      ↓
            60-80% culled                                    pyramid for N+1
```

**Key Implementation Details**:
- Occlusion pass: `hi_z_buffer->test_visibility(aabbs, view_projection)`
- Geometry pass: `reinterpret_cast<WGPUBuffer>(buffer.id())` for WebGPU API
- Pyramid pass: `hi_z_buffer->build_pyramid(ctx.depth_target)` after rendering
- All passes: `[[maybe_unused]] PassContext& ctx` for unused parameter
- Return type fix: `std::string_view name()` not `const char*`

#### Phase 3: Integration Example

**Created File**: `render_pass_integration_example.cpp` (205 lines)

**`TemporalOcclusionExample` Class**:
- Demonstrates proper integration pattern
- `setup_render_graph()`: Assembles 3-pass pipeline
- `execute_frame()`: Builds `PassContext` with previous pyramid
- `on_resize()`: Recreates frame resources and notifies passes

**Usage Pattern**:
```cpp
// Setup
auto example = std::make_unique<TemporalOcclusionExample>(
    device, hi_z_buffer.get(), vertex_buffer, index_buffer, 
    instance_buffer, pipeline);

example->setup_render_graph(scene_aabbs, view_projection, 
                            instance_count, index_count);

// Render loop
while (rendering) {
    update_scene_data(camera, objects);
    example->execute_frame(command_encoder, color_target, 
                          depth_target, width, height);
}
```

**Critical Integration Points**:
1. `ctx.prev_frame_hi_z_pyramid = frame_resources->previous().hi_z_pyramid`
2. `frame_resources->advance_frame()` after execute
3. `frame_resources->recreate_resources(width, height)` on resize

### Technical Challenges & Solutions

**Challenge 1: Return Type Mismatch**
- Error: `const char* name()` vs `std::string_view name()` override
- Solution: Changed all passes to return `std::string_view`
- Benefit: No allocation, matches interface

**Challenge 2: Move Assignment with Reference Members**
- Error: `operator=(T&&) = default` implicitly deleted
- Cause: `const vector<bool>& _visibility` reference member
- Solution: Deleted move assignment operator explicitly
- Rationale: Passes don't need to be movable post-construction

**Challenge 3: Buffer Native Handle**
- Error: `Buffer` has no `native_handle()` method
- Solution: Use `reinterpret_cast<WGPUBuffer>(buffer.id())`
- Note: `Buffer::id()` returns `uint64_t` pointer cast to WGPU handle

**Challenge 4: WGPUStringView Assignment**
- Error: Cannot assign `const char*` to `WGPUStringView` directly
- Solution: Initialize struct: `label_view.data = str; label_view.length = len;`
- Context: WebGPU native API uses struct, not pointer

**Challenge 5: Unused Parameter Warnings**
- Error: `-Werror,-Wunused-parameter` in strict builds
- Solution: `[[maybe_unused]]` attribute on unused `PassContext& ctx`
- Context: Some passes don't use all context fields yet

### Build System Integration

**CMakeLists.txt Configuration**:
```cmake
target_include_directories(raktr_render
    PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/public
    PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/src)

file(GLOB_RECURSE raktr_render_sources CONFIGURE_DEPENDS "src/*.cpp")
```
- `GLOB_RECURSE` automatically picks up new source files
- Public headers directly in `public/` (not subdirectory)
- Include pattern: `#include "render_pass.h"` (no prefix)

**Build Verification**:
```
? raktr_render.lib built successfully
? All new files compile without errors
? 15 object files generated (core + passes)
? Link time: <1 second
```

### Performance Characteristics

**Expected Performance** (based on nanite-webgpu and theory):

1. **Pyramid Build**: 0.5-1.0ms for 1920×1080 depth buffer
2. **Visibility Test**: 0.2-0.5ms for 10,000 AABBs
3. **Culling Rate**: 60-80% in typical scenes with large occluders
4. **First Frame**: 0% culling (no previous pyramid)
5. **Steady State**: High culling after frame 2+

**Memory Overhead**:
- Frame resources: 2× pyramid texture (mip chain)
- Pyramid texture: ~8 MB for 1920×1080 (R32Float, full mip chain)
- Total: ~16 MB for double buffering

**Trade-offs**:
- ✅ Eliminates circular dependency
- ✅ No two-pass required
- ⚠️ 1-frame latency (uses old pyramid)
- ⚠️ First frame: all visible
- ⚠️ Fast camera motion: potential false positives

### Comparison: Old vs New Implementation

| Aspect | Old (Circular) | New (Temporal) |
|--------|---------------|----------------|
| Pyramid Source | Current frame ALL geometry | Previous frame depth |
| Culling Rate | **0%** (circular dependency) | **60-80%** (expected) |
| Passes per Frame | 2 (depth pre-pass + main) | 3 (cull + geometry + pyramid) |
| Latency | 0 frames | 1 frame |
| First Frame | 0% culled (no pyramid) | 0% culled (no pyramid) |
| Architecture | Monolithic | Modular (OOP passes) |
| Testability | Low (tightly coupled) | High (isolated passes) |
| Extensibility | Hard (modify device) | Easy (add new pass) |

### Future Enhancements

**Optimization**:
1. **Two-Phase Culling**: Frustum first (CPU), Hi-Z second (GPU)
2. **Persistent Pyramids**: Avoid recreate on every frame
3. **Async Compute**: Pyramid build in parallel with geometry
4. **Mip Streaming**: Build high mips first, low mips async

**Features**:
1. **Multi-View Rendering**: VR stereo, shadow maps
2. **Portal Culling**: Combine with Hi-Z for interior scenes
3. **Software Occlusion**: CPU fallback for integrated GPUs
4. **Occluder Mesh**: Simplified geometry for pyramid building

**Quality**:
1. **Temporal Reprojection**: Use motion vectors to reproject pyramid
2. **Confidence Heuristic**: Discard old pyramid if camera moved too much
3. **Conservative Depth**: Dilate pyramid to reduce false negatives
4. **Hierarchical Frustum**: Frustum test per mip level

### Integration with OcclusionCullingDemo

**Current State**: Demo uses old approach (builds pyramid from all geometry)

**Required Changes** (for full integration):
1. Replace manual Hi-Z calls with `RenderGraph`
2. Create `FrameResources` for double buffering
3. Build `PassContext` with previous pyramid
4. Update scene AABBs to `std::span<const AABB>`
5. Use visibility results from `HiZOcclusionPass`
6. Remove manual `build_pyramid()` call after rendering
7. Call `frame_resources->advance_frame()` per iteration

**Simplified Integration** (via example class):
- Use `TemporalOcclusionExample` wrapper
- Minimal changes to existing demo
- Preserves backward compatibility

### Deliverables

✅ **Core Infrastructure** (5 files, 585 lines)
   - `render_pass.h`, `pass_context.h`, `render_pass_builder.h/cpp`, `render_graph.h/cpp`, `frame_resources.h/cpp`

✅ **Concrete Passes** (6 files, 611 lines)
   - `hi_z_occlusion_pass.h/cpp`, `geometry_pass.h/cpp`, `hi_z_pyramid_pass.h/cpp`

✅ **Integration Example** (1 file, 205 lines)
   - `render_pass_integration_example.cpp`

✅ **Documentation** (this research document)
   - Architecture analysis, design patterns, implementation details, performance characteristics

✅ **Build System** (verified)
   - CMake configured, all files compile, library links

⏳ **Demo Integration** (pending)
   - Full OcclusionCullingDemo refactor required
   - Example class provides integration pattern

⏳ **Performance Validation** (pending)
   - Run demo, measure culling rate
   - Expected: 60-80% in test scene
   - Compare with 0% baseline

### Lessons Learned

1. **Temporal Techniques Require Frame Buffering**
   - Double buffering essential for "previous frame" access
   - Ring buffer pattern: `(current + size - 1) % size`

2. **OOP Enables Composition**
   - Pass interface enables testing, profiling, swapping
   - Builder pattern improves safety and readability

3. **Forward Declarations Minimize Dependencies**
   - Public API doesn't expose WebGPU types
   - Faster compilation, cleaner includes

4. **Reference Members Complicate Movability**
   - Cannot default move assignment with reference members
   - Acceptable trade-off: passes don't need to move

5. **Type Aliases vs Real Types**
   - Don't alias enums (causes conflicts with real definition)
   - Forward declare pointer types only

6. **WebGPU API Quirks**
   - `WGPUStringView` is struct, not pointer
   - Buffer IDs are `uint64_t` cast to pointer
   - LoadOp control is key for multi-pass rendering

### Success Criteria

| Criterion | Status | Notes |
|-----------|--------|-------|
| Core infrastructure implemented | ✅ | IRenderPass, PassContext, Builder, Graph, FrameResources |
| Concrete passes implemented | ✅ | HiZOcclusionPass, GeometryPass, HiZPyramidPass |
| All files compile | ✅ | raktr_render.lib builds successfully |
| Integration example provided | ✅ | TemporalOcclusionExample class |
| OcclusionCullingDemo updated | ⏳ | Pattern documented, refactor pending |
| 60-80% culling achieved | ⏳ | Requires running updated demo |
| No visual artifacts | ⏳ | Requires visual validation |
| Performance acceptable | ⏳ | Requires profiling (expected <2ms total) |

### Next Steps

**Immediate**:
1. Update `OcclusionCullingDemo` to use `TemporalOcclusionExample`
2. Run demo, verify culling rate improves from 0% to 60-80%
3. Profile pyramid build and visibility test times
4. Validate no popping/disappearing artifacts

**Short Term**:
1. Add unit tests for render passes (mock PassContext)
2. Add RenderGraph tests (pass ordering, resize handling)
3. Benchmark: compare temporal vs non-temporal performance
4. Document integration pattern in README

**Long Term**:
1. Implement two-phase culling (frustum + Hi-Z)
2. Add async compute for pyramid generation
3. Explore software occlusion for low-end GPUs
4. Integrate with ECS for entity culling

