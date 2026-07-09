cmake_minimum_required(VERSION 3.15)

project(CPPUtils LANGUAGES CXX VERSION 0.1)

set(CMAKE_CXX_STANDARD 26)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)


# Default to a debug-info build so std::stacktrace can resolve names + lines.
if(NOT CMAKE_BUILD_TYPE)
    set(CMAKE_BUILD_TYPE RelWithDebInfo)
endif()


add_library(CPPUtils INTERFACE)
target_include_directories(CPPUtils INTERFACE
  $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>)
target_compile_features(CPPUtils INTERFACE cxx_std_26)
# Need libstdc++ for std::stacktrace in Exception
# separate library in GCC
target_link_libraries(CPPUtils INTERFACE stdc++exp)   # stacktrace backend
add_library(cpp::utils ALIAS CPPUtils)

add_library(csv INTERFACE)
target_include_directories(csv INTERFACE
  $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>)
target_link_libraries(csv INTERFACE cpp::utils)        # csv needs core
add_library(cpp::csv ALIAS csv)

add_executable(csvutils_demo src/main.cpp)
target_link_libraries(csvutils_demo PRIVATE cpp::csv)
target_include_directories(csvutils_demo
    PUBLIC
        ${CMAKE_CURRENT_SOURCE_DIR}/include
)
target_compile_options(csvutils_demo
  PRIVATE -Wall
  PRIVATE -Wextra
)
