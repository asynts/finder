cmake_minimum_required(VERSION 3.16.2)

project(finder LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED YES)

add_compile_options(-Wall -Wextra -pedantic)

find_package(fmt)

add_executable(finder finder.cpp)
target_link_libraries(finder PRIVATE fmt::fmt)

install(TARGETS finder
    RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR})
