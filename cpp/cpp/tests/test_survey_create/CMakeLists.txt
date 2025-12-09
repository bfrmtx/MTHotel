project(test_survey_create LANGUAGES CXX)

set(CMAKE_INCLUDE_CURRENT_DIR ON)
include(${CMAKE_SOURCE_DIR}/cmake/rpath.cmake)
include(${CMAKE_SOURCE_DIR}/cmake/out_of_tree_build.cmake)
include_directories(${CMAKE_SOURCE_DIR}/cpp/pt/ptspc_lib)


# Add test source file(s)
add_executable(test_survey_create test_survey_create.cpp)

install(TARGETS ${PROJECT_NAME} DESTINATION ${CMAKE_INSTALL_PREFIX}/bin)
