cmake_minimum_required(VERSION 3.20)

project(sqltest VERSION 1.0.0 LANGUAGES CXX)


include(${CMAKE_CURRENT_SOURCE_DIR}/../../cmake/rpath.cmake)
include(${CMAKE_CURRENT_SOURCE_DIR}/../../cmake/out_of_tree_build.cmake)
include_directories(${CMAKE_INSTALL_PREFIX}/include ${CMAKE_SOURCE_DIR}/include)

find_package(SQLite3 REQUIRED)


add_executable(${PROJECT_NAME} main.cpp)
target_link_libraries(${PROJECT_NAME} sqlite3)
