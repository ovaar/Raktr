# Math Utilities

This module provides convenient wrappers around GLM (OpenGL Mathematics) for 3D transformations.

## Two Approaches

Raktr offers **two ways** to work with transformations:

1. **Type-Safe Wrappers** (`transform_types.h`) - Strongly-typed, prevents errors, self-documenting ✨ **Recommended for beginners**
2. **Function Helpers** (`transform.h`) - Direct GLM usage, more flexible for advanced users

---

## Type-Safe Approach (Recommended)

Include the type-safe transform header:

```cpp
#include "math/transform_types.h"

using namespace raktr::render::math;
```

### Basic Example

```cpp
// Create strongly-typed transformations
Rotation model(angle, Axis::Y());  // Rotate around Y axis
View view = View::look_at(
    glm::vec3(0.0f, 0.0f, 3.0f),  // Camera position
    glm::vec3(0.0f, 0.0f, 0.0f),  // Look at origin
    glm::vec3(0.0f, 1.0f, 0.0f)   // Up vector
);
Perspective projection = Perspective::from_fov_degrees(
    45.0f,        // FOV in degrees
    aspect_ratio, // Aspect ratio
    0.1f,         // Near plane
    100.0f        // Far plane
);

// Compose with type-safe operators (enforces correct order)
ModelViewProjection mvp = projection * view * model;

// Upload to uniform buffer
device->update_uniform_buffer(uniform_buf, mvp.to_bytes());
```

### Benefits

- **Type Safety**: Compiler prevents mixing transformation types incorrectly
- **Self-Documenting**: `Rotation` is clearer than `glm::mat4`
- **Correct Ordering**: Operators enforce projection * view * model order
- **Beginner Friendly**: Clear intent, harder to make mistakes

### Available Types

**Basic Transformations:**
- `Rotation(angle, axis)` - Rotation around an axis
- `Translation(x, y, z)` - Position offset
- `Scale(x, y, z)` or `Scale::uniform(factor)` - Scaling

**Camera:**
- `View::look_at(eye, target, up)` - Camera view matrix

**Projection:**
- `Perspective::from_fov(radians, aspect, near, far)` - Perspective projection
- `Perspective::from_fov_degrees(degrees, aspect, near, far)` - Perspective in degrees
- `Orthographic::from_bounds(l, r, b, t, near, far)` - Orthographic projection

**Combined:**
- `Model` - Combined model transformations
- `ModelView` - View * Model
- `ModelViewProjection` - Projection * View * Model

**Helpers:**
- `Axis::X()`, `Axis::Y()`, `Axis::Z()` - Standard axes
- `Axis(x, y, z)` - Custom axis (auto-normalized)

### Complex Example

```cpp
// Build complex model transformation
Model model = Translation(0.0f, 1.0f, 0.0f) *  // Move up
              Rotation(angle, Axis::Y()) *      // Rotate around Y
              Scale::uniform(2.0f);             // Scale 2x

// Combine with camera and projection
View view = View::look_at({5, 5, 5}, {0, 0, 0});
Perspective proj = Perspective::from_fov_degrees(60.0f, 16.0f/9.0f, 0.1f, 100.0f);

ModelViewProjection mvp = proj * view * model;
device->update_uniform_buffer(buf, mvp.to_bytes());
```

---

## Function Helper Approach

Include the function helper header:

```cpp
#include "math/transform.h"
#include <glm/glm.hpp>

using namespace raktr::render;
```

### Basic Example

```cpp
// Create transformation matrices using helper functions
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

---

## Coordinate System Utilities

The math library includes right-handed coordinate system utilities and strongly-typed coordinate space separation.

### Coordinate System (`coordinate_system.h`)

Provides utilities for working with right-handed coordinate systems (OpenGL/Vulkan convention):

```cpp
#include "math/coordinate_system.h"

using namespace raktr::render::math;

// Standard basis vectors
glm::vec3 right = CoordinateBasis::Standard::right();     // (1, 0, 0)
glm::vec3 up = CoordinateBasis::Standard::up();           // (0, 1, 0)
glm::vec3 forward = CoordinateBasis::Standard::forward(); // (0, 0, -1)

// Validate handedness
bool is_valid = CoordinateBasis::is_right_handed(right, up, forward);

// Compute basis vectors from two axes
glm::vec3 computed_forward = CoordinateBasis::forward_from_right_up(right, up);
```

**Convention:**
- Right-handed coordinate system
- Right = +X, Up = +Y, Forward = -Z (toward viewer in OpenGL/Vulkan)
- All cross products account for -Z forward convention

### Coordinate Spaces (`coordinate_spaces.h`)

Strongly-typed coordinate space separation prevents mixing vectors from different coordinate systems:

```cpp
#include "math/coordinate_spaces.h"

using namespace raktr::render::math::spaces;

// Create vectors in different spaces
LocalVector local_pos(1.0f, 2.0f, 3.0f);
WorldVector world_pos(5.0f, 10.0f, -3.0f);

// ✅ Same-space operations work
LocalVector offset(0.5f, 0.0f, 0.0f);
LocalVector combined = local_pos + offset;

// ❌ Won't compile - different spaces
// auto bad = local_pos + world_pos;

// Transform between spaces
LocalToWorld model_transform(model_matrix);
WorldVector transformed = model_transform * local_pos;
```

**Available Spaces:**
- `LocalSpace` - Object's local coordinate system
- `WorldSpace` - Global scene coordinates
- `ViewSpace` - Camera-relative coordinates
- `ClipSpace` - Post-projection homogeneous coordinates
- `NDCSpace` - Normalized Device Coordinates [-1,1]
- `ScreenSpace` - Pixel/viewport coordinates
- `TangentSpace` - Per-vertex surface space (normal mapping)

**Benefits:**
- Compile-time space checking
- Zero runtime overhead
- Self-documenting code
- Prevents common graphics bugs

See [Coordinate Space Examples](coordinate_spaces_examples.md) for comprehensive usage patterns.

---

## See Also

- [Coordinate Space Examples](coordinate_spaces_examples.md) - Practical examples and patterns
- [GLM Documentation](https://github.com/g-truc/glm)
- Full example: `raktr/render/tests/test_visual_triangle.cpp` (DISABLED_SpinningCube test)
