cmake_minimum_required(VERSION 3.20)
project(math_vector  VERSION 1.0.0 LANGUAGES CXX)

set(CMAKE_INCLUDE_CURRENT_DIR ON)

include(${CMAKE_CURRENT_SOURCE_DIR}/../cmake/out_of_tree_build.cmake)
include_directories(${CMAKE_INSTALL_PREFIX}/include ${CMAKE_SOURCE_DIR}/include)


set(SOURCES math_vector.cpp)
set(HEADERS_INSTALL math_vector.h)

add_library(${PROJECT_NAME} SHARED ${SOURCES}  ${HEADERS_INSTALL})
target_include_directories(${PROJECT_NAME} PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})
target_link_directories(${PROJECT_NAME} PRIVATE ${CMAKE_INSTALL_PREFIX}/lib )


target_compile_definitions(${PROJECT_NAME} PUBLIC MATH_VECTOR_LIBRARY)

install(TARGETS ${PROJECT_NAME} DESTINATION ${CMAKE_INSTALL_PREFIX}/lib)
install(FILES ${HEADERS_INSTALL} DESTINATION ${CMAKE_INSTALL_PREFIX}/include)
