cmake_minimum_required(VERSION 3.20)
project(mk_calibration VERSION 1.0.0 LANGUAGES CXX)

set(CMAKE_INCLUDE_CURRENT_DIR ON)
include(${CMAKE_CURRENT_SOURCE_DIR}/../../cmake/out_of_tree_build.cmake)
include_directories(${CMAKE_INSTALL_PREFIX}/include ${CMAKE_SOURCE_DIR}/include)


set(SOURCES mk_calibration.cpp)
set(HEADERS_INSTALL mk_calibration.h synthetic.h)

add_library(${PROJECT_NAME} SHARED ${SOURCES}  ${HEADERS_INSTALL})
target_include_directories(${PROJECT_NAME} PRIVATE ${CMAKE_CURRENT_SOURCE_DIR})


target_link_libraries (${PROJECT_NAME} PRIVATE read_cal)
target_compile_definitions(${PROJECT_NAME} PRIVATE MK_CALIBRATION_LIBRARY)

install(TARGETS ${PROJECT_NAME} DESTINATION ${CMAKE_INSTALL_PREFIX}/lib)
install(FILES ${HEADERS_INSTALL} DESTINATION ${CMAKE_INSTALL_PREFIX}/include)
