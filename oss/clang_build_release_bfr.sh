#!/bin/bash
clear
PROJ='oss'
INTSTALLDIR='/usr/local/mthotel'
# debug here
SRC_DIR=$HOME'/devel/github_mthotel/MTHotel/oss/matplotplusplus'
# build outside please!
SRC_DIR_BUILD=$HOME'/build/'$PROJ'_cpp/matplotplusplus'
BUILD_DIR=$SRC_DIR_BUILD'/build'
#
#
mkdir -p $BUILD_DIR
echo $SRC_DIR $SRC_DIR_BUILD
cp -a $SRC_DIR $HOME'/build/'$PROJ'_cpp/'
cd $BUILD_DIR
pwd
cmake -S $SRC_DIR_BUILD -DWITH_SYSTEM_CIMG=ON -DCMAKE_INSTALL_PREFIX=$INTSTALLDIR -DCMAKE_BUILD_TYPE=RELEASE
#
# in case make a single thread (maybe facingin dependency errors) remove --parallel 8
#
cmake --build $BUILD_DIR --parallel 8
cmake --install $INTSTALLDIR
