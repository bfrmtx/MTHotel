project(test_channel_osr LANGUAGES CXX)

add_executable(test_channel_osr test_channel_osr.cpp)
target_link_libraries(test_channel_osr PRIVATE tinyxml2 sqlite3 fftw3 read_cal_xml)
