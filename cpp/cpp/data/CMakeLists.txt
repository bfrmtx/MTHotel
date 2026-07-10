
project(mt_data  VERSION 1.0.0 LANGUAGES CXX)


set(FILES_INSTALL info.db filter.db master_calibration.db)
install(FILES ${FILES_INSTALL} DESTINATION ${CMAKE_INSTALL_PREFIX}/data)


