cmake_minimum_required(VERSION 3.20)
project(mt_data  VERSION 1.0.0 LANGUAGES CXX)


set(FILES_INSTALL info.sql3 filter.sql3 master_calibration.sql3)
install(FILES ${FILES_INSTALL} DESTINATION ${CMAKE_INSTALL_PREFIX}/data)


