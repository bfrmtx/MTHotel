project(test_pst_spectra LANGUAGES CXX)

add_executable(test_pst_spectra test_pst_spectra.cpp)
target_link_libraries(test_pst_spectra PRIVATE pst_spectra)
