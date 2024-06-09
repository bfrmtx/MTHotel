cmake_minimum_required(VERSION 3.20)
project(raw_spectra VERSION 1.0.0 LANGUAGES CXX)

set(CMAKE_INCLUDE_CURRENT_DIR ON)

include(${CMAKE_CURRENT_SOURCE_DIR}/../../cmake/out_of_tree_build.cmake)
#top tree dir is CMAKE_SOURCE_DIR
include_directories(${CMAKE_SOURCE_DIR}/include ${CMAKE_SOURCE_DIR}/math_vector)

include(${CMAKE_CURRENT_SOURCE_DIR}/../../cmake/fft.cmake)

set(SOURCES raw_spectra.cpp)
set(HEADERS_INSTALL raw_spectra.h)

add_library(${PROJECT_NAME} SHARED ${SOURCES}  ${HEADERS_INSTALL})


target_link_directories(${PROJECT_NAME} PRIVATE ${CMAKE_INSTALL_PREFIX}/lib )
target_link_libraries(${PROJECT_NAME} sqlite3 fftw3 math_vector)


install(TARGETS ${PROJECT_NAME} DESTINATION ${CMAKE_INSTALL_PREFIX}/lib)
install(FILES ${HEADERS_INSTALL} DESTINATION ${CMAKE_INSTALL_PREFIX}/include)
