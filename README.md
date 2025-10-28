# Raktr

The next generation modern c++ 3D game engine — Where Worlds Take Shape.

## Setup

```sh
pip install uv
```

### Activate venv

```sh
source .venv/bin/activate
```

```ps1
.\.venv\Scripts\Activate.ps1
```

## Build

### Windows 

```ps1
# Install
conan install raktr --output-folder=. -pr:a=profiles/llvm_clang_vs.profile -o:a='&:with_tests=True'

# Configure
cd raktr/; cmake --preset conan-release 

# Build
cmake --build --preset conan-release

# Clean
cmake --build --preset conan-release --clean
```




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