#
# other sub ordinate CMakeLists must not contain
# repeated settings from here 
#
# # don't forget in QtCreator to set CMAKE_INSTALL_PREFIX in the GUI under build
# cmake standard
cmake_minimum_required(VERSION 3.20)
project(MTHotel LANGUAGES CXX)
#
# CXX standard; that may be repeated as -std=c++20
set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
#
#
############# Require out-of-source builds
# on synced drives you do NOT want to sync the build! only the source
file(TO_CMAKE_PATH "${PROJECT_BINARY_DIR}/CMakeLists.txt" LOC_PATH)
if(EXISTS "${LOC_PATH}")
    message(FATAL_ERROR "You can not build in a source directory (or any directory with a CMakeLists.txt file). Please make a build subdirectory. Feel free to remove CMakeCache.txt and CMakeFiles.")
endif()
#############

message("compiler used / detected " ${CMAKE_CXX_COMPILER_ID})
#
# set CMAKE_BUILD_TYPE RELEASE for CLANG
IF(CMAKE_CXX_COMPILER_ID MATCHES "Clang" AND NOT WIN32)
# using regular Clang or AppleClang
    message ("UNIX LINUX Apple Clang build")
    # on Linux and MacOS we can build shared libs - for all not explicitly marked as STATIC
    option(BUILD_SHARED_LIBS "Build using shared libraries" ON)
    IF(CMAKE_BUILD_TYPE MATCHES DEBUG)
        message ("debug build ---------------------")
        add_compile_options(-std=c++20 -march=native -pthread -ggdb3 -O0 -Wsign-compare -Wall -Wextra -pedantic  -Wno-unused-parameter)
        set(CMAKE_VERBOSE_MAKEFILE ON)
    ENDIF(CMAKE_BUILD_TYPE MATCHES DEBUG)
    IF(CMAKE_BUILD_TYPE MATCHES RELEASE)
        message ("release build ---------------------")
        # no sign compare is for Qt, containers are not strictly C++ compliant
        add_compile_options(-std=c++20 -march=native -pthread -O3 -Wno-unused-parameter -Wno-sign-compare)
    ENDIF(CMAKE_BUILD_TYPE MATCHES RELEASE)
    IF(APPLE)
        message ("MacOS build adding sqlite from homebrew")
        ########## SQLITE and others may need extra instructions; Linux does not have this problems ######
        include_directories(/opt/homebrew/opt/sqlite/include)
        include_directories(/usr/local/include)
        link_directories(/usr/local/lib)
        add_definitions(-DSQLite3_INCLUDE_DIRS=/opt/homebrew/opt/sqlite/include)
        add_definitions(-DSQLite3_LIBRARIES=/opt/homebrew/opt/sqlite/lib/libsqlite3.dylib)
        # Add your MacOS specific commands here
        # refer also to the FFTW cmake file!
    ENDIF(APPLE)
ENDIF(CMAKE_CXX_COMPILER_ID MATCHES "Clang" AND NOT WIN32)
#
#
#
# set CMAKE_BUILD_TYPE RELEASE for GCC
IF(CMAKE_CXX_COMPILER_ID MATCHES "GNU" AND NOT WIN32)
# using regular Clang or AppleClang
    message ("UNIX LINUX GNU GCC build")
    # on Linux and MacOS we can build shared libs for all not explicitly marked as STATIC
    option(BUILD_SHARED_LIBS "Build using shared libraries" ON)
    IF(CMAKE_BUILD_TYPE MATCHES DEBUG)
        message ("debug build ---------------------")
        add_compile_options(-std=c++20 -pthread -ggdb3 -O0 -Wsign-compare -Wall -Wextra -Werror -pedantic)
        set(CMAKE_VERBOSE_MAKEFILE ON)
    ENDIF(CMAKE_BUILD_TYPE MATCHES DEBUG)
    IF(CMAKE_BUILD_TYPE MATCHES RELEASE)
        message ("release build ---------------------")
        add_compile_options(-std=c++20 -pthread -Wno-unused-parameter -Wno-sign-compare)
    ENDIF(CMAKE_BUILD_TYPE MATCHES RELEASE)
ENDIF(CMAKE_CXX_COMPILER_ID MATCHES "GNU" AND NOT WIN32)
#
#

