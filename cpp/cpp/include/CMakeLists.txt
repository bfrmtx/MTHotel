
project(mt_base VERSION 1.0.0 LANGUAGES CXX)

set(CMAKE_INCLUDE_CURRENT_DIR ON)
add_subdirectory(cal/read_cal) # contains a library top build

if(NOT DEFINED HEADERS)
    file(GLOB HEADERS_INSTALL "*.h" "*.hpp")
endif()
# message("incs" "${HEADERS_INSTALL}")
install(FILES ${HEADERS_INSTALL} DESTINATION ${CMAKE_INSTALL_PREFIX}/include)

if(NOT DEFINED FILES_INSTALL)
    file(GLOB FILES_INSTALL "*.md")
endif()

install(FILES ${FILES_INSTALL} DESTINATION ${CMAKE_INSTALL_PREFIX}/doc)

# install the COMPLETE nlohmann subdir
# install(DIRECTORY ats DESTINATION ${CMAKE_INSTALL_PREFIX}/include)
# install(DIRECTORY cal DESTINATION ${CMAKE_INSTALL_PREFIX}/include)
# install(DIRECTORY channel DESTINATION ${CMAKE_INSTALL_PREFIX}/include)
# install(DIRECTORY nlohmann DESTINATION ${CMAKE_INSTALL_PREFIX}/include)
# install(DIRECTORY thread-pool DESTINATION ${CMAKE_INSTALL_PREFIX}/include)



# file(GLOB files "*h")
#  foreach(file IN LISTS files)
#    message(STATUS " ------------- ${file} ------------- ")
#  endforeach()
