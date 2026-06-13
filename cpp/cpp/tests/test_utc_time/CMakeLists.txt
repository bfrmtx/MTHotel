project(test_utc_time LANGUAGES CXX)

set(CMAKE_INCLUDE_CURRENT_DIR ON)
include(${CMAKE_SOURCE_DIR}/cmake/rpath.cmake)
include(${CMAKE_SOURCE_DIR}/cmake/out_of_tree_build.cmake)


# Add test source file(s)
add_executable(test_utc_time test_utc_time.cpp)

target_link_libraries(test_utc_time PRIVATE fftw3)

install(TARGETS ${PROJECT_NAME} DESTINATION ${CMAKE_INSTALL_PREFIX}/bin)
