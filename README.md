# Raktr

The next generation modern c++ 3D game engine — Where Worlds Take Shape.

## Setup

```sh
pip install uv
uv sync
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