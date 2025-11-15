
# copilot-instructions.md

## Purpose

This document instructs GitHub Copilot (and any code generator) how to produce code for **Raktr**.
These rules define Raktr’s architecture, coding style, and quality expectations to ensure the engine remains maintainable, portable, and production-ready.

> **High-level summary**
>
> * **Language:** C++23 (clang-compatible features only)
> * **Build:** CMake 4.x + Ninja
> * **Packages:** Conan 2.x
> * **Formatting:** clang-format (LLVM-based)
> * **Linting:** clang-tidy
> * **Testing:** GoogleTest + GoogleMock (TDD / Triple-A)
> * **Sanitizers:** ASan / MSan / TSan in CI + dev builds
> * **Targets:** Windows / Linux / macOS on x86_64 + arm64
> * **Math:** GLM via Conan
> * **Quality:** SOLID / KISS / Fail-Fast / DDD patterns

---

## Priority Guidelines

1. **Platform & Tooling**

   * Must build with CMake 4.x + Ninja and integrate with Conan 2.x.
   * Prefer clang / clang-cl; only use GCC / MSVC fallbacks when required.

2. **Language Standards**

   * Use modern, portable **C++23** features (concepts, ranges, `std::span`, `constexpr` algorithms).
   * Avoid compiler-specific extensions.

3. **Formatting & Linting**

   * Always format using clang-format (LLVM base + project overrides).
   * Code must be clang-tidy clean; prefer fixes that pass configured checks.

4. **Architecture & Layout**

   * Follow the canonical directory structure (see below).
   * Public headers in `include/raktr/`; implementation in `src/raktr/<subsystem>/`.

5. **API Surface**

   * Keep public API minimal and documented.
   * Everything public belongs under `raktr::` namespace.

6. **Testing**

   * Follow **Red → Green → Refactor** TDD cycle.
   * Always add a test before implementation.

7. **Safety / Performance**

   * Validate preconditions (Fail-Fast).
   * Use `gsl::not_null`, `std::span`, `std::optional`, or `std::expected` as appropriate.
   * Avoid hidden allocations and unnecessary dynamic memory.

8. **Documentation**

   * Use Doxygen-style `/*! … */` comments for all public APIs.

---

## Technology Stack

| Area                | Tooling / Policy                             |
| ------------------- | -------------------------------------------- |
| **Language**        | C++23                                        |
| **Build System**    | CMake ≥ 4.0 + Ninja                          |
| **Package Manager** | Conan 2.x (profiles + lockfiles)             |
| **Testing**         | GoogleTest / GoogleMock via CTest            |
| **Formatting**      | clang-format (LLVM-based)                    |
| **Linting**         | clang-tidy                                   |
| **Static Analysis** | ASan / TSan / UBSan                          |
| **Math**            | GLM (Conan package)                          |
| **CI**              | Multi-platform + multi-arch (x86_64 / arm64) |

---

## Directory Structure

```
raktr/
├── CMakeLists.txt
├── conanfile.py / conan.lock
├── .clang-format
├── .clang-tidy
├── raktr/
│   ├── engine/
│   │   ├── public/      # public API headers
│   │   └── src/         # implementation
│   │   │   ├── core/        # memory, logging, config
│   │   │   ├── math/        # GLM wrappers & helpers
│   │   │   ├── ecs/         # entity–component system
│   │   │   ├── scene/       # scene graph, camera
│   │   │   ├── io/          # filesystem, resource loading
│   │   │   ├── platform/    # platform abstractions
│   │   │   ├── physics/
│   │   │   ├── scripting/
│   │   │   └── CMakeLists.txt
│   │   └── tests/       # unit/integration tests for engine
│   ├── renderer/            # 3d renderer subsystem supporting various graphics backends
│   │   ├── public/      # public API headers
│   │   └── src/         # implementation
│   │   │   ├── opengl/      # OpenGL backend
│   │   │   ├── vulkan/      # Vulkan backend
│   │   │   └── directx12/   # DirectX 12 backend
│   │   │   └── CMakeLists.txt
│   │   └── tests/       # unit/integration tests for renderer
│   └── editor/            # editor executables
├── tests/
├── external/
├── tools/
├── assets/
├── docs/
└── ci/
```

