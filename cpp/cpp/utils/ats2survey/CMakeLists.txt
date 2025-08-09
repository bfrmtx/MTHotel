project(ats2survey LANGUAGES CXX)


# for hybrid project - not used here
# Library
# add_library(ats2surv STATIC
#   ats2surv/ats2surv.cpp
#   ats2surv/ats2surv.hpp
# )
#

# Executable
add_executable(ats2survey main.cpp)
#target_link_libraries(ats2survey PRIVATE ats2surv)
target_link_libraries (${PROJECT_NAME}
    PRIVATE tinyxml2
    PRIVATE sqlite3
    PRIVATE read_cal_xml
)
# Install the ats2survey executable
install(TARGETS ats2survey
    RUNTIME DESTINATION ${CMAKE_INSTALL_PREFIX}/bin
)
