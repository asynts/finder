cmake_minimum_required(VERSION 3.16.2)

project(finder LANGUAGES CXX)

add_executable(finder finder.cpp)

target_compile_features(finder PRIVATE cxx_std_20)

if(CMAKE_CXX_COMPILER_ID STREQUAL "GNU" OR CMAKE_CXX_COMPILER_ID STREQUAL "Clang")
    target_compile_options(finder PRIVATE "-Wall")
    target_compile_options(finder PRIVATE "-Wextra")
    target_compile_options(finder PRIVATE "-pedantic")
endif()
