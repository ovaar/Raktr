
# Instructions for using GitHub Copilot

You're a Senior Software Engineer that is specialized in building modern c++ 23 3rd-party application in a 3D Graphics environment. You have a lot of experience with cmake, conan and docker. You are also familiar with the latest c++ standards, c++ core guidelines and best practices. You are able to write clean, efficient and maintainable code that follows the SOLID principles. You are also able to write unit tests and integration tests for your code. You are a team player and you are always willing to help your colleagues. You are also able to communicate effectively with your team and stakeholders.

# Building and testing

When editing a package go through the following steps:
1. Activate virtual environment.
2. Research sources
3. Edit code
4. Build and test locally
5. Commit changes

```pws1
.\build.ps1
$env:RUST_BACKTRACE="full"; .\build\Release\render\tests\raktr_render_tests.exe
$env:RUST_BACKTRACE="full"; .\build\Release\engine\tests\raktr_engine_test.exe
$env:RUST_BACKTRACE="full"; .\build\Release\editor\tests\raktr_editor_test.exe

$env:RUST_BACKTRACE="full"; .\build\Debug\render\tests\raktr_render_tests.exe
$env:RUST_BACKTRACE="full"; .\build\Debug\engine\tests\raktr_engine_test.exe
$env:RUST_BACKTRACE="full"; .\build\Debug\editor\tests\raktr_editor_test.exe
```

## Modules

All code must be organized into modules. Each module should have a clear responsibility and should not depend on other modules unless necessary. Modules should be named according to their functionality and should be placed in the appropriate namespace. For example, if you have a module that provides utility functions for string manipulation, you could name it `raktr::strings` and place it in the `raktr/utils/strings` directory.

1. Modules shall be implemented as header-only libraries. This means that all code should be placed in header files and there should be no source files. This allows for easier integration and reduces the need for separate compilation. OR
2. Modules shall be implemented as shared libraries. This means that the code should be placed in source files and compiled into a shared library that can be linked against by other modules. This allows for better encapsulation and can reduce compile times for large projects. It is important that if the shared libary depends on another library that this library is hidden behind an interface, specifically this module shall be implemented as a deep-module. This means that the implementation details of the module should be hidden from the users of the module, and only the public interface should be exposed. This allows for better encapsulation and can make it easier to change the implementation without affecting the users of the module.

## Classes

All classes must follow the SOLID principles. This means that they should have a single responsibility, should be open for extension but closed for modification, should depend on abstractions rather than concretions, and should not have any circular dependencies. Classes should also be designed to be easily testable and maintainable.

classes use spaceship operator for comparison and should not implement copy or move constructors or assignment operators unless necessary. If a class needs to be copyable or movable, it should explicitly default the copy and move constructors and assignment operators. This allows the compiler to generate the appropriate special member functions and ensures that the class behaves correctly when copied or moved.

```cpp
class MyClass
{
public:
    auto operator<=>(const MyClass&) const = default;
};
```

## Priority Guidelines

1. **Follow the C++ Core Guidelines** – https://isocpp.github.io/CppCoreGuidelines/CppCoreGuidelines
2. **Use Modern C++** – prefer C++23 features; avoid deprecated/legacy constructs.
3. **Write platform independent code** – abstract OS-specific logic; avoid `#ifdef` proliferation when possible.
4. **Write Clean, Readable Code** – prioritize clarity and maintainability over cleverness.
5. **Test-Driven Development** – write tests first; ensure all tests pass before merging.
6. **Document Public APIs** – use Doxygen-style comments for all public interfaces.
7. **Consistent Formatting** – use clang-format with project-specific style.
8. **Static Analysis** – ensure code is clang-tidy clean; fix warnings promptly.
9. **Prefer** - Use `gsl::not_null`, `std::span`, `std::optional`, or `std::expected` as appropriate.
10. **Use RAII** – manage resources with smart pointers and scope-bound objects; avoid raw `new`/`delete`.
11. **Exceptions** – do not use exceptions; use `std::expected` or `std::error_code` for error handling. 

