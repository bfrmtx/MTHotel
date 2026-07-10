#!/bin/zsh
clear
PROJ='MTHotel'
INTSTALLDIR='/usr/local/mthotel'
# debug here
SRC_DIR='./'
# build outside please!
BUILD_DIR=$HOME'/build/'$PROJ/'cpp/build'
#
# Number of parallel jobs if you want to limit the number of threads detected by ninja
PARALLEL_JOBS=8
#
#
mkdir -p $BUILD_DIR
if ! which ninja > /dev/null; then
  echo "using build without ninja"
  cmake -S $SRC_DIR -B $BUILD_DIR -DCMAKE_CXX_STANDARD=23 -DCMAKE_CXX_COMPILER=clang++ -DCMAKE_INSTALL_PREFIX=$INTSTALLDIR -DCMAKE_BUILD_TYPE=RELEASE -DBUILD_RAW_CPP=ON -DBUILD_QT_PROJECT=OFF
  cmake --build $BUILD_DIR --parallel $PARALLEL_JOBS
else
  echo "using build with ninja"
  cmake -GNinja -S $SRC_DIR -B $BUILD_DIR -DCMAKE_CXX_STANDARD=23 -DCMAKE_CXX_COMPILER=clang++ -DCMAKE_INSTALL_PREFIX=$INTSTALLDIR -DCMAKE_BUILD_TYPE=RELEASE -DBUILD_RAW_CPP=ON -DBUILD_QT_PROJECT=OFF
  # if you want limit the number of threads detected by ninja
  # cmake --build $BUILD_DIR -- -j $PARALLEL_JOBS
  cmake --build $BUILD_DIR
fi
cmake --install $BUILD_DIR
