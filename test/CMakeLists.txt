if(CPPPROJECTS_TEST_CPP_UTILS)
    #add_executable(cpp_utils_tests cpp_utils/test_utils.cpp)
    #target_link_libraries(cpp_utils_tests PRIVATE cpp::utils GTest::gtest_main)
    #gtest_discover_tests(cpp_utils_tests PROPERTIES LABELS cpp_utils)
endif()

if(CPPPROJECTS_TEST_CSV)
    add_subdirectory(readerTests)
endif()

if(CPPPROJECTS_TEST_CSV_DIFF)
    add_subdirectory(diffTests)
endif()

if(CPPPROJECTS_TEST_JSON)
    add_subdirectory(jsonTests)
endif()