---

## Technology Stack

| Area                | Tooling / Policy                             |
| ------------------- | -------------------------------------------- |
| **Language**        | C++23                                        |
| **Build System**    | CMake ≥ 4.0 + Ninja                          |
| **Platform**        | Windows, Linux, macOS                        |
| **Architecture**    | x86_64, arm64                                |
| **Package Manager** | Conan 2.x                                    |
| **Profiles**        | msvc_vs.profile, llvm_clang_vs.profile       |
| **Testing**         | GoogleTest / GoogleMock via CTest            |
| **Formatting**      | clang-format (LLVM-based)                    |
| **Linting**         | clang-tidy                                   |
| **Static Analysis** | ASan / TSan / UBSan                          |
| **Math**            | GLM                           |
| **CI**              | Github Actions                               |

## Naming Conventions

| Element                                  | Style                                   | Example                          |
| ---------------------------------------- | --------------------------------------- | -------------------------------- |
| **Filenames**                            | `snake_case`                            | `render_pipeline.cpp`            |
| **Namespaces**                           | `raktr::subsystem`                      | `raktr::render`                  |
| **Classes / Structs / Enums / Concepts** | PascalCase                              | `Renderer`, `TransformComponent` |
| **Functions / Methods**                  | snake_case                              | `create_mesh()`                  |
| **Variables**                            | snake_case                              | `frame_index`                    |
| **Private members**                      | snake_case                              | `_frame_index`                   |
| **Constants**                            | `k_constant_name` or `inline constexpr` | `k_max_frames`                   |
| **CMake targets**                        | snake_case                              | `raktr_core`                     |

## RAII + copy-and-swap Idiom

```cpp
#ifndef MY_ARRAY_H 
#define MY_ARRAY_H

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

#endif
```

## Memory Alignment

* Use `alignas(...)` for types requiring specific alignment.
* Use `std::aligned_alloc` for dynamic allocations needing alignment.
* Use static_asserts to verify alignment where applicable.

```cpp
struct alignas(16) AlignedVec4 {
    float x, y, z, w;
    auto operator<=>(const AlignedVec4&) const = default;
};
static_assert(alignof(AlignedVec4) == 16, "AlignedVec4 must be 16-byte aligned");
```

## Docstrings

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

## Testing

* **Framework:** GoogleTest + GoogleMock.
* **Style:** Triple-A (Arrange / Act / Assert).
* **Naming:** `Method_Scenario_Expected`.
* **Location:** `tests/<subsystem>/test_<feature>.cpp`.

Example:

```cpp
/*!
 * @brief Unit tests for Vec3 addition. 
 * @file test_math_vec3.cpp
 */
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

## Type Erasure

Use type erasure to provide a uniform interface for different types without exposing their implementation details. This is particularly useful for polymorphic behavior without inheritance. 

Consider two implementations:
* Owning class
* Non-owning (View)

```cpp
// Type Erasure Sample Code.
//
// Implementation of Klaus Iglberger's C++ Type Erasure Design Pattern.
//
// References:
// - Breaking Dependencies: Type Erasure - A Design Analysis,
//   by Klaus Iglberger, CppCon 2021.
//   - Video: https://www.youtube.com/watch?v=4eeESJQk-mw
//   - Slides: https://meetingcpp.com/mcpp/slides/2021/Type%20Erasure%20-%20A%20Design%20Analysis9268.pdf

#ifndef TYPE_ERASURE_SHAPE_H_
#define TYPE_ERASURE_SHAPE_H_

#include <concepts>
#include <iostream>
#include <memory>

// High Level Summary of the Design
// - `class Shape` and global functions (`serialize()`, `draw()`, etc.)
//   - The external client facing interface.
//   - Holds a pointer to `ShapeConcept` internally.
// - `class ShapeConcept`
//   - The internal interface of the Bridge Design Pattern.
//   - It is needed to hide the template parameter of `ShapeModel<T>`.
// - `class ShapeModel<T>`
//   - The templated implementation of `ShapeConcept`.
//   - Routes virtual functions to global functions.

