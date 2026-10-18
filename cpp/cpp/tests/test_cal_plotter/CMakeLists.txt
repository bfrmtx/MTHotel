project(test_cal_plotter LANGUAGES CXX)

add_executable(test_cal_plotter
    test_cal_plotter.cpp
    ${QT6_PROJECT_DIR}/cal-plot/cal_plotter.cpp
    ${QT6_PROJECT_DIR}/cal-plot/cal_plotter.h
)
set_target_properties(test_cal_plotter PROPERTIES AUTOMOC ON)
target_include_directories(test_cal_plotter PRIVATE
    ${QT6_PROJECT_DIR}/cal-plot
    ${QT6_PROJECT_DIR}/qcustomplot
)
target_compile_definitions(test_cal_plotter PRIVATE MTHOTEL_TEST_SOURCE_DIR="${CMAKE_SOURCE_DIR}")
target_link_libraries(test_cal_plotter PRIVATE Qt6::Widgets qcustomplot read_cal_xml)
