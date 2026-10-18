project(pstplotter VERSION 1.0.0 LANGUAGES CXX)

set(CMAKE_AUTOUIC OFF)
set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)

include_directories(${CMAKE_CURRENT_SOURCE_DIR}/../qcustomplot)

set(PROJECT_SOURCES
    main.cpp
    pstplotter.cpp
    pst_spectra_plot.cpp
)

add_executable(${PROJECT_NAME} ${PROJECT_SOURCES})
target_link_libraries(${PROJECT_NAME} PRIVATE
    pst_spectra
    raw_spectra
    qt_survey_tree
)

include(${CMAKE_SOURCE_DIR}/cmake/qcustomplot_use.cmake)
include(${CMAKE_SOURCE_DIR}/cmake/qt6_app_end.cmake)
