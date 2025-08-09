project(MTHotel)

# Add subdirectories
add_subdirectory(tinyxml2)
add_subdirectory(tinyxmlwriter)

if(NOT DEFINED HEADERS)
    file(GLOB HEADERS_INSTALL "*.h" "*.hpp")
endif()
install(FILES ${HEADERS_INSTALL} DESTINATION ${CMAKE_INSTALL_PREFIX}/include/xml)