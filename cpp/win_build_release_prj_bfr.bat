cls
set PROJ=MTHotel
set out=%HOMEDRIVE%%HOMEPATH%\build\%PROJ%_cpp
set targ=%HOMEDRIVE%%HOMEPATH%\install\%PROJ%
rem set in="Y:\github_procmt_2021\procmt_2021"
set in="Z:\github_mthotel\MTHotel\cpp"
rem set in="Z:/procmt_2021"
rem setx BOOST_ROOT C:/boost/boost_1_78_0
rem setx /u bfr BOOST_INCLUDEDIR c:/boost/boost_1_78_0/boost
rem setx Boost_INCLUDEDIR C:/boost/boost_1_78_0/boost
rem
md %out%
md %targ%
cmake -S %in% -B %out% -GNinja -DCMAKE_CXX_COMPILER=g++ -DCMAKE_INSTALL_PREFIX=%targ% -DCMAKE_VERBOSE_MAKEFILE=OFF -DCMAKE_BUILD_TYPE=RELEASE
cmake --build %out%
cmake --install %out%
