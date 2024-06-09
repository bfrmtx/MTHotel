cmake_minimum_required(VERSION 3.20)

project(fftw_length_basics  VERSION 1.0.0 LANGUAGES CXX)

set(CMAKE_INCLUDE_CURRENT_DIR ON)

include(${CMAKE_CURRENT_SOURCE_DIR}/../../cmake/rpath.cmake)
include(${CMAKE_CURRENT_SOURCE_DIR}/../../cmake/out_of_tree_build.cmake)
include_directories(${CMAKE_INSTALL_PREFIX}/include ${CMAKE_SOURCE_DIR}/mt/raw_spectra ${CMAKE_SOURCE_DIR}/include ${CMAKE_SOURCE_DIR}/math_vector)


find_package(SQLite3 REQUIRED)
set(SOURCES main.cpp)
add_executable(${PROJECT_NAME} ${SOURCES})

target_link_libraries (${PROJECT_NAME}
    PRIVATE fftw3
    PRIVATE sqlite3
    PUBLIC raw_spectra
    PUBLIC math_vector
)

install(TARGETS ${PROJECT_NAME} DESTINATION ${CMAKE_INSTALL_PREFIX}/bin)