# set CMAKE_BUILD_TYPE RELEASE for g++ on mingw WINDOWS
# I use the mingw delivered together with Qt, that works great
IF(WIN32)
    message ( "WINDOWS build" )
    # on WINDOWS it is easier to make STATIC libs BUT it does not work with templates
    # option(BUILD_SHARED_LIBS "Build using static libraries" OFF)
    # add_compile_definitions( _USE_MATH_DEFINES _msvc )
    IF(CMAKE_BUILD_TYPE MATCHES DEBUG)
        message ("debug build ---------------------")
        add_compile_options(-DDEBUG -pthread)
        set(CMAKE_VERBOSE_MAKEFILE ON)
    ENDIF(CMAKE_BUILD_TYPE MATCHES DEBUG)
    IF(CMAKE_BUILD_TYPE MATCHES RELEASE)
        message ("release build ---------------------")
        add_compile_options(-DNDEBUG -pthread)
    ENDIF(CMAKE_BUILD_TYPE MATCHES RELEASE)
ENDIF(WIN32)

IF (WIN32)
    message ( "starting WINDOWS build" )
# add_definitions(-DQCUSTOMPLOT_USE_OPENGL)
# set(QCUSTOMPLOT_USE_OPENGL TRUE)
ELSE()
# add_definitions(-DQCUSTOMPLOT_USE_OPENGL)
# set(QCUSTOMPLOT_USE_OPENGL TRUE)
    message ("starting UNIX LINUX Apple Clang / GCC build")
ENDIF()
message ( "adding subdirs" )
## templates FIRST
include_directories(${CMAKE_SOURCE_DIR}/include)
include_directories(${CMAKE_SOURCE_DIR}/math_vector)
include_directories(${CMAKE_SOURCE_DIR}/cal/read_cal)
# add include path for xml/tinyxmlwriter
include_directories(${CMAKE_SOURCE_DIR}/xml/tinyxmlwriter)
include_directories(${CMAKE_SOURCE_DIR}/xml/tinyxml2)
include_directories(${CMAKE_SOURCE_DIR}/mt/raw_spectra)
add_subdirectory(include)
add_subdirectory(math_vector)
# stop here for now
# return()
#
# add_subdirectory(cal/mk_calibration)
add_subdirectory(xml/tinyxmlwriter)
add_subdirectory(xml/tinyxml2)
add_subdirectory(ats/atsinfo)
add_subdirectory(cal/read_cal)
add_subdirectory(cal/chcal)
add_subdirectory(cal/mod_xml_cal)
add_subdirectory(data)
add_subdirectory(mt/tensor)
add_subdirectory(mt/raw_spectra)
add_subdirectory(mt/fir_filter)
#add_subdirectory(file/atssfile)
add_subdirectory(ats/ascii2ats)
add_subdirectory(ats/atmtools)
add_subdirectory(ats/atstools)
add_subdirectory(utils/mk_noise)
add_subdirectory(utils/filter)
add_subdirectory(utils/rda)
add_subdirectory(utils/atss_to_ascii)
# add_subdirectory(utils/spcplot)
add_subdirectory(utils/tsplot)
add_subdirectory(utils/show_system_cal)
add_subdirectory(pt/pt2surv)
add_subdirectory(pt/ptspc)
add_subdirectory(adu_sim)
add_subdirectory(tests/fftw_inv)
#
#
# add_subdirectory(tests/json_test)
#add_subdirectory(tests/ats_read_all)
#add_subdirectory(tests/atss_example)
#add_subdirectory(tests/tinyxmlwriter_test)
#add_subdirectory(tests/xmlcal_test)
#add_subdirectory(tests/sqltest)
#add_subdirectory(tests/lambdas)
#add_subdirectory(tests/read_cal_test)
if (!APPLE)
#add_subdirectory(tests/thread_pool)
#add_subdirectory(tests/sqlite_threads)
endif()
add_subdirectory(tests/execpts)
#add_subdirectory(tests/times)
#add_subdirectory(tests/prztest)
#add_subdirectory(tests/dirtest)
#add_subdirectory(tests/atmm_bin)
add_subdirectory(tests/fftw_length)
# add_subdirectory(tests/fftw_length_basics)
# add_subdirectory(tests/fftw_length_noise)
# add_subdirectory(tests/fftw_length_noise_survey)
# add_subdirectory(tests/all_runs)
# add_subdirectory(tests/filtertest)
# add_subdirectory(tests/gplt)
add_subdirectory(tests/fftw_length_noise_timeser)
# add_subdirectory(tests/fftw_length_noise_timeser_wwo)
# add_subdirectory(tests/firsum)




