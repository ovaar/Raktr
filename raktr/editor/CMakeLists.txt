target_include_directories(${PROJECT_NAME}::editor PUBLIC include)

find_package(mimalloc REQUIRED CONFIG)

add_executable(${PROJECT_NAME}::editor src/main.cpp)
target_link_libraries(
  ${PROJECT_NAME}::editor PRIVATE ${PROJECT_NAME}::render
                                  ${PROJECT_NAME}::engine mimalloc-static)
