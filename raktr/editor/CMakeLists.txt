
find_package(mimalloc REQUIRED CONFIG)

file(GLOB_RECURSE ${PROJECT_NAME}_editor_sources CONFIGURE_DEPENDS "src/*.cpp")

add_executable(${PROJECT_NAME}_editor ${${PROJECT_NAME}_editor_sources})

target_include_directories(
  ${PROJECT_NAME}_editor 
    PRIVATE include)

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
