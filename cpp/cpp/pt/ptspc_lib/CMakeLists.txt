project(ptspc_lib VERSION 1.0.0 LANGUAGES CXX)

set(CMAKE_INCLUDE_CURRENT_DIR ON)
include(${CMAKE_SOURCE_DIR}/cmake/fft.cmake)
include_directories(${CMAKE_SOURCE_DIR}/cpp/mt/raw_spectra)



set(SOURCES ptspc_lib.cpp)
set(HEADERS_INSTALL ptspc_lib.hpp)

add_library(${PROJECT_NAME} SHARED ${SOURCES}  ${HEADERS_INSTALL})


target_link_libraries(${PROJECT_NAME}
    PRIVATE fftw3
    PRIVATE sqlite3
    PRIVATE raw_spectra
)

# Make headers available to targets that link against this library
target_include_directories(${PROJECT_NAME} 
    PUBLIC $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}>
    PUBLIC $<INSTALL_INTERFACE:include>
)

#IF (WIN32)
#set_target_properties(${PROJECT_NAME} PROPERTIES COMPILE_FLAGS -pthread LINK_FLAGS -lpthread)
#ENDIF()


target_compile_definitions( ${PROJECT_NAME} PRIVATE PTSPC_LIB_LIBRARY)
install(TARGETS ${PROJECT_NAME} DESTINATION ${CMAKE_INSTALL_PREFIX}/lib)
install(FILES ${HEADERS_INSTALL} DESTINATION ${CMAKE_INSTALL_PREFIX}/include)

