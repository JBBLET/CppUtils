find_package(GTest REQUIRED)
include(GoogleTest)

# Absolute path to the shared fixture so tests don't depend on the ctest CWD.
set(KITCHEN_SINK_CSV "${CMAKE_SOURCE_DIR}/test/resources/kitchen_sink.csv")

add_executable(reader_tests
    csvReaderTests.cpp
    csvReaderAwareTests.cpp
)

target_link_libraries(reader_tests
    PRIVATE cpp::csv
    PRIVATE GTest::gtest_main
)

# product_serde.hpp / sample_types.hpp (user-side deserialization) live beside
# these test sources, so quote-includes resolve without an extra include dir.
target_compile_definitions(reader_tests
    PRIVATE KITCHEN_SINK_CSV="${KITCHEN_SINK_CSV}"
)

target_compile_options(reader_tests PRIVATE -Wall -Wextra)

gtest_discover_tests(reader_tests)
