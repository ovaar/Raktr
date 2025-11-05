# Coordinate Space Examples

This document demonstrates practical usage patterns for the strongly-typed coordinate space system and shows what operations are allowed versus what will fail at compile time.

## Table of Contents

- [Coordinate Space Examples](#coordinate-space-examples)
  - [Table of Contents](#table-of-contents)
  - [Basic Usage](#basic-usage)
  - [Transforming Between Spaces](#transforming-between-spaces)
  - [Composing Transformations](#composing-transformations)
  - [Direction vs Position Vectors](#direction-vs-position-vectors)
  - [Real-World MVP Pipeline](#real-world-mvp-pipeline)
  - [Preventing Common Bugs](#preventing-common-bugs)
  - [Advanced Pattern: Per-Object Local Spaces](#advanced-pattern-per-object-local-spaces)
  - [Summary](#summary)

---

## Basic Usage

Shows how to create vectors in different spaces and demonstrates that same-space operations work correctly.

```cpp
#include "math/coordinate_spaces.h"

using namespace raktr::render::math::spaces;

void basic_usage() {
    // Create vectors in different spaces
    LocalVector local_pos(1.0f, 2.0f, 3.0f);
    WorldVector world_pos(5.0f, 10.0f, -3.0f);
    
    // ✅ Same-space arithmetic works
    LocalVector local_offset(0.5f, 0.0f, 0.0f);
    LocalVector combined = local_pos + local_offset;
    
    // ✅ Vector operations in same space work
    float distance_sq = local_pos.dot(local_offset);
    LocalVector cross = local_pos.cross(local_offset);
    
    // ❌ These would NOT compile (different spaces):
    // auto bad1 = local_pos + world_pos;           // ❌ Can't add local to world
    // float bad2 = local_pos.dot(world_pos);       // ❌ Can't dot local with world
    // auto bad3 = local_pos.cross(world_pos);      // ❌ Can't cross local with world
}
```

**Key Points:**
- Vectors are tagged with their coordinate space at compile time
- Operations between same-space vectors are allowed
- Operations between different-space vectors won't compile

---

## Transforming Between Spaces

Demonstrates how to explicitly transform vectors between spaces using typed transformation matrices.

```cpp
#include "math/coordinate_spaces.h"
#include <glm/gtc/matrix_transform.hpp>

using namespace raktr::render::math::spaces;

void space_transformations() {
    // Define transforms
    glm::mat4 model = glm::translate(glm::mat4(1.0f), glm::vec3(5, 0, 0));
    glm::mat4 view = glm::lookAt(
        glm::vec3(0, 0, 10),  // eye
        glm::vec3(0, 0, 0),   // target
        glm::vec3(0, 1, 0)    // up
    );
    glm::mat4 projection = glm::perspective(
        glm::radians(45.0f), 16.0f/9.0f, 0.1f, 100.0f
    );
    
    // Create typed transforms
    LocalToWorld local_to_world(model);
    WorldToView world_to_view(view);
    ViewToClip view_to_clip(projection);
    
    // Transform through the pipeline
    LocalVector vertex(1, 0, 0);
    WorldVector world_vertex = local_to_world * vertex;
    ViewVector view_vertex = world_to_view * world_vertex;
    ClipVector clip_vertex = view_to_clip * view_vertex;
    
    // ❌ These would NOT compile (wrong space order):
    // auto bad1 = world_to_view * vertex;          // ❌ WorldToView needs WorldVector
    // auto bad2 = local_to_world * world_vertex;   // ❌ LocalToWorld needs LocalVector
}
```

**Key Points:**
- `SpaceTransform<FromSpace, ToSpace>` ensures correct transformation order
- Type system prevents applying transforms to wrong space
- Explicit space transitions make data flow clear

---

## Composing Transformations

Shows how transformations can be composed while maintaining type safety.

```cpp
#include "math/coordinate_spaces.h"
#include <glm/gtc/matrix_transform.hpp>

using namespace raktr::render::math::spaces;

void transform_composition() {
    glm::mat4 model = glm::translate(glm::mat4(1.0f), glm::vec3(5, 0, 0));
    glm::mat4 view = glm::lookAt(
        glm::vec3(0, 0, 10),
        glm::vec3(0, 0, 0),
        glm::vec3(0, 1, 0)
    );
    glm::mat4 projection = glm::perspective(glm::radians(45.0f), 1.0f, 0.1f, 100.0f);
    
    LocalToWorld l2w(model);
    WorldToView w2v(view);
    ViewToClip v2c(projection);
    
    // ✅ Compose transforms with correct type order
    LocalToView l2v = w2v * l2w;        // OK: WorldToView * LocalToWorld
    LocalToClip l2c = v2c * l2v;        // OK: ViewToClip * LocalToView
    
    // Alternative: compose all at once
    LocalToClip mvp = v2c * w2v * l2w;  // ✅ OK
    
    // Now we can transform directly from local to clip
    LocalVector vertex(1, 0, 0);
    ClipVector clip = mvp * vertex;
    
    // ❌ These would NOT compile (wrong composition order):
    // auto bad1 = l2w * w2v;              // ❌ Can't compose LocalToWorld * WorldToView
    // auto bad2 = l2w * v2c;              // ❌ Can't compose LocalToWorld * ViewToClip
}
```

**Key Points:**
- Transforms compose right-to-left: `B * A` means "apply A, then B"
- Type system enforces correct composition order
- Composed transforms maintain end-to-end type safety

---

## Direction vs Position Vectors

Shows the difference between transforming positions (w=1) and directions (w=0), which affects translation.

```cpp
#include "math/coordinate_spaces.h"
#include <glm/gtc/matrix_transform.hpp>

using namespace raktr::render::math::spaces;

void directions_vs_positions() {
    glm::mat4 model = glm::translate(glm::mat4(1.0f), glm::vec3(10, 20, 30));
    LocalToWorld transform(model);
    
    // Position vector: affected by translation
    LocalVector position(1, 0, 0);
    WorldVector world_pos = transform * position;
    // world_pos will be approximately (11, 20, 30)
    
    // Direction vector: NOT affected by translation
    LocalVector direction(1, 0, 0);
    WorldVector world_dir = transform.transform_direction(direction);
    // world_dir will be (1, 0, 0) - translation ignored
}
```

**Key Points:**
- `operator*` treats vectors as positions (w=1, affected by translation)
- `transform_direction()` treats vectors as directions (w=0, ignores translation)
- Use `transform_direction()` for normals, tangents, and other directional data

---

## Real-World MVP Pipeline

Demonstrates a complete Model-View-Projection transformation pipeline with type safety throughout.

```cpp
#include "math/coordinate_spaces.h"
#include <glm/gtc/matrix_transform.hpp>

using namespace raktr::render::math::spaces;

void mvp_pipeline() {
    // Setup scene
    glm::mat4 model = glm::rotate(
        glm::translate(glm::mat4(1.0f), glm::vec3(0, 0, -5)),
        glm::radians(45.0f),
        glm::vec3(0, 1, 0)
    );
    
    glm::mat4 view = glm::lookAt(
        glm::vec3(0, 2, 10),
        glm::vec3(0, 0, 0),
        glm::vec3(0, 1, 0)
    );
    
    glm::mat4 projection = glm::perspective(
        glm::radians(45.0f),
        1920.0f / 1080.0f,
        0.1f,
        1000.0f
    );
    
    // Create typed transforms
    LocalToWorld model_transform(model);
    WorldToView view_transform(view);
    ViewToClip projection_transform(projection);
    
    // Compose into full MVP
    LocalToClip mvp = projection_transform * view_transform * model_transform;
    
    // Transform vertices
    LocalVector vertices[] = {
        LocalVector(-1, -1, 0),
        LocalVector( 1, -1, 0),
        LocalVector( 0,  1, 0)
    };
    
    // Each vertex goes through the full pipeline
    for (const auto& vertex : vertices) {
        ClipVector clip_pos = mvp * vertex;
        // Ready for rasterization
    }
    
    // Also compute lighting in world space
    LocalVector normal(0, 0, 1);
    WorldVector world_normal = model_transform.transform_direction(normal);
    // Use world_normal for lighting calculations
}
```

**Key Points:**
- Separate transforms for each stage of the pipeline
- Type-safe composition into combined MVP matrix
- Can extract intermediate results (e.g., world-space normals for lighting)
- Clear data flow from local space → world space → view space → clip space

---

## Preventing Common Bugs

Demonstrates how the type system prevents common mistakes that would otherwise cause subtle rendering bugs.

```cpp
#include "math/coordinate_spaces.h"
#include <glm/gtc/matrix_transform.hpp>

using namespace raktr::render::math::spaces;

void preventing_bugs() {
    glm::mat4 model1 = glm::translate(glm::mat4(1.0f), glm::vec3(5, 0, 0));
    glm::mat4 model2 = glm::translate(glm::mat4(1.0f), glm::vec3(0, 5, 0));
    
    LocalToWorld transform1(model1);
    LocalToWorld transform2(model2);
    
    LocalVector obj1_pos(1, 0, 0);
    LocalVector obj2_pos(0, 1, 0);
    
    // ✅ Correct: transform each to world space first
    WorldVector world_obj1 = transform1 * obj1_pos;
    WorldVector world_obj2 = transform2 * obj2_pos;
    WorldVector difference = world_obj1 - world_obj2;
    
    // ❌ Common bug prevented by type system:
    // auto bad = obj1_pos - obj2_pos;
    // This looks innocent but is WRONG! obj1_pos and obj2_pos are in
    // DIFFERENT local spaces (different objects). The compiler prevents
    // this because they're both LocalVector but from different objects.
    // 
    // To properly handle this, you'd need:
    // struct Object1LocalSpace {};
    // struct Object2LocalSpace {};
    // Vector3<Object1LocalSpace> obj1_pos;
    // Vector3<Object2LocalSpace> obj2_pos;
    // Now obj1_pos - obj2_pos won't compile!
}
```

**Key Points:**
- Each object has its own local space
- Cannot directly compare positions from different local spaces
- Must transform to common space (usually world space) before operations
- Type system catches these bugs at compile time, not runtime

---

## Advanced Pattern: Per-Object Local Spaces

For maximum safety, you can create unique space types for each object:

```cpp
// Define unique spaces per object
struct Player1LocalSpace {};
struct Player2LocalSpace {};
struct EnemyLocalSpace {};

// Now these are incompatible types
Vector3<Player1LocalSpace> player1_pos(1, 0, 0);
Vector3<Player2LocalSpace> player2_pos(2, 0, 0);

// ❌ Won't compile - different local spaces
// auto bad = player1_pos + player2_pos;

// ✅ Must explicitly transform to common space
SpaceTransform<Player1LocalSpace, WorldSpace> player1_transform(model1);
SpaceTransform<Player2LocalSpace, WorldSpace> player2_transform(model2);

Vector3<WorldSpace> world_player1 = player1_transform * player1_pos;
Vector3<WorldSpace> world_player2 = player2_transform * player2_pos;

// ✅ Now we can safely compare in world space
Vector3<WorldSpace> distance = world_player1 - world_player2;
```

This provides the strongest compile-time guarantees but requires more boilerplate.

---

## Summary

The strongly-typed coordinate space system provides:

| Benefit | Description |
|---------|-------------|
| **Compile-Time Safety** | Mixing vectors from different spaces won't compile |
| **Zero Runtime Overhead** | Template metaprogramming resolves at compile time |
| **Self-Documenting** | Type names show which space vectors belong to |
| **GLM Compatible** | Can extract underlying `glm::vec3` when needed |
| **Bug Prevention** | Catches coordinate space errors at compile time |

Use this system whenever working with transformations between coordinate spaces to catch bugs early and make your code more maintainable.
