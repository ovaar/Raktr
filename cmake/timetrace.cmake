
include_guard()

# ENABLE_TIME_TRACE: Generates .json files with build timings viewable in a chrome://tracing/ flame graph.
option(ENABLE_TIME_TRACE "Enable build time tracing (.json files for chrome://tracing/)" OFF)
if(ENABLE_TIME_TRACE)
  if(("${CMAKE_CXX_COMPILER_ID}" STREQUAL "Clang" AND CMAKE_CXX_COMPILER_VERSION VERSION_GREATER_EQUAL 9.0))
    add_compile_options("$<$<COMPILE_LANGUAGE:CXX>:-ftime-trace>")
    add_link_options("$<$<LINK_LANGUAGE:CXX>:-ftime-trace>")
  else()
    message(FATAL_ERROR "ENABLE_TIME_TRACE may only be used with the clang compiler 9.0+ (need to set CMAKE_CXX_COMPILER)")
  endif()
endif()
