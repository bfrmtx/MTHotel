

project(chcal  VERSION 1.0.0 LANGUAGES CXX)

set(CMAKE_INCLUDE_CURRENT_DIR ON)

include(${CMAKE_CURRENT_SOURCE_DIR}/../../cmake/rpath.cmake)
include(${CMAKE_CURRENT_SOURCE_DIR}/../../cmake/out_of_tree_build.cmake)


set(SOURCES main.cpp)
add_executable(${PROJECT_NAME} ${SOURCES})
target_link_libraries (${PROJECT_NAME}
    PRIVATE read_cal
    PRIVATE tinyxml2
    PRIVATE sqlite3
)

IF(MINGW)
    message ( "WINDOWS CONSOLE MINGW" )
    target_link_options(${PROJECT_NAME} PRIVATE
    -mconsole
    -mwindows
)
ENDIF(MINGW)

install(TARGETS ${PROJECT_NAME} DESTINATION ${CMAKE_INSTALL_PREFIX}/bin)
