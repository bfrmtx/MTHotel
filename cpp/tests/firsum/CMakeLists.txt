cmake_minimum_required(VERSION 3.20)

project(firsum VERSION 1.0.0 LANGUAGES CXX)


include(${CMAKE_CURRENT_SOURCE_DIR}/../../cmake/rpath.cmake)
include(${CMAKE_CURRENT_SOURCE_DIR}/../../cmake/out_of_tree_build.cmake)
include_directories(${CMAKE_SOURCE_DIR}/include ${CMAKE_INSTALL_PREFIX}/include)

find_package(SQLite3 REQUIRED)


add_executable(${PROJECT_NAME} main.cpp)
target_link_libraries(${PROJECT_NAME} sqlite3)
install(TARGETS ${PROJECT_NAME} DESTINATION ${CMAKE_INSTALL_PREFIX}/bin)
