cmake_minimum_required(VERSION 3.15)

project(CPPUtils LANGUAGES CXX VERSION 0.1)

set(CMAKE_CXX_STANDARD 26)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)


# Default to a debug-info build so std::stacktrace can resolve names + lines.
if(NOT CMAKE_BUILD_TYPE)
    set(CMAKE_BUILD_TYPE RelWithDebInfo)
endif()

# Basic Utils
add_library(CPPUtils INTERFACE)
target_include_directories(CPPUtils INTERFACE
  $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>)
target_compile_features(CPPUtils INTERFACE cxx_std_26)
# Need libstdc++ for std::stacktrace in Exception
# separate library in GCC
target_link_libraries(CPPUtils INTERFACE stdc++exp)   # stacktrace backend
add_library(cpp::utils ALIAS CPPUtils)

# CSV Raders
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

# Standalone-only targets (playground executable + tests). Skipped when CPPUtils is
# pulled into another project via FetchContent/add_subdirectory.
if(CMAKE_SOURCE_DIR STREQUAL CMAKE_CURRENT_SOURCE_DIR)
    # Main for Testing
    add_executable(main src/main.cpp)
    target_link_libraries(main PRIVATE cpp::csv)
    target_include_directories(main
        PUBLIC
            ${CMAKE_CURRENT_SOURCE_DIR}/include
    )
    target_compile_options(main
      PRIVATE -Wall
      PRIVATE -Wextra
    )

    # Testing
    enable_testing()
    add_subdirectory(test/readerTests)
    add_subdirectory(test/diffTests)
endif()
