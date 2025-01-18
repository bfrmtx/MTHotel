 
# ------------------RPATH for loading .so from ../lib ------------------------------------------
# use, i.e. don't skip the full RPATH for the build tree
set(CMAKE_SKIP_BUILD_RPATH FALSE)

# when building, don't use the install RPATH; 
# make always a fresh build
set(CMAKE_BUILD_WITH_INSTALL_RPATH FALSE)

set(CMAKE_INSTALL_RPATH "${CMAKE_INSTALL_PREFIX}/lib")

# add the automatically determined parts of the RPATH
# which point to directories outside the build tree to the install RPATH
set(CMAKE_INSTALL_RPATH_USE_LINK_PATH TRUE)
#
# ensure that the library is compiled first!
link_directories(${CMAKE_INSTALL_PREFIX}/lib)

# ------------------------------------------------------------

# check it under Linux
# objdump -x  executable_or_lib.so | grep RPATH

#under MAC
#otool -D <file> to view the install name of a dylib
#otool -L <file> to view the dependencies
#otool -l <file> | grep LC_RPATH -A2 to view the RPATHs
#install_name_tool -id ... to change an install name
#install_name_tool -change ... to change the dependencies
#install_name_tool -rpath ... -add_rpath ... -delete_rpath ... to change RPATHs
