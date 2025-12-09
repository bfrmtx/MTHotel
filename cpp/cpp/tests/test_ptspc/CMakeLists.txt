project(test_ptspc LANGUAGES CXX)

set(CMAKE_INCLUDE_CURRENT_DIR ON)
include(${CMAKE_SOURCE_DIR}/cmake/rpath.cmake)
include(${CMAKE_SOURCE_DIR}/cmake/out_of_tree_build.cmake)

# Add test source file(s)
add_executable(test_ptspc test_ptspc.cpp)

# Add include directories for headers (modern approach)
target_include_directories(test_ptspc PRIVATE
    ${CMAKE_SOURCE_DIR}/cpp/cpp/include
    ${CMAKE_SOURCE_DIR}/cpp/cpp/include/thread-pool
    ${CMAKE_SOURCE_DIR}/cpp/cpp/include/pt/ptspc_lib
)

# Link against required libraries
target_link_libraries(test_ptspc 
    PRIVATE ptspc_lib
    PRIVATE sqlite3
    PRIVATE fftw3
    PRIVATE raw_spectra
)

install(TARGETS ${PROJECT_NAME} DESTINATION ${CMAKE_INSTALL_PREFIX}/bin)
