# 
# CMake module to enable Include-What-You-Use (IWYU) analysis

# Usage:
# cmake -DENABLE_IWYU=ON -DCMAKE_CXX_COMPILER=clang++ -S . -B build
# cmake --build build --target iwyu_myapp
# 
# @example
# ```
# cmake_minimum_required(VERSION 3.15)
# include(cmake/iwyu.cmake)

# add_executable(myapp main.cpp foo.cpp)
# if(ENABLE_IWYU)
#   add_iwyu_for_target(myapp)
# endif()
# ```

include_guard()

# Option: ENABLE_IWYU
# When ON, CMake will run include-what-you-use during compilation
# and also enables the iwyu_tool.py helper for per-target analysis.
option(ENABLE_IWYU "Enable include-what-you-use analysis" OFF)

if(ENABLE_IWYU)
  if(CMAKE_CXX_COMPILER_ID STREQUAL "Clang")
    find_program(IWYU_EXE include-what-you-use)
    if(NOT IWYU_EXE)
      message(FATAL_ERROR "ENABLE_IWYU failed: could not find 'include-what-you-use' binary")
    endif()

    # Generate compile_commands.json — required for iwyu_tool.py
    set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

    # IWYU arguments (adjust mapping file path as needed)
    set(IWYU_ARGS
      -Xiwyu;--cxx17ns;
      --mapping_file=${PROJECT_SOURCE_DIR}/iwyu.imp
    )

    set(IWYU "${IWYU_EXE};${IWYU_ARGS}")
    set(CMAKE_C_INCLUDE_WHAT_YOU_USE "${IWYU}")
    set(CMAKE_CXX_INCLUDE_WHAT_YOU_USE "${IWYU}")

    message(STATUS "IWYU enabled: ${IWYU_EXE}")

    # --------------------------------------------------------------------------
    # Function: add_iwyu_for_target(<target>)
    # Adds a custom target to run iwyu_tool.py on only that target's sources.
    # --------------------------------------------------------------------------
    function(add_iwyu_for_target target)
      find_program(PYTHON_EXECUTABLE python3 REQUIRED)
      find_program(IWYU_TOOL_EXE iwyu_tool.py REQUIRED)

      if(NOT IWYU_TOOL_EXE)
        message(WARNING "iwyu_tool.py not found; skipping IWYU target for ${target}")
        return()
      endif()

      # Path to compile_commands.json (needed by iwyu_tool.py)
      set(COMPILE_COMMANDS_PATH ${CMAKE_BINARY_DIR}/compile_commands.json)
      if(NOT EXISTS ${COMPILE_COMMANDS_PATH})
        message(WARNING "compile_commands.json not found; did you enable CMAKE_EXPORT_COMPILE_COMMANDS?")
      endif()

      # Get target's sources (so IWYU runs only on this target)
      get_target_property(SRCS ${target} SOURCES)
      if(NOT SRCS)
        message(WARNING "Target ${target} has no SOURCES property — skipping IWYU")
        return()
      endif()

      set(IWYU_LOG ${CMAKE_BINARY_DIR}/${target}_iwyu.log)

      add_custom_target(
        iwyu_${target}
        COMMAND ${PYTHON_EXECUTABLE} ${IWYU_TOOL_EXE}
                -p ${CMAKE_BINARY_DIR}
                ${SRCS}
                -- --mapping_file=${PROJECT_SOURCE_DIR}/iwyu.imp
                > ${IWYU_LOG} 2>&1
        WORKING_DIRECTORY ${PROJECT_SOURCE_DIR}
        COMMENT "Running IWYU analysis for target ${target}..."
        VERBATIM
      )

      message(STATUS "Added IWYU target: iwyu_${target}")
    endfunction()

  else()
    message(FATAL_ERROR "ENABLE_IWYU may only be used with the Clang compiler")
  endif()
endif()
