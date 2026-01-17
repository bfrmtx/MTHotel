project(test_prz_sum LANGUAGES CXX)

set(CMAKE_INCLUDE_CURRENT_DIR ON)
include(${CMAKE_SOURCE_DIR}/cmake/rpath.cmake)
include(${CMAKE_SOURCE_DIR}/cmake/out_of_tree_build.cmake)


# Add test source file(s)
add_executable(test_prz_sum test_prz_sum.cpp)

install(TARGETS ${PROJECT_NAME} DESTINATION ${CMAKE_INSTALL_PREFIX}/bin)
