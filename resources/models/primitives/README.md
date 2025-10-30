# Geometric Primitives

This directory contains basic geometric primitives in WaveFront OBJ format for testing and prototyping.

## Available Primitives

### triangle.obj
- **Description**: Equilateral triangle in XY plane
- **Vertices**: 3
- **Faces**: 1 triangle
- **Features**: Positions, normals, UVs
- **Use case**: Minimal test geometry, basic rasterization tests

### plane.obj
- **Description**: 2×2 square in XZ plane (Y=0)
- **Vertices**: 4
- **Faces**: 2 triangles
- **Features**: Positions, normals (pointing up), UVs
- **Use case**: Ground plane, texture mapping tests

### cube.obj
- **Description**: 2×2×2 cube centered at origin
- **Vertices**: 8
- **Faces**: 12 triangles (6 quad faces)
- **Features**: Positions, normals (per-face), UVs
- **Use case**: Basic 3D object, depth testing, backface culling

### pyramid.obj
- **Description**: Square pyramid with 2×2 base, apex at (0,1,0)
- **Vertices**: 5
- **Faces**: 6 triangles (4 sides + 2 base)
- **Features**: Positions, approximate face normals, UVs
- **Use case**: Non-convex faces, sloped surface lighting

### sphere.obj
- **Description**: Low-poly UV sphere (6 segments × 4 rings)
- **Vertices**: 20
- **Faces**: 36 triangles
- **Features**: Positions, smooth normals, structured topology
- **Use case**: Curved surface approximation, lighting tests

## Coordinate System

All primitives follow the **right-handed coordinate system**:
- **+X**: Right
- **+Y**: Up
- **+Z**: Towards viewer (out of screen)

## Winding Order

All faces use **counter-clockwise (CCW)** winding when viewed from the outside. This is the default for OpenGL backface culling.

## UV Coordinates

Texture coordinates are normalized to [0,1] range:
- **(0,0)**: Bottom-left
- **(1,1)**: Top-right

## Normal Convention

- Normals point **outward** from the surface
- Cube uses **flat shading** (per-face normals)
- Sphere uses **smooth shading** (per-vertex normals matching vertex positions)

## Usage Example

```cpp
#include "io/obj_loader.h"

auto mesh = raktr::render::io::load_obj_file("resources/models/primitives/cube.obj");
if (mesh.has_value()) {
    // mesh->positions, mesh->normals, mesh->uvs, mesh->indices
}
```

## Testing

These primitives are used in integration tests to verify:
- OBJ loader correctness
- Vertex/index buffer uploads
- Rasterization and depth testing
- Normal-based lighting
- Texture coordinate interpolation (future)

## Notes

- **Low polygon count**: Optimized for fast loading and rendering in tests
- **Simple topology**: Easy to reason about, predictable behavior
- **Complete attributes**: All primitives have positions, normals, and UVs
- **Manifold geometry**: Watertight meshes (except plane)
