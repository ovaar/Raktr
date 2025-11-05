# Raktr

The next generation modern c++ 3D game engine — Where Worlds Take Shape.

## Setup

* Install [LLVM](https://releases.llvm.org/)
* Install [uv](https://docs.astral.sh/uv/) `pip install uv`

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
git clone https://github.com/ovaar/Raktr.git

pip install uv
uv sync
.\.venv\Scripts\Activate.ps1

# Create 3rd-party deps
conan export-pkg external/wgpu-native --version=27.0.2.0 -s:a build_type=Release
conan export-pkg external/wgpu-native --version=27.0.2.0 -s:a build_type=Debug

cd ./raktr/

# Install
conan install . --output-folder=../ -pr:a=../profiles/llvm_clang_cl.profile -o:a='&:with_tests=True' --build=missing

# Configure
cmake --preset conan-release -DENABLE_IWYU=ON

# Build
cmake --build --preset conan-release

# Build tests
cmake --build ..\build --target render_tests --config Release

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