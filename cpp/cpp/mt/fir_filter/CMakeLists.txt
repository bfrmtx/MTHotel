
project(fir_filter VERSION 1.0.0 LANGUAGES CXX)

set(CMAKE_INCLUDE_CURRENT_DIR ON)


#top tree dir is CMAKE_SOURCE_DIR


# add fft - which is included in atss - link if needed
# include(${CMAKE_CURRENT_SOURCE_DIR}/../../cmake/fft.cmake)

set(SOURCES fir_filter.cpp)
set(HEADERS_INSTALL fir_filter.hpp)

add_library(${PROJECT_NAME} SHARED ${SOURCES}  ${HEADERS_INSTALL})


target_link_directories(${PROJECT_NAME} PRIVATE ${CMAKE_INSTALL_PREFIX}/lib )
target_link_libraries(${PROJECT_NAME} sqlite3 fftw3)


install(TARGETS ${PROJECT_NAME} DESTINATION ${CMAKE_INSTALL_PREFIX}/lib)
install(FILES ${HEADERS_INSTALL} DESTINATION ${CMAKE_INSTALL_PREFIX}/include)
