
find_package(GTest REQUIRED CONFIG)

include(GoogleTest)

file(GLOB_RECURSE raktr_render_test_sources CONFIGURE_DEPENDS "${CMAKE_CURRENT_SOURCE_DIR}/*.cpp")

add_executable(render_tests
    ${raktr_render_test_sources}
)

target_include_directories(render_tests
    PRIVATE
        ${CMAKE_CURRENT_SOURCE_DIR}/include)

target_link_libraries(render_tests
        PRIVATE
            raktr_render 
            GTest::gtest 
            GTest::gtest_main)

gtest_discover_tests(render_tests DISCOVERY_MODE PRE_TEST)