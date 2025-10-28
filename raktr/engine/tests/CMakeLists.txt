find_package(GTest REQUIRED CONFIG)

include(GoogleTest)

file(GLOB_RECURSE raktr_engine_test_sources CONFIGURE_DEPENDS "${CMAKE_CURRENT_SOURCE_DIR}/*.cpp")

add_executable(raktr_engine_test 
    ${raktr_engine_test_sources}
)

target_include_directories(raktr_engine_test
    PRIVATE 
        ${CMAKE_CURRENT_SOURCE_DIR}/include)

target_link_libraries(raktr_engine_test
    PRIVATE 
        raktr_engine
        GTest::gtest 
        GTest::gtest_main
)

gtest_discover_tests(raktr_engine_test DISCOVERY_MODE PRE_TEST)