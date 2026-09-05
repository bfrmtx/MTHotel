#!/bin/zsh
clear
PROJ='MTHotel'
INTSTALLDIR='/usr/local/mthotel'
# debug here
# must be the top-level source dir: qt6/CMakeLists.txt relies on QT6_PROJECT_DIR and other
# variables/targets (raw_spectra, SQLite3, ...) that are only set up by the top-level CMakeLists.txt
SRC_DIR='./'
# build outside please!
BUILD_DIR=$HOME'/build/'$PROJ/'qt6/build'
#
#
# Number of parallel jobs if you want to limit the number of threads detected by ninja
PARALLEL_JOBS=8
#
mkdir -p $BUILD_DIR
# clang and ninja if ninja is installed
if ! which ninja > /dev/null; then
  echo "using build without ninja"
  cmake -S $SRC_DIR -B $BUILD_DIR -DCMAKE_CXX_STANDARD=23 -DCMAKE_CXX_COMPILER=clang++ -DCMAKE_INSTALL_PREFIX=$INTSTALLDIR -DCMAKE_BUILD_TYPE=RELEASE -DBUILD_RAW_CPP=OFF -DBUILD_QT_PROJECT=ON
  cmake --build $BUILD_DIR --parallel $PARALLEL_JOBS
else
  echo "using build with ninja"
  cmake -GNinja -S $SRC_DIR -B $BUILD_DIR -DCMAKE_CXX_STANDARD=23 -DCMAKE_CXX_COMPILER=clang++ -DCMAKE_INSTALL_PREFIX=$INTSTALLDIR -DCMAKE_BUILD_TYPE=RELEASE -DBUILD_RAW_CPP=OFF -DBUILD_QT_PROJECT=ON
  # if you want limit the number of threads detected by ninja
  # cmake --build $BUILD_DIR -- -j $PARALLEL_JOBS
  cmake --build $BUILD_DIR
fi

#
# in case make a single thread (maybe facing dependency errors) remove --parallel 8
#

cmake --install $BUILD_DIR
