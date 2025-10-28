
include_guard()

# ENABLE_IWYU: Generates .iwyu files suitable for input into include-what-you-use's fix_includes.py tool.
option(ENABLE_IWYU "Enable include-what-you-use advice (.iwyu files for fix_includes.py)" OFF)
if(ENABLE_IWYU)
 if("${CMAKE_CXX_COMPILER_ID}" STREQUAL "Clang")
   find_program(IWYU_EXE include-what-you-use)
   if(IWYU_EXE)
    set(IWYU "${IWYU_EXE};-Xiwyu;--cxx17ns;--mapping_file=${CMAKE_CURRENT_LIST_DIR}/iwyu.imp")
    set(CMAKE_C_INCLUDE_WHAT_YOU_USE "${IWYU}")
    set(CMAKE_CXX_INCLUDE_WHAT_YOU_USE "${IWYU}")
   else()
     message(FATAL_ERROR "ENABLE_IWYU failed to find the include-what-you-use binary")
   endif()
 else()
   message(FATAL_ERROR "ENABLE_IWYU may only be used with the clang compiler (need to set CMAKE_CXX_COMPILER)")
  endif()
endif()
