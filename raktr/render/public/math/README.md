# Math Utilities

This module provides convenient wrappers around GLM (OpenGL Mathematics) for 3D transformations.

## Usage

Include the transform header to access transformation utilities:

```cpp
#include "math/transform.h"
#include <glm/glm.hpp>

using namespace raktr::render;
```

## Basic Example

```cpp
// Create transformation matrices
auto model = math::create_rotation(angle, glm::vec3(0.0f, 1.0f, 0.0f));
auto view = math::create_look_at(
    glm::vec3(0.0f, 0.0f, 3.0f),  // Camera position
    glm::vec3(0.0f, 0.0f, 0.0f),  // Look at origin
    glm::vec3(0.0f, 1.0f, 0.0f)   // Up vector
);
auto projection = math::create_perspective(
    glm::radians(45.0f),  // FOV
    aspect_ratio,         // Aspect ratio
    0.1f,                 // Near plane
    100.0f                // Far plane
);

// Compute MVP matrix
glm::mat4 mvp = projection * view * model;

// Upload to uniform buffer
device->update_uniform_buffer(uniform_buf, math::matrix_to_bytes(mvp));
```

## Available Functions

### Projection

- **`create_perspective(fov, aspect, near, far)`** - Creates a perspective projection matrix
- FOV in radians, use `glm::radians(degrees)` to convert

### View (Camera)

- **`create_look_at(eye, target, up)`** - Creates a view matrix for a camera
- `eye`: Camera position
- `target`: Point to look at
- `up`: Up direction (typically `{0, 1, 0}`)

### Transformations

- **`create_rotation(angle, axis)`** - Creates a rotation matrix around an axis
- **`create_translation(vec)`** - Creates a translation matrix
- **`create_scale(vec)`** - Creates a scale matrix

### Utility

- **`matrix_to_bytes(matrix)`** - Converts GLM matrix to byte span for uniform buffer upload

## Direct GLM Access

You can also use GLM directly since it's a public dependency:

```cpp
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>

// Build transformations manually
glm::mat4 model = glm::mat4(1.0f);
model = glm::translate(model, glm::vec3(1.0f, 0.0f, 0.0f));
model = glm::rotate(model, glm::radians(45.0f), glm::vec3(0.0f, 1.0f, 0.0f));
model = glm::scale(model, glm::vec3(2.0f));
```

## Matrix Ordering

GLM uses column-major matrices, which matches WGSL/GLSL shader expectations.
Matrix multiplication order: `projection * view * model`

## See Also

- [GLM Documentation](https://github.com/g-truc/glm)
- Full example: `raktr/render/tests/test_visual_triangle.cpp` (DISABLED_SpinningCube test)
