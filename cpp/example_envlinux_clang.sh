#
# debug so libs in QTCreator
# Tools -> Options -> Debugger -> GDB
# Additional Startup Commands
# set solib-search-path /home/bfr/sw/build/procmt/bin/
# and or in .gdbinit
#
set(CMAKE_SYSTEM_NAME Linux)
set(CMAKE_C_COMPILER /usr/bin/clang)
set(CMAKE_CXX_COMPILER /usr/bin/clang++)
# set(CMAKE_BUILD_TYPE Debug)
