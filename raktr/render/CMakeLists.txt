find_package(glm REQUIRED CONFIG)
find_package(spdlog REQUIRED CONFIG)
find_package(fmt REQUIRED CONFIG)
find_package(wgpu-native REQUIRED CONFIG)
find_package(glfw3 REQUIRED CONFIG)

set(${PROJECT_NAME}_libtype STATIC)
if(BUILD_SHARED_LIBS)
  set(${PROJECT_NAME}_libtype SHARED)
endif()

file(GLOB_RECURSE ${PROJECT_NAME}_render_sources CONFIGURE_DEPENDS "src/*.cpp")
add_library(${PROJECT_NAME}_render ${${PROJECT_NAME}_libtype}
            ${${PROJECT_NAME}_render_sources})

if(WIN32 AND BUILD_SHARED_LIBS)
  target_compile_definitions(${PROJECT_NAME}_render PRIVATE RAKTR_EXPORT)
endif() # Define __declspec() export

target_include_directories(
  ${PROJECT_NAME}_render
  PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/public
  PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/src)

target_link_libraries(
  ${PROJECT_NAME}_render
  PUBLIC
    glm::glm
  PRIVATE 
    spdlog::spdlog
    fmt::fmt
    wgpu-native::wgpu-native
    glfw)
    
if(BUILD_TESTS)
  enable_testing()
  add_subdirectory(tests)
endif()
