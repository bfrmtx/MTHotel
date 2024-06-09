#!/bin/zsh
clear
PROJ='MTHotel'
INTSTALLDIR='/usr/local/mthotel'
# debug here
SRC_DIR=$HOME'/devel/github_mthotel/MTHotel/cpp/'
# build outside please!
BUILD_DIR=$HOME'/build/'$PROJ/'cpp/build'
#
# comment when final
#INTSTALLDIR=$HOME'/build/'$PROJ'_cpp/install'


#
mkdir -p $BUILD_DIR
# clang
# -DCMAKE_VERBOSE_MAKEFILE=ON
# -GNinja
cmake -S $SRC_DIR -B $BUILD_DIR -DCMAKE_CXX_STANDARD=20 -DCMAKE_CXX_COMPILER=clang++ -DCMAKE_INSTALL_PREFIX=$INTSTALLDIR -DCMAKE_BUILD_TYPE=RELEASE
#
# in case make a single thread (maybe facinging dependency errors) remove --parallel 8
#
cmake --build $BUILD_DIR --parallel 8
cmake --install $BUILD_DIR
