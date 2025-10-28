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
endif()