* **Filenames:** `snake_case`. Example: `render_pipeline.cpp`.
* **Headers:** `ifndef` include guard preferred.
* **Header placement:**

  * Public → `include/raktr/`
  * Private → `src/raktr/<subsystem>/detail/`

---

## Naming Conventions

| Element                                  | Style                                   | Example                          |
| ---------------------------------------- | --------------------------------------- | -------------------------------- |
| **Namespaces**                           | `raktr::subsystem`                      | `raktr::render`                  |
| **Classes / Structs / Enums / Concepts** | PascalCase                              | `Renderer`, `TransformComponent` |
| **Functions / Methods**                  | snake_case                              | `create_mesh()`                  |
| **Variables**                            | snake_case                              | `frame_index`                   |
| **Private members**                      | snake_case                              | `_frame_index`                   |
| **Constants**                            | `k_constant_name` or `inline constexpr` | `k_max_frames`                   |
| **CMake targets**                        | snake_case                              | `raktr_core`                     |

---

## RAII + copy-and-swap Idiom

When managing resources, use RAII and the copy-and-swap idiom for strong exception safety:

```cpp
#include <iostream>
#include <vector>
#include <string>
#include <stdint.h>
#include <optional>

class my_array
{
public:
    // (default) constructor
    my_array(std::size_t size = 0)
        : mSize(size),
          mArray(mSize ? new int[mSize]() : nullptr) {}

    // copy-constructor
    my_array(const my_array& other)
        : mSize(other.mSize),
          mArray(mSize ? new int[mSize] : nullptr)
    {
      // note that this is non-throwing, because of the data
      // types being used; more attention to detail with regards
      // to exceptions must be given in a more general case, however
      std::copy(other.mArray, other.mArray + mSize, mArray);
    }

    // Move constructor
    my_array(my_array&& other) noexcept
        : my_array() // initialize via default constructor, C++11 only
    {
      swap(*this, other);
    }

    // destructor
    ~my_array()
    {
      delete [] mArray;
    }

    // Copy Assignment operator
    my_array& operator=(const my_array& other) noexcept
    {
      my_array tmp(other);
      swap(*this, tmp);

      return *this;
    }

    my_array& operator=(my_array&& other) noexcept
    {
      my_array tmp(std::move(other));
      swap(*this, tmp);
      return *this;
    }
    
    std::size_t size() const {
      return mSize;
    }
    int* get_array() {
      return mArray;
    }

    friend void swap(my_array& first, my_array& second) noexcept // nothrow
    {
      // enable ADL (not necessary in our case, but good practice)
      using std::swap;

      // by swapping the members of two objects,
      // the two objects are effectively swapped
      swap(first.mSize, second.mSize);
      swap(first.mArray, second.mArray);
    }

private:
    std::size_t mSize;
    int* mArray;
};
```

## Docstrings

All public APIs must use Doxygen-style comments:

```cpp
/*!
 * @brief Splits a string into substrings.
 * @example
 * auto parts = split("a,b,c", ",");
 * // => ["a","b","c"]
 * @param str  Input string.
 * @param sep  Separator characters.
 * @return Vector of substrings.
 */
```

Include `@brief`, `@param`, `@return`, and `@example`.
Document invariants, ownership, and thread-safety.

---

## Coding Practices

* **SOLID / KISS** – small, focused classes.
* **Fail-Fast** – validate arguments early.
* **Pure Functions** – prefer stateless helpers.
* **Encapsulation** – minimal public surface; avoid `friend`.
* **Memory & Ownership** – use smart pointers and `std::span`. Never call `new` / `delete`.
* **Concurrency** – isolate shared state; prefer message-passing.
* **Error Handling** – no exceptions. Use `std::expected` or `std::error_code` via `raktr::error` category.
* **Performance** – avoid hidden allocations; expose profiling hooks.

---

## Testing

* **Framework:** GoogleTest + GoogleMock.
* **Style:** Triple-A (Arrange / Act / Assert).
* **Naming:** `Method_Scenario_Expected`.
* **Location:** `tests/<subsystem>/test_<feature>.cpp`.

Example:

