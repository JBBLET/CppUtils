find_package(GTest REQUIRED)
include(GoogleTest)

add_executable(diff_tests csvDifftests.cpp)

target_link_libraries(diff_tests
    PRIVATE cpp::csv::diff
    PRIVATE GTest::gtest_main
)

# Absolute path to the committed fixtures so tests don't depend on the ctest CWD.
target_compile_definitions(diff_tests
    PRIVATE DIFF_FIXTURE_DIR="${CMAKE_SOURCE_DIR}/test/diffTests"
)

target_compile_options(diff_tests PRIVATE -Wall -Wextra)

gtest_discover_tests(diff_tests)
