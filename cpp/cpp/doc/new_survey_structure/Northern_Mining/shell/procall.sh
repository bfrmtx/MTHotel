#!/bin/bash
  mpwd=`pwd`;
  echo $mpwd
  cd jobs
  for pdir in `ls`;
  do
      echo 'preparing'  $pdir
      cd $mpwd
      procmt_main ./jobs/$pdir
  done
  