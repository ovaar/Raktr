

include(GoogleTest)

file(GLOB_RECURSE raktr_engine_test_sources CONFIGURE_DEPENDS "tests/*.cpp")

add_executable(raktr_engine_test 
    ${raktr_engine_test_sources}
)

target_include_directories(raktr_engine_test
    PRIVATE 
        ${CMAKE_SOURCE_DIR}
)
target_link_libraries(raktr_engine_test
    PRIVATE 
        raktr::engine
        GTest::gtest 
        GTest::gtest_main
)

gtest_discover_tests(raktr_engine_test
  DISCOVERY_MODE PRE_TEST)