include_guard()

option(USE_CCACHE "Enable caching of compiled artifacts using ccache" ON)

find_program(CCACHE_PROGRAM ccache)
if(USE_CCACHE)
  if(NOT CCACHE_PROGRAM)
    message(FATAL_ERROR "USE_CCACHE is ON but ccache was not found!")
  endif()
  message(STATUS "Using ccache for compilation.")
  set(CMAKE_C_COMPILER_LAUNCHER ${CCACHE_PROGRAM})
  set(CMAKE_CXX_COMPILER_LAUNCHER ${CCACHE_PROGRAM})
  if (MSVC)
    # ccache is incompatible with `/Zi` or `/ZI` and needs `/Z7`, see
    # - https://discourse.cmake.org/t/early-experiences-with-msvc-debug-information-format-and-cmp0141/6859
    # - https://learn.microsoft.com/en-us/cpp/build/reference/z7-zi-zi-debug-information-format?view=msvc-170
    # - https://cmake.org/cmake/help/latest/variable/CMAKE_MSVC_DEBUG_INFORMATION_FORMAT.html
    set(CMAKE_MSVC_DEBUG_INFORMATION_FORMAT "Embedded")
  endif()
endif()
