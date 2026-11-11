add_executable(json_tests
    jsonSerializeTest.cpp
)

target_link_libraries(json_tests
    PRIVATE cpp::json
    PRIVATE GTest::gtest_main
)

target_compile_options(json_tests PRIVATE -Wall -Wextra $<$<CXX_COMPILER_ID:GNU>:-freflection>)

gtest_discover_tests(json_tests)
