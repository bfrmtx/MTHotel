cmake_minimum_required(VERSION 3.20)

project(atssfile VERSION 1.0.0 LANGUAGES CXX)

set(CMAKE_INCLUDE_CURRENT_DIR ON)

include(${CMAKE_CURRENT_SOURCE_DIR}/../../cmake/rpath.cmake)
include(${CMAKE_CURRENT_SOURCE_DIR}/../../cmake/out_of_tree_build.cmake)
include_directories(${CMAKE_SOURCE_DIR}/include ${CMAKE_INSTALL_PREFIX}/include)

add_library(atssfile SHARED
  atssfile.cpp
  atssfile.h
)

target_compile_definitions(atssfile PRIVATE ATSSFILE_LIBRARY)
