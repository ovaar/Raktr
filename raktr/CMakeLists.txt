# ------------------------------------------------------------------------------
# CMake basic options
# ------------------------------------------------------------------------------

cmake_minimum_required(VERSION 3.25)

# ------------------------------------------------------------------------------
# Project configuration options
# ------------------------------------------------------------------------------

# Project options
# cmake-format: off
option(BUILD_TESTS       "Setup target to build and run tests."              ON)
# cmake-format: on

# ------------------------------------------------------------------------------
# Project Declaration
# ------------------------------------------------------------------------------

project(raktr
    VERSION 0.1.0
    DESCRIPTION "Raktr - The next generation modern c++ 3D game engine — Where Worlds Take Shape"
    HOMEPAGE_URL "https://github.com/ovaar/Raktr"
    LANGUAGES CXX
)

# ------------------------------------------------------------------------------
# Project Options
# ------------------------------------------------------------------------------

set(CMAKE_EXPORT_COMPILE_COMMANDS ON) # Export compile_commands.json for tools like clangd
set(CMAKE_CXX_STANDARD 23)
set(CMAKE_CXX_STANDARD_REQUIRED ON)


# ------------------------------------------------------------------------------
# Include CMake modules
# ------------------------------------------------------------------------------

list(APPEND CMAKE_MODULE_PATH "${CMAKE_CURRENT_SOURCE_DIR}/cmake")
include(cmake/ccache.cmake)
include(cmake/iwyu.cmake)
include(cmake/timetrace.cmake)


# ------------------------------------------------------------------------------
# Global compile options
# ------------------------------------------------------------------------------

if(MSVC)
    message(STATUS "Visual Studio specific flags")
    add_compile_options(/W4 /WX /utf-8)
elseif(CMAKE_CXX_COMPILER_ID MATCHES "Clang")
    message(STATUS "Clang flags")
    add_compile_options(-Wall -Wextra -finput-charset=UTF-8 -fexec-charset=UTF-8)
elseif(CMAKE_CXX_COMPILER_ID MATCHES "GNU")
    message(STATUS "GCC flags")
    add_compile_options(-Wall -Wextra -finput-charset=UTF-8 -fexec-charset=UTF-8)
else()
    message(ERROR "Unsupported compiler: ${CMAKE_CXX_COMPILER_ID}")
endif()

if(WITH_ASAN)
    message(STATUS "AddressSanitizer enabled")
    add_compile_options(-fsanitize=address)
    add_link_options(-fsanitize=address)
endif()

add_subdirectory(raktr)
add_subdirectory(editor)
