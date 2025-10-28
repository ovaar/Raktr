
find_package(mimalloc REQUIRED CONFIG)

add_executable(${PROJECT_NAME}_editor src/main.cpp)

target_include_directories(${PROJECT_NAME}_editor PUBLIC include)
target_link_libraries(
  ${PROJECT_NAME}_editor 
  PRIVATE 
    ${PROJECT_NAME}_render
    ${PROJECT_NAME}_engine 
    mimalloc-static)
