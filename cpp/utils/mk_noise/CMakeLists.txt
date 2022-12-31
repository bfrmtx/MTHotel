cmake_minimum_required(VERSION 3.20)

project(mk_noise  VERSION 1.0.0 LANGUAGES CXX)

set(CMAKE_INCLUDE_CURRENT_DIR ON)

find_package(Python3 COMPONENTS Interpreter Development NumPy REQUIRED)
include_directories(${Python_EXECUTABLE}  PRIVATE ${PYTHON_INCLUDE_DIRS})


include(${CMAKE_CURRENT_SOURCE_DIR}/../../cmake/rpath.cmake)
include(${CMAKE_CURRENT_SOURCE_DIR}/../../cmake/out_of_tree_build.cmake)
include_directories(${CMAKE_INSTALL_PREFIX}/include ${CMAKE_SOURCE_DIR}/mt/ac_spectra ${CMAKE_SOURCE_DIR}/mt/raw_spectra ${CMAKE_SOURCE_DIR}/include ${CMAKE_SOURCE_DIR}/math_vector  ${Python3_INCLUDE_DIRS} ${Python3_NumPy_INCLUDE_DIRS} )


include_directories(${PYTHON_INCLUDE_DIRS} ${NumPy_INCLUDE_DIRS})
find_package(SQLite3 REQUIRED)
set(SOURCES main.cpp)
add_executable(${PROJECT_NAME} ${SOURCES})

target_link_libraries (${PROJECT_NAME}
    PRIVATE fftw3
    PRIVATE sqlite3
    PUBLIC raw_spectra
    PUBLIC math_vector
    PRIVATE Python3::Python
    PRIVATE Python3::NumPy
)



install(TARGETS ${PROJECT_NAME} DESTINATION ${CMAKE_INSTALL_PREFIX}/bin)
