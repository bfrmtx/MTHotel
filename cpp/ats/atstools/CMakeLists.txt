

project(atstools VERSION 1.0.0 LANGUAGES CXX)


include(${CMAKE_CURRENT_SOURCE_DIR}/../../cmake/rpath.cmake)
include(${CMAKE_CURRENT_SOURCE_DIR}/../../cmake/out_of_tree_build.cmake)


# add fft - which is included in atss - link if needed
include(${CMAKE_CURRENT_SOURCE_DIR}/../../cmake/fft.cmake)


set(PROJECT_SOURCES main.cpp pt_cat.h pt_tojson.h adu06_fname.h chats.h)


add_executable(${PROJECT_NAME}
            ${PROJECT_SOURCES}
)

target_link_libraries(${PROJECT_NAME}
    PRIVATE tinyxml2
    PRIVATE read_cal
    PRIVATE raw_spectra
)

IF (WIN32)
set_target_properties(${PROJECT_NAME} PROPERTIES COMPILE_FLAGS -pthread LINK_FLAGS -lpthread)
ENDIF()


install(TARGETS ${PROJECT_NAME} DESTINATION ${CMAKE_INSTALL_PREFIX}/bin)

