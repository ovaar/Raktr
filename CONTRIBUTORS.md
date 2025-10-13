Here’s a **concise “executive summary”** version of your `copilot-instructions.md` — perfect for including in your root `README.md` under something like **“Code Generation & Style Summary”** or **“Contributor TL;DR”**.

It captures all key policies in a fast, high-level form without losing substance:

---

# 🧭 Raktr Code Generation & Style Summary

> **Purpose:**
> This is a quick reference for how code should be written, generated, and structured for **Raktr**, our cross-platform C++23 3D engine.
> It summarizes conventions from `copilot-instructions.md` and applies to both humans and AI assistants.

---

## ⚙️ Core Principles

| Area           | Policy                                              |
| -------------- | --------------------------------------------------- |
| **Language**   | C++23 (clang-first, portable to GCC/MSVC)           |
| **Build**      | CMake ≥ 4.0 + Ninja                                 |
| **Packages**   | Conan 2.x (profiles + lockfiles)                    |
| **Formatting** | clang-format (LLVM base + overrides)                |
| **Linting**    | clang-tidy (`cppcoreguidelines`, `modernize`, etc.) |
| **Testing**    | GoogleTest + GoogleMock (TDD / Triple-A)            |
| **Sanitizers** | ASan / TSan / UBSan in CI                           |
| **Platforms**  | Windows / Linux / macOS (x86_64 + arm64)            |
| **Math**       | GLM via Conan                                       |
| **Philosophy** | SOLID, KISS, Fail-Fast, Domain-Driven Design        |

---

## 🧱 Project Layout

```
raktr/
├── include/raktr/      # Public headers
├── src/raktr/          # Implementation (by subsystem)
│   ├── core/           # Core runtime (memory, logging, config)
│   ├── math/           # GLM helpers
│   ├── ecs/            # Entity–component system
│   ├── render/         # Rendering backend
│   ├── scene/          # Scene graph
│   ├── io/             # Filesystem & resources
│   ├── platform/       # Platform abstraction
│   └── scripting/      # Future scripting integration
├── tests/              # GoogleTest suites
├── external/           # Vendored third-party code
└── ci/                 # CI/CD configurations
```

**File naming:** `snake_case.cpp`, `snake_case.h`
**Headers:** Public → `include/raktr/`; Private → `src/raktr/.../detail/`

---

## ✍️ Naming Conventions

| Element               | Convention         | Example                         |
| --------------------- | ------------------ | ------------------------------- |
| Namespaces            | `raktr::subsystem` | `raktr::render`                 |
| Types                 | PascalCase         | `Renderer`, `Vec3`              |
| Functions / Variables | snake_case         | `create_mesh()`, `frame_index_` |
| Constants             | `k_constant_name`  | `k_max_frames`                  |
| CMake Targets         | snake_case         | `raktr_core`                    |

---

## 📚 Documentation

All public APIs use Doxygen-style blocks:

```cpp
/*!
 * @brief Adds two vectors.
 * @param a First vector.
 * @param b Second vector.
 * @return Sum of both vectors.
 */
```

Include `@brief`, `@param`, `@return`, and examples when possible.
Document invariants, ownership, and thread safety.

---

## 🧩 Coding Standards

* **No exceptions.** Use `std::expected` or `std::error_code` (`raktr::error` domain).
* **Fail Fast:** Validate arguments early.
* **Memory safety:** Prefer smart pointers and `std::span`. No raw `new` / `delete`.
* **Pure functions:** Limit side effects.
* **Concurrency:** Avoid shared mutable state.
* **Performance:** Avoid hidden allocations; expose profiling hooks.

---

## 🧪 Testing

* **Framework:** GoogleTest + GoogleMock
* **Pattern:** Triple-A (Arrange / Act / Assert)
* **Naming:** `Method_Scenario_Expected`
* **Location:** `tests/<subsystem>/test_<feature>.cpp`
* **Goal:** Write tests first (TDD).

Example:

```cpp
TEST(Vec3_add, two_zero_vectors_returns_zero_vector) {
    raktr::math::Vec3 a{}, b{};
    auto result = a + b;
    EXPECT_FLOAT_EQ(result.x(), 0.0f);
    EXPECT_FLOAT_EQ(result.y(), 0.0f);
    EXPECT_FLOAT_EQ(result.z(), 0.0f);
}
```

---

## 🔧 Build & CI Highlights

* **CMake:** Modern target-based config (`target_link_libraries`, etc.).
* **Warnings:**

  ```
  -Wall -Wextra -Werror -Wpedantic -Wshadow -Wconversion
  ```
* **CI Matrix:** Windows, Linux, macOS × x86_64 / arm64.
* **Checks:** clang-format, clang-tidy, ASan/TSan, tests, coverage.
* **GitHub Actions:** Auto-merge only when all pass.

---

## 🔒 Contribution Flow

* **Branches:** `main` (stable), `dev` (active), `feature/<name>`
* **Commits:** Follow [Conventional Commits](https://www.conventionalcommits.org/)
* **PRs must:**

  * Include tests
  * Pass lint + format
  * Build cleanly
  * Document new APIs

---

## 🧰 Developer Tools

* Pre-commit hooks run:
  `clang-format`, `clang-tidy`, `cmake-format`, `ctest`.
* Scripts (in `/scripts`):

  * `setup_dev_env.sh`
  * `format.sh`
  * `run_tests.sh --with-sanitizers`

---

## 🚦 Pull Request Checklist

* [ ] Tests added or updated (TDD: Red → Green)
* [ ] All public APIs documented (`/*! */`)
* [ ] `clang-format` + `clang-tidy` clean
* [ ] Sanitizer builds pass locally
* [ ] Platform-specific code isolated under `/platform`

