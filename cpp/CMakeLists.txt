cmake_minimum_required(VERSION 3.31)
project(MTHotel)

# we have only one choice:
# for header only classes WE CAN use Qt in a non Qt project
# for a LIBRARY we can NOT

# Optionally, control whether to build each subproject
option(BUILD_RAW_CPP "Build raw C++ project" ON)
# other options follow later
###  YOU NEED TO CLEAR THE CACHE AND BUILD DIRECTORY IF YOU CHANGE THE OPTIONS
option(BUILD_QT_PROJECT "Build Qt project" ON)
set(QCUSTOMPLOT_USE_OPENGL TRUE)
#
# optionally use homebrew
option (USE_HOMEBREW "Use Homebrew on MacOS" ON)



# now variables for all project; do not repeat them in subdirectories
set(CMAKE_CXX_STANDARD 23)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
# that should be default
set(CMAKE_INCLUDE_CURRENT_DIR ON)

#
# for your orientation, the project structure
set(CPP_PROJECT_DIR ${CMAKE_SOURCE_DIR}/cpp)
if (BUILD_QT_PROJECT)
    # if you want to build the Qt project, set the directory
    # this is the directory where the Qt project is located
    # it is not used in the CMakeLists.txt of the Qt project
    set(QT6_PROJECT_DIR ${CMAKE_SOURCE_DIR}/qt6)
else()
    # if you do not build the Qt project, set it to an empty string
    set(QT6_PROJECT_DIR "")
endif(BUILD_QT_PROJECT)
#
#
# that is deeply nested in menus and selections
find_package(SQLite3 REQUIRED)
#
############# Require out-of-source builds
# include adds here cmake files to be included here.
# on synced drives you do NOT want to sync the build! only the source
include(${CMAKE_SOURCE_DIR}/cmake/out_of_tree_build.cmake)
# use the rpath, we have /bin /lib and so on in the target INSTALL
include(${CMAKE_SOURCE_DIR}/cmake/rpath.cmake)


##############################################################################
# check it under Linux
# objdump -x  executable_or_lib.so | grep RPATH

#under MAC
#otool -D <file> to view the install name of a dylib
#otool -L <file> to view the dependencies
#otool -l <file> | grep LC_RPATH -A2 to view the RPATHs
#install_name_tool -id ... to change an install name
#install_name_tool -change ... to change the dependencies
#install_name_tool -rpath ... -add_rpath ... -delete_rpath ... to change RPATHs
#
# rpath does not work under Windows
# set CMAKE_BUILD_TYPE RELEASE for CLANG ########################## UNIX APPLE
IF(CMAKE_CXX_COMPILER_ID MATCHES "Clang" AND NOT WIN32)
# using regular Clang or AppleClang
    message ("UNIX LINUX Apple Clang build")
    # on Linux and MacOS we can build shared libs - for all not explicitly marked as STATIC
    option(BUILD_SHARED_LIBS "Build using shared libraries" ON)
    IF(CMAKE_BUILD_TYPE MATCHES DEBUG)
        message ("debug build ---------------------")
        # Qt uses int instead of size_t but I show it here
        add_compile_options(-std=c++23 -march=native -pthread -glldb -fstandalone-debug -Wsign-compare -Wall -Wextra -pedantic  -Wno-unused-parameter)
        set(CMAKE_VERBOSE_MAKEFILE ON)
    ENDIF(CMAKE_BUILD_TYPE MATCHES DEBUG)
    IF(CMAKE_BUILD_TYPE MATCHES RELEASE)
        message ("release build ---------------------")
        # no sign compare is for Qt, containers are not strictly C++ compliant
        add_compile_options(-std=c++23 -march=native -pthread -O2 -DNDEBUG -Wno-unused-parameter -Wno-sign-compare)
    ENDIF(CMAKE_BUILD_TYPE MATCHES RELEASE)
    IF(APPLE)
        message ("MacOS build adding sqlite from homebrew")
        ########## SQLITE and others may need extra instructions; Linux does not have this problems ######
        include_directories(/usr/local/include)
        link_directories(/usr/local/lib)
        if (USE_HOMEBREW)
            # homebrew's sqlite
            include_directories(/opt/homebrew/opt/sqlite/include)
            link_directories(/opt/homebrew/opt/sqlite/lib)
            add_definitions(-DSQLite3_INCLUDE_DIRS=/opt/homebrew/opt/sqlite/include)
            add_definitions(-DSQLite3_LIBRARIES=/opt/homebrew/opt/sqlite/lib/libsqlite3.dylib)
            # and homebrew's fftw
            include_directories(/opt/homebrew/opt/fftw/include)
            link_directories(/opt/homebrew/opt/fftw/lib)
            add_definitions(-DFFTW_INCLUDE_DIRS=/opt/homebrew/opt/fftw/include)
            add_definitions(-DFFTW_LIBRARIES=/opt/homebrew/opt/fftw/lib/libfftw3.dylib)
        endif(USE_HOMEBREW)
        # Add your MacOS specific commands here
        # refer also to the FFTW cmake file!
    ENDIF(APPLE)
