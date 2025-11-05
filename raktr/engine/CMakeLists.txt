find_package(glm REQUIRED CONFIG)
find_package(spdlog REQUIRED CONFIG)
find_package(fmt REQUIRED CONFIG)

set(${PROJECT_NAME}_libtype STATIC)
if(BUILD_SHARED_LIBS)
  set(${PROJECT_NAME}_libtype SHARED)
endif()

file(GLOB_RECURSE ${PROJECT_NAME}_engine_sources CONFIGURE_DEPENDS "src/*.cpp")

add_library(${PROJECT_NAME}_engine 
            ${${PROJECT_NAME}_libtype} 
            ${${PROJECT_NAME}_engine_sources})

if(WIN32 AND BUILD_SHARED_LIBS)
  target_compile_definitions(
    ${PROJECT_NAME}_engine 
    PRIVATE 
      RAKTR_EXPORT)
endif() # Define __declspec() export

target_include_directories(
  ${PROJECT_NAME}_engine
  PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/public
  PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/src)

target_link_libraries(
  ${PROJECT_NAME}_engine
  PUBLIC
    ${PROJECT_NAME}_render
    glm::glm
    spdlog::spdlog
    fmt::fmt)

if(ENABLE_IWYU)
  add_iwyu_for_target(${PROJECT_NAME}_engine)
endif()

if(BUILD_TESTS)
  enable_testing()
  add_subdirectory(tests)
endif()
