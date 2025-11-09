find_package(GTest REQUIRED CONFIG)

include(GoogleTest)

file(GLOB_RECURSE raktr_editor_test_sources CONFIGURE_DEPENDS "${CMAKE_CURRENT_SOURCE_DIR}/*.cpp")

add_executable(raktr_editor_test 
    ${raktr_editor_test_sources}
)

target_include_directories(raktr_editor_test
    PRIVATE 
        ${CMAKE_CURRENT_SOURCE_DIR}
        ${CMAKE_CURRENT_SOURCE_DIR}/../include
        ${CMAKE_CURRENT_SOURCE_DIR}/../src)

target_link_libraries(raktr_editor_test
    PRIVATE 
        raktr_engine
        raktr_render
        GTest::gtest 
        GTest::gtest_main
)

gtest_discover_tests(raktr_editor_test DISCOVERY_MODE PRE_TEST)