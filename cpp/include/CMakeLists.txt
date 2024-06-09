cmake_minimum_required(VERSION 3.20)
project(mt_base VERSION 1.0.0 LANGUAGES CXX)

set(CMAKE_INCLUDE_CURRENT_DIR ON)
# include_directories(${CMAKE_SOURCE_DIR}/include ${CMAKE_SOURCE_DIR}/xml/tinyxml2 ${CMAKE_SOURCE_DIR}/tinyxmlwriter )


if(NOT DEFINED HEADERS)
    file(GLOB HEADERS_INSTALL "*.h")
endif()
message("incs" "${HEADERS_INSTALL}")
install(FILES ${HEADERS_INSTALL} DESTINATION ${CMAKE_INSTALL_PREFIX}/include)

if(NOT DEFINED FILES_INSTALL)
    file(GLOB FILES_INSTALL "*.md")
endif()

install(FILES ${FILES_INSTALL} DESTINATION ${CMAKE_INSTALL_PREFIX}/doc)

# file(GLOB files "*h")
#  foreach(file IN LISTS files)
#    message(STATUS " ------------- ${file} ------------- ")
#  endforeach()
