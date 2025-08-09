
project(read_cal_xml  VERSION 1.0.0 LANGUAGES CXX)

set(CMAKE_INCLUDE_CURRENT_DIR ON)




set(SOURCES read_cal_xml.cpp)
set(HEADERS_INSTALL read_cal_xml.h)

add_library(${PROJECT_NAME} SHARED ${SOURCES}  ${HEADERS_INSTALL})
target_include_directories(${PROJECT_NAME} PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})
target_link_directories(${PROJECT_NAME} PRIVATE ${CMAKE_INSTALL_PREFIX}/lib )

target_link_libraries (${PROJECT_NAME}
    PRIVATE tinyxml2
    PRIVATE sqlite3
)

target_compile_definitions(${PROJECT_NAME} PUBLIC READ_CAL_LIBRARY)

install(TARGETS ${PROJECT_NAME} DESTINATION ${CMAKE_INSTALL_PREFIX}/lib)
install(FILES ${HEADERS_INSTALL} DESTINATION ${CMAKE_INSTALL_PREFIX}/include)
