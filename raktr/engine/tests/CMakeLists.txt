find_package(GTest REQUIRED CONFIG)

include(GoogleTest)

file(GLOB_RECURSE raktr_engine_test_sources CONFIGURE_DEPENDS "${CMAKE_CURRENT_SOURCE_DIR}/*.cpp")

add_executable(raktr_engine_test 
    ${raktr_engine_test_sources}
)

target_include_directories(raktr_engine_test
    PRIVATE 
        ${CMAKE_CURRENT_SOURCE_DIR}
        ${CMAKE_CURRENT_SOURCE_DIR}/../include
        ${CMAKE_CURRENT_SOURCE_DIR}/../src)

target_link_libraries(raktr_engine_test
    PRIVATE 
        raktr_engine
        GTest::gtest 
        GTest::gtest_main
)

if (WIN32)
  # copy runtime dlls to output directory
  add_custom_command(TARGET raktr_engine_test POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E copy_if_different
    $<TARGET_RUNTIME_DLLS:raktr_engine_test>
    $<TARGET_FILE_DIR:raktr_engine_test>
    COMMAND_EXPAND_LISTS)
endif()

gtest_discover_tests(raktr_engine_test DISCOVERY_MODE PRE_TEST)