```cpp
#include "gtest/gtest.h"
#include "raktr/math/vec3.h"

TEST(Vec3_add, two_zero_vectors_returns_zero_vector) {
    raktr::math::Vec3 a{}, b{};
    auto result = a + b;
    EXPECT_FLOAT_EQ(result.x(), 0.0f);
    EXPECT_FLOAT_EQ(result.y(), 0.0f);
    EXPECT_FLOAT_EQ(result.z(), 0.0f);
}
```

---

## Build & CI

### CMake

* Use `target_*` commands; never global includes.
* Provide `RaktrConfig.cmake` and per-subsystem options.
* Mandatory warning flags:

  ```cmake
  -Wall -Wextra -Werror -Wpedantic -Wshadow -Wconversion
  ```

### Conan

* All dependencies pinned in `conanfile.py`.
* Use profiles + lockfiles; integrate toolchain with CMake.

### Sanitizers

* Enable via CMake options:
  `-fsanitize=address,undefined` etc.
* CI runs sanitizer builds per platform.

### CI Matrix

* Platforms: Windows / Linux / macOS
* Architectures: x86_64 / arm64
* Jobs: build + test + clang-tidy + clang-format + sanitizers + coverage (`gcovr` / `llvm-cov`)

### Build instructions

* see `README.md` for setup, build, and test commands.

---

## Formatting (`.clang-format`)

```yaml
BasedOnStyle: LLVM
IndentWidth: 4
ColumnLimit: 100
AllowShortFunctionsOnASingleLine: Empty
SpacesBeforeTrailingComments: 1
SpaceBeforeParens: ControlStatements
AlignAfterOpenBracket: DontAlign
BreakBeforeBraces: Attach
IndentCaseLabels: false
DerivePointerBinding: false
```

---

## Linting / Static Analysis

* Configure `.clang-tidy` with checks:
  `cppcoreguidelines-*`, `performance-*`, `modernize-*`, `readability-*`, `bugprone-*`.
* Code must compile warning-free under these checks.

---

## Development Tooling

* **Pre-commit hooks** must run:

  * `clang-format`
  * `clang-tidy`
  * `cmake-format`
  * `ctest --output-on-failure`

* **Local scripts** in `scripts/`:

  * `setup_dev_env.sh`
  * `format.sh`
  * `run_tests.sh --with-sanitizers`

---

## Platform Abstraction

All OS-specific logic resides in `src/raktr/platform/`.
Do **not** scatter `#ifdef` blocks elsewhere.
Use defines `RAKTR_ARCH_ARM64` / `RAKTR_ARCH_X86_64` for SIMD paths.

---

## Dependencies & Third-Party Code

* Managed via Conan 2.x; never hard-coded includes.
* Vendored libs (if any) live under `/external/`.
* Never modify vendor source; patch externally.

---

## Contribution Flow

* **Branches:**
  `main` (stable), `dev` (active), `feature/<subsystem>`
* **Commits:**
  Follow [Conventional Commits 1.0.0](https://www.conventionalcommits.org/en/v1.0.0/#specification)
* **PR Requirements:**
  Pass `clang-format`, `clang-tidy`, sanitizers, and all tests.
* **CI Platform:**
  GitHub Actions; auto-merge only when all checks succeed.

---

## Optional Enhancements

* Code generation conventions (shader reflection, ECS components)
* Scripting integration (Lua, Python)
* Asset build pipeline (shader/model preprocessing)
* Subsystem naming consistency (`raktr::render::renderer_backend`)
* Data layout guidelines (`std::array`, `std::vector`, `std::span`, `std::string_view`, `gsl::not_null`, `std::optional`, `std::expected`)

---

## When in Doubt

1. Mirror existing code patterns.
2. Write the test first (TDD).
3. Keep new headers under `include/raktr/...`.
4. Touch top-level build files only when adding public components.
5. Document anything non-obvious in `docs/`.

---

## Pull-Request Checklist

* [ ] Tests added or updated (Red → Green)
* [ ] All public APIs documented (`/*! */`)
* [ ] Builds with `cmake -S . -B build -G Ninja`
* [ ] `clang-format` shows no diffs
* [ ] `clang-tidy` passes cleanly
* [ ] Platform-specific code isolated in `src/raktr/platform/`
* [ ] Sanitizer tests run locally