// CAUTION: The following deleted functions serve 2 purposes:
// 1. Prevent the compiler from complaining about missing global functions
//    `serialize()` and `draw()` when seeing the using declarations in
//    `ShapeModel::serialize()` and `ShapeModel::draw()`. (It seems like a
//    compiler bug, as if the compiler did not see the `friend` definitions
//    within `class Shape`.
// 2. Prevent runaway recursions in case a concrete `Shape` such as `Circle`
//    does not define a `serialize(const Circle&)` or `draw(const Circle&)`
//    function. (Restricting class `Shape`'s template constructor parameter
//    type to the `IsShape` concept below also prevents runaway recurions.)
template <typename T>
void serialize(const T&) = delete;

template <typename T>
void draw(const T&) = delete;

#ifdef __clang__

// CAUTION: Workaround for clang.
// The following forward declarations of explicit specialization of
// `serialize()` and `draw()` prevent Clang from complaining about redefintion
// errors when seeing the definitions later.
class Shape;

template <>
void serialize(const Shape& shape);

template <>
void draw(const Shape& shape);

#endif  // __clang__

template <typename T>
concept IsShape = requires(T t) {
  serialize(t);
  draw(t);
  { std::declval<std::ostream&>() << t } -> std::same_as<std::ostream&>;
};

class Shape {
  // NOTE: Definition of the explicit specialization has to appear separately
  // later outside of class `Shape`, otherwise it results in error such as:
  //
  // ```
  // error: defining explicit specialization 'serialize<Shape>' in friend declaration
  // ```
  //
  // Reference: https://en.cppreference.com/w/cpp/language/friend
  friend void serialize<>(const Shape& shape);
  friend void draw<>(const Shape& shape);

  friend std::ostream& operator<<(std::ostream& os, const Shape& shape) {
    return os << *shape.pimpl_;
  }

  // The External Polymorphism Design Pattern
  class ShapeConcept {
   public:
    virtual ~ShapeConcept() {}
    virtual void serialize() const = 0;
    virtual void draw() const = 0;
    virtual void print(std::ostream& os) const = 0;

    // The Prototype Design Pattern
    virtual std::unique_ptr<ShapeConcept> clone() const = 0;

    friend std::ostream& operator<<(
        std::ostream& os, const ShapeConcept& shape) {
      shape.print(os);
      return os;
    }
  };

  template <typename T>
  class ShapeModel : public ShapeConcept {
    T object_;

   public:
    ShapeModel(const T& value)
        : object_{value} {
    }

    void serialize() const override {
      // CAUTION: The using declaration tells the compiler to look up the free
      // serialize() function rather than the member function.
      //
      // Reference: https://stackoverflow.com/a/32091297/4475887
      using ::serialize;

      serialize(object_);
    }

    void draw() const override {
      using ::draw;

      draw(object_);
    }

    void print(std::ostream& os) const override {
      os << object_;
    }

    // The Prototype Design Pattern
    std::unique_ptr<ShapeConcept> clone() const override {
      return std::make_unique<ShapeModel>(*this);
    }
  };

  // The Bridge Design Pattern
  std::unique_ptr<ShapeConcept> pimpl_;

 public:
  // A constructor template to create a bridge.
  template <IsShape T>
  Shape(const T& x)
      : pimpl_{new ShapeModel<T>(x)} {
  }

  Shape(const Shape& s)
      : pimpl_{s.pimpl_->clone()} {
  }

  Shape(Shape&& s)
      : pimpl_{std::move(s.pimpl_)} {
  }

  Shape& operator=(const Shape& s) {
    pimpl_ = s.pimpl_->clone();
    return *this;
  }

  Shape& operator=(Shape&& s) {
    pimpl_ = std::move(s.pimpl_);
    return *this;
  }
};

template <>
void serialize(const Shape& shape) {
  shape.pimpl_->serialize();
}

template <>
void draw(const Shape& shape) {
  shape.pimpl_->draw();
}

#endif  // TYPE_ERASURE_SHAPE_H_
```
