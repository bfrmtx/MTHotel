cmake_minimum_required(VERSION 3.20)
project(sqlite3  VERSION 3.38.2)

set(CMAKE_INCLUDE_CURRENT_DIR ON)

set(SOURCES sqlite3.c)
set(HEADERS_INSTALL sqlite3.h sqlite3ext.h)

add_library(${PROJECT_NAME} SHARED ${SOURCES} shell.c ${HEADERS_INSTALL})
target_include_directories(${PROJECT_NAME} PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})


add_executable(sqlite ${SOURCES} shell.c ${HEADERS_INSTALL})
#set_target_properties(sqlite PROPERTIES OUTPUT_NAME sqlite3)

add_definitions(-DSQLITE_ENABLE_RTREE)
add_definitions(-DSQLITE_ENABLE_FTS4)
add_definitions(-DSQLITE_ENABLE_FTS5)
add_definitions(-DSQLITE_ENABLE_JSON1)
add_definitions(-DSQLITE_ENABLE_RBU)
add_definitions(-DSQLITE_ENABLE_STAT4)

# Uncomment this for single-threaded variant (faster)
#add_definitions(-DSQLITE_THREADSAFE=0)

#if(WIN32)
  #add_custom_command(TARGET sqlite POST_BUILD
    #COMMAND ${CMAKE_COMMAND} -E copy $<TARGET_FILE:sqlite> ${CMAKE_BINARY_DIR}/sqlite3.exe
    #DEPENDS sqlite
  #)
  #install(FILES sqlite3.exe DESTINATION bin)
#else()
  #include(FindThreads)
  #target_link_libraries(sqlite m ${CMAKE_THREAD_LIBS_INIT} ${CMAKE_DL_LIBS})
  #install(TARGETS sqlite RUNTIME DESTINATION bin)
#endif()

install(TARGETS sqlite DESTINATION ${CMAKE_INSTALL_PREFIX}/bin)
install(TARGETS ${PROJECT_NAME} DESTINATION ${CMAKE_INSTALL_PREFIX}/lib)
install(FILES ${HEADERS_INSTALL} DESTINATION ${CMAKE_INSTALL_PREFIX}/include)
