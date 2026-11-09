cmake_minimum_required(VERSION 3.25) #4.4 verified

project(CPPUtils LANGUAGES CXX VERSION 0.1)
if(NOT CMAKE_CXX_COMPILER_ID STREQUAL "GNU" OR CMAKE_CXX_COMPILER_VERSION VERSION_LESS 16)
    message(FATAL_ERROR "GCC >= 16 required (-freflection). Got ${CMAKE_CXX_COMPILER_ID} ${CMAKE_CXX_COMPILER_VERSION}")
endif()
include(CMakeDependentOption)
include(CTest)

set(CMAKE_CXX_STANDARD 26)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

# Default to a debug-info build so std::stacktrace can resolve names + lines.
if(NOT CMAKE_BUILD_TYPE)
    set(CMAKE_BUILD_TYPE RelWithDebInfo)
endif()

# Separation into subparts proejcts to enable testing per part
set(CPPPROJECTS_PARTS cpp_utils csv csv_diff json)

# One option per part, only available when BUILD_TESTING is ON
foreach(part IN LISTS CPPPROJECTS_PARTS)
    string(TOUPPER "${part}" PART)
    cmake_dependent_option(CPPPROJECTS_TEST_${PART}
    "Build tests for ${part}" ON "BUILD_TESTING" OFF)
endforeach()

# Dependency
if(BUILD_TESTING)
    find_package(GTest REQUIRED)
    include(GoogleTest)
endif()

## Parts Definition
# Basic Utils
add_library(CPPUtils INTERFACE)
target_include_directories(CPPUtils INTERFACE
  $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>)
target_compile_features(CPPUtils INTERFACE cxx_std_26)
# Need libstdc++ for std::stacktrace in Exception
# separate library in GCC
target_link_libraries(CPPUtils INTERFACE stdc++exp)   # stacktrace backend
add_library(cpp::utils ALIAS CPPUtils)

# CSV Readers
add_library(csv INTERFACE)
target_include_directories(csv INTERFACE
  $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>)
target_link_libraries(csv INTERFACE cpp::utils)        # csv needs core
add_library(cpp::csv ALIAS csv)

# CSV Diffs
add_library(csvdiff INTERFACE)
target_include_directories(csvdiff INTERFACE
  $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>)
target_link_libraries(csvdiff INTERFACE cpp::utils csv)   # diff needs core + csv reader
add_library(cpp::csv::diff ALIAS csvdiff)

## JSON serializer (header-only, C++26 reflection)
add_library(json INTERFACE)
target_include_directories(json INTERFACE
  $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>)
target_compile_features(json INTERFACE cxx_std_26)
target_compile_options(json INTERFACE
  $<$<CXX_COMPILER_ID:GNU>:-freflection>)
add_library(cpp::json ALIAS json)


if(BUILD_TESTING AND PROJECT_IS_TOP_LEVEL)
    find_package(GTest REQUIRED)
    include(GoogleTest)
    add_subdirectory(test)
endif()

if(PROJECT_IS_TOP_LEVEL)
    add_executable(main src/main.cpp)
    target_link_libraries(main PRIVATE cpp::csv cpp::json)
    target_include_directories(main
        PUBLIC
            ${CMAKE_CURRENT_SOURCE_DIR}/include
    )
    target_compile_options(main
      PRIVATE -Wall
      PRIVATE -Wextra
    )
endif()
