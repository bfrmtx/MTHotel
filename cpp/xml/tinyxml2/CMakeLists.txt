cmake_minimum_required(VERSION 3.20)

project(tinyxml2 VERSION 1.0.0 LANGUAGES CXX)

set(CMAKE_INCLUDE_CURRENT_DIR ON)
include(${CMAKE_CURRENT_SOURCE_DIR}/../../cmake/out_of_tree_build.cmake)


set(SOURCES tinyxml2.cpp)
set(HEADERS_INSTALL tinyxml2.h)

add_library(${PROJECT_NAME} SHARED ${SOURCES}  ${HEADERS_INSTALL})

target_compile_definitions( ${PROJECT_NAME} PRIVATE TINYXML2_LIBRARY)
install(TARGETS ${PROJECT_NAME} DESTINATION ${CMAKE_INSTALL_PREFIX}/lib)
install(FILES ${HEADERS_INSTALL} DESTINATION ${CMAKE_INSTALL_PREFIX}/include)