ENDIF(CMAKE_CXX_COMPILER_ID MATCHES "Clang" AND NOT WIN32)
message("compiler used / detected " ${CMAKE_CXX_COMPILER_ID})
#
#
#
# ####################### WINDOWS MSVC BUILD, RELEASE only ###################
#
#
IF(WIN32)
    # ensure that boost is found under C:\boost\boost
    # need to find required package boost
    # include <boost/math/distributions/students_t.hpp>
    # on windows, boost is not found by default, it should be installed in C:\boost
    set(Boost_ROOT "C:/devel/boost")
    find_package(Boost REQUIRED)
    set(Vulkan_INCLUDE_DIR "C:/devel/VulkanSDK/")
    if(MSVC)
        # _USE_MATH_DEFINES is only needed for MSVC to expose math constants in <cmath>
        message("Detected MSVC compiler ++++++++++++++++++++++++++++++++++++++++++++++++++++++++")
        # Uncomment if you need MSVC-specific defines
        add_compile_definitions(_USE_MATH_DEFINES _msvc)
        # my vulkan
        # /arch:AVX2 is supported by Intel and AMD processors that support AVX2 instructions.
        # It is supported by MSVC on Windows for both Intel and AMD CPUs with AVX2.
        add_compile_options(/arch:AVX2 /std:c++latest)
        # Force static build for all libraries
        set(BUILD_SHARED_LIBS OFF CACHE BOOL "Build static libraries" FORCE)
        # Windows does not natively support pthreads; use -pthread only on POSIX systems.
        # For MSVC, threading is handled by the runtime, so you do not need to add -pthread.
        # If you need POSIX threads on Windows, you would need to use a library like pthreads-w32, but that's uncommon.
        # Typically, do not add -pthread for MSVC.
        add_compile_options(/EHsc /Ox)
    elseif(MINGW)
        message("Detected MinGW")
        # Add MinGW/MSYS2 specific options here if needed
        # we need static libs for the clang++ compiler !! shared libs are not working
        add_compile_options(-DBUILD_SHARED_LIBS=OFF -static)
        IF(CMAKE_BUILD_TYPE MATCHES DEBUG)
            message ("debug build ---------------------")
            add_compile_options(-DDEBUG -g -pthread -Wno-unused-parameter -Wno-sign-compare)
            set(CMAKE_VERBOSE_MAKEFILE ON)
        ENDIF(CMAKE_BUILD_TYPE MATCHES DEBUG)
        IF(CMAKE_BUILD_TYPE MATCHES RELEASE)
            message ("release build ---------------------")
            add_compile_options(-DNDEBUG -O2 -pthread -march=x86-64-v3 -Wno-unused-parameter -Wno-sign-compare)
        ENDIF(CMAKE_BUILD_TYPE MATCHES RELEASE)
    else()
        message("Unknown Windows compiler")
    endif()
    message ( "starting WINDOWS build <<<<<<<<<<<<<<<<<<<<<<" )
