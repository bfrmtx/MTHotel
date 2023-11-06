 
cmake_minimum_required(VERSION 3.20)

project(pt2surv  VERSION 1.0.0 LANGUAGES CXX)

include(${CMAKE_CURRENT_SOURCE_DIR}/../../cmake/rpath.cmake)
include(${CMAKE_CURRENT_SOURCE_DIR}/../../cmake/out_of_tree_build.cmake)
include_directories(${CMAKE_SOURCE_DIR}/include ${CMAKE_INSTALL_PREFIX}/include)



set(PROJECT_SOURCES main.cpp)

add_executable(${PROJECT_NAME}
            ${PROJECT_SOURCES}
)

target_link_libraries(${PROJECT_NAME}
    PRIVATE tinyxml2
    PRIVATE read_cal
)

IF (WIN32)
set_target_properties(${PROJECT_NAME} PROPERTIES COMPILE_FLAGS -pthread LINK_FLAGS -lpthread)
ENDIF()


install(TARGETS ${PROJECT_NAME} DESTINATION ${CMAKE_INSTALL_PREFIX}/bin)