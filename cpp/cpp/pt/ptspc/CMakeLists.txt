project(ptspc VERSION 1.0.0 LANGUAGES CXX)
add_executable(${PROJECT_NAME} main.cpp)

target_link_libraries(${PROJECT_NAME}
	PRIVATE ptspc_lib
	PRIVATE fftw3
	PRIVATE sqlite3
	PRIVATE raw_spectra
)

install(TARGETS ${PROJECT_NAME} DESTINATION ${CMAKE_INSTALL_PREFIX}/bin)