ENDIF(WIN32)
#
#
##################### ADD BASE INCLUDE DIRS FOR ALL #################################
# this adds a search path for the include directories; you can use "json.hpp" without a "nlohmann/" prefix
include_directories(${CPP_PROJECT_DIR}/include)
include_directories(${CPP_PROJECT_DIR}/include/nlohmann)
include_directories(${CPP_PROJECT_DIR}/include/thread-pool)
include_directories(${CPP_PROJECT_DIR}/include/channel)
include_directories(${CPP_PROJECT_DIR}/include/ats)
include_directories(${CPP_PROJECT_DIR}/include/cal)
include_directories(${CPP_PROJECT_DIR}/include/cal/read_cal)
include_directories(${CPP_PROJECT_DIR}/include/channel)
include_directories(${CPP_PROJECT_DIR}/include/gnuplot)
include_directories(${CPP_PROJECT_DIR}/math_vector)
# add include path for xml/tinyxmlwriter
include_directories(${CPP_PROJECT_DIR}/xml/tinyxmlwriter)
include_directories(${CPP_PROJECT_DIR}/xml)
include_directories(${CPP_PROJECT_DIR}/xml/tinyxml2)
include_directories(${CPP_PROJECT_DIR}/mt/raw_spectra)
add_subdirectory(${CPP_PROJECT_DIR} cpp)

# ####################### RAW C++ part of project ##############################
# if(BUILD_RAW_CPP)
#     # Continue processing with the CMakeLists.txt in the cpp project directory if it exists
#     if(EXISTS "${CPP_PROJECT_DIR}/CMakeLists.txt")
#         add_subdirectory(${CPP_PROJECT_DIR} cpp)
#     else()
#         message(WARNING "CMakeLists.txt not found in ${CPP_PROJECT_DIR}, skipping add_subdirectory.")
#     endif()
# endif()
#

###################################### Qt ####################################
if(BUILD_QT_PROJECT)
    message("Qt project build is enabled.")
    add_definitions(-DUSE_QT)
    ################################## Always needed #########################
    find_package(Qt6
        REQUIRED COMPONENTS Core Gui Widgets Sql Network Xml
        OPTIONAL_COMPONENTS PrintSupport
    )
    message(STATUS "Qt6 found: ${Qt6Core_VERSION}")
    if (NOT Qt6_FOUND)
        message(FATAL_ERROR "Package Qt6 required, but not found!")
    endif()
    # uncomment the line below to use OpenGL #################################
    IF (NOT DEFINED QCUSTOMPLOT_USE_OPENGL)
        message ("BUILD WITHOUT OpenGL               <<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<")
    ELSE()
        message ("BUILD WITH OpenGL                  <<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<")
        # make it availabe in header and sources
        # this also switches OpenGL on in general
        #ifdef QCUSTOMPLOT_USE_OPENGL checks the line below
        add_definitions(-DQCUSTOMPLOT_USE_OPENGL)
        find_package(Qt6
            REQUIRED COMPONENTS OpenGL
        )
    ENDIF()
    #
    # finally add the qt6 tree
    add_subdirectory(${QT6_PROJECT_DIR} qt6)
    include_directories(${QT6_PROJECT_DIR}/include)

ENDIF(BUILD_QT_PROJECT)

## tests
# Enable testing for the project
add_subdirectory(${CPP_PROJECT_DIR}/tests)
    
        # Add Qt include directories globally for header-only usage
        #include_directories(${Qt6Core_INCLUDE_DIRS})
        #include_directories(${Qt6Gui_INCLUDE_DIRS})
        #include_directories(${Qt6Widgets_INCLUDE_DIRS})
        #include_directories(${Qt6Sql_INCLUDE_DIRS})
        #include_directories(${Qt6Network_INCLUDE_DIRS})
        #include_directories(${Qt6Xml_INCLUDE_DIRS})
        # if you ever link to Qt in a library or executable, use target_link_libraries() and
        # target_include_directories() instead.

# Finalize Qt project (must be at the end of the top-level CMakeLists.txt)
if(BUILD_QT_PROJECT)
    qt_finalize_project()
endif()
