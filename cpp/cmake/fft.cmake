#
# fftw cmake is buggy on some compilers and systems
# ./configure  --enable-shared --disable-fortran CFLAGS="-mmacosx-version-min=12"
# try find_package(FFTW3 REQUIRED)

# if we are on apple xcode we check for fftw in user local
if (APPLE)
    message(STATUS "FFTW3 searching /usr/local on apple")
    if (EXISTS /usr/local/include/fftw3.h)
        message(STATUS "FFTW3 found in /usr/local")
        # add the include directory to existing include directories
        include_directories(/usr/local/include)
        link_directories(/usr/local/lib)
        set(FFTW_LIBRARIES fftw3)
    endif()
# else we use the find_package
else()
    find_package(FFTW3 REQUIRED)
    if (FFTW3_FOUND)
        include_directories(${FFTW3_INCLUDE_DIR})
        set(FFTW_LIBRARIES ${FFTW3_LIBRARIES})
    endif()
endif()

