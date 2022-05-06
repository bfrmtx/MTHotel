cls
set PROJ=MTHotel
set out=%HOMEDRIVE%%HOMEPATH%\build\%PROJ%_SQLite3
set targ=%HOMEDRIVE%%HOMEPATH%\install\%PROJ%%
rem set in="Y:\github_procmt_2021\procmt_2021"
set in="Z:\github_mthotel\MTHotel\oss\sqlite\SQLite3"
rem set in="Z:/procmt_2021"

rem
md %out%
md %targ%
cmake -S %in% -B %out% -GNinja -DCMAKE_CXX_STANDARD=20 -DCMAKE_CXX_COMPILER=g++ -DCMAKE_INSTALL_PREFIX=%targ% -DCMAKE_VERBOSE_MAKEFILE=OFF -DCMAKE_BUILD_TYPE=RELEASE
cmake --build %out%
cmake --install %out%
