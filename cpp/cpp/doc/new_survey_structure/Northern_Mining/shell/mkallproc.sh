#!/bin/bash
if [ "$1" == "-r" ] ; then
export DISPLAY=:0
echo 'XDISPLAY OFF'
shift
fi
  mpwd=`pwd`;
  echo $mpwd
  
  cd ts

  for pdir in `ls`;
  do
      echo 'preparing'  $pdir
      cd $mpwd
      procmt_createjob $1 $2 $3 $4 $5 $6 $7 $8 $mpwd/ts/$pdir/*/*xml >$mpwd/jobs/$pdir.xml
  done
  