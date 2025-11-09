
find_package(GTest REQUIRED CONFIG)

include(GoogleTest)

file(GLOB_RECURSE raktr_render_test_sources CONFIGURE_DEPENDS "${CMAKE_CURRENT_SOURCE_DIR}/*.cpp")

add_executable(render_tests
    ${raktr_render_test_sources}
)

target_include_directories(render_tests
    PRIVATE
        ${CMAKE_CURRENT_SOURCE_DIR}
        ${CMAKE_CURRENT_SOURCE_DIR}/../include
        ${CMAKE_CURRENT_SOURCE_DIR}/../src)

target_link_libraries(render_tests
        PRIVATE
            raktr_render
            GTest::gtest 
            GTest::gtest_main)

if (WIN32)
  # copy runtime dlls to output directory
  add_custom_command(TARGET render_tests POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E copy_if_different
    $<TARGET_RUNTIME_DLLS:render_tests>
    $<TARGET_FILE_DIR:render_tests>
    COMMAND_EXPAND_LISTS)
endif()

gtest_discover_tests(render_tests DISCOVERY_MODE PRE_TEST)
