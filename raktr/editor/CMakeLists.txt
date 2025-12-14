
find_package(mimalloc REQUIRED CONFIG)


# Collect source and header files
file(GLOB_RECURSE ${PROJECT_NAME}_editor_sources CONFIGURE_DEPENDS "src/*.cpp")

# Set up IDE source groups to maintain folder structure
source_group("Include Files" REGULAR_EXPRESSION public/*.*)
source_group("Source Files" REGULAR_EXPRESSION src/*.*)

add_executable(${PROJECT_NAME}_editor ${${PROJECT_NAME}_editor_sources})

target_include_directories(
  ${PROJECT_NAME}_editor 
    PRIVATE 
      ${CMAKE_CURRENT_SOURCE_DIR}/public
      ${CMAKE_CURRENT_SOURCE_DIR}/src)

target_link_libraries(
  ${PROJECT_NAME}_editor 
  PRIVATE 
    ${PROJECT_NAME}_render
    ${PROJECT_NAME}_engine 
    mimalloc-static)

if(BUILD_TESTS)
  enable_testing()
  add_subdirectory(tests)
endif()
