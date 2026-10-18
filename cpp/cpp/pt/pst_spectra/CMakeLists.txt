project(pst_spectra LANGUAGES CXX)

add_library(pst_spectra STATIC pst_spectra.cpp)
target_include_directories(pst_spectra PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})
target_link_libraries(pst_spectra PUBLIC fftw3 math_vector sqlite3)
