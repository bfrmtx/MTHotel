#!/bin/bash

#  gnuplot -e "filename='28_scalar_mt_processing_stack_all_2.dat'" eplot.gp
# ==========================================
# This function is used to print the command
# line arguments for the script.
# ==========================================
function usage {
   if [  $# -lt 3 ] ; then
   echo ' -c component xx, xy, yx, yy, cohx, cohy , cohz, ix, iy'
   echo ' -w what phi, rho, coh, ind'
   echo ' -f ascii file '
#   echo ' -s 8 for 8 GB or greater'
#   echo ' -p p4 or p5 (selects the product step, p4 = old partition)'
   echo ' '
   echo ' example:  ' $0 ' -c xy -w rho -f 27_mt_stack_all.dat'
   echo ' '
   exit
fi
}




# ==========================================
# This is the main function
# ==========================================

# print command line options for script
echo ' '
usage $@

component='xy'
what='rho'
gpfile="edi.gpl"

# read command line options into correct variables
while getopts c:w:f: opt 
do
  case $opt in
    c ) component=$OPTARG;;
    w ) what=$OPTARG;;
    f ) aedi=$OPTARG;;
#    p ) pstep=$OPTARG;;
    \? ) usage ;;
    h ) usage ;;
    ? ) usage ;;
 
  esac
done

echo '# gnuplot file' > $gpfile


if [ $what == "rho" ] ; then
  echo 'set logscale x' >> $gpfile
  echo 'set xrange [*:*] reverse' >> $gpfile
  echo 'set yrange [*:*]' >> $gpfile
  echo 'set logscale y' >> $gpfile
  echo 'set xlabel ' \''f [Hz]'\' >> $gpfile
  echo 'set ylabel ' \''rho a [ohm*m]'\' >> $gpfile 
  echo 'set title '  \''rho' $aedi\'''  >> $gpfile
  
fi

if [ $what == "phi" ] ; then
  echo 'set logscale x' >> $gpfile
  echo 'set xrange [*:*] reverse' >> $gpfile
  echo 'set xlabel ' \''f [Hz]'\' >> $gpfile
  echo 'set yrange [*:*]' >> $gpfile
  echo 'set ylabel ' \''phi [deg]'\' >> $gpfile 
  echo 'set title '  \''phi' $aedi\'''  >> $gpfile
  
fi

if [ $what == "ind" ] ; then
  echo 'set logscale x' >> $gpfile
  echo 'set xrange [*:*] reverse' >> $gpfile
  echo 'set xlabel ' \''f [Hz]'\' >> $gpfile
  echo 'set yrange [*:*]' >> $gpfile
  echo 'set y2tics' >> $gpfile
  echo 'set ylabel ' \''ampl'\' >> $gpfile
  echo 'set y2label ' \''angle'\' >> $gpfile
  echo 'set title '  \''induction arrow' $aedi\'''  >> $gpfile
fi



if [ $component == "xx" ] && [ $what == "rho" ] ; then
  echo 'plot filename using 1:2:3 with yerrorbars title "rho_a xx"' >> $gpfile  
fi
if [ $component == "xy" ] && [ $what == "rho" ] ; then
  echo 'plot filename using 1:6:7 with yerrorbars title "rho_a xy"' >> $gpfile  
fi
if [ $component == "yx" ] && [ $what == "rho" ] ; then
  echo 'plot filename using 1:10:11 with yerrorbars lt 3 title "rho_a yx"' >> $gpfile  
fi
if [ $component == "yy" ] && [ $what == "rho" ] ; then
  echo 'plot filename using 1:14:15 with yerrorbars  lt 3 title "rho_a xy"' >> $gpfile  
fi

if [ $component == "xx" ] && [ $what == "phi" ] ; then
  echo 'plot filename using 1:4:5 with yerrorbars title "phi xx"' >> $gpfile  
fi
if [ $component == "xy" ] && [ $what == "phi" ] ; then
  echo 'plot filename using 1:8:9 with yerrorbars title "phi xy"' >> $gpfile  
fi
if [ $component == "yx" ] && [ $what == "phi" ] ; then
  echo 'plot filename using 1:12:13 with yerrorbars  lt 3 title "phi yx"' >> $gpfile  
fi
if [ $component == "yy" ] && [ $what == "phi" ] ; then
  echo 'plot filename using 1:16:17 with yerrorbars title  lt 3 "phi yy"' >> $gpfile  
fi

if [ $component == "xyyx" ] && [ $what == "rho" ] ; then
  echo 'plot filename using 1:6:7 with yerrorbars linecolor rgb "blue" title "rho_a xy" , filename using 1:10:11 with yerrorbars linecolor rgb "red" lt 3 title "rho_a yx' >> $gpfile  
fi

if [ $component == "xyyx" ] && [ $what == "phi" ] ; then
  echo 'plot filename using 1:8:9 with yerrorbars title "phi xy" , filename using 1:($12 + 180):13 with yerrorbars  lt 3 title "phi yx' >> $gpfile  
fi


if [ $component == "ix" ] && [ $what == "ind" ] ; then
  echo 'plot filename using 1:( sqrt( $21 * $21 + $22 * $22 ) ) axis x1y1 with linespoints linecolor rgb "blue" title "ampl" , ""   using 1:( (atan ($22 / $21)) / 3.1415 * 180 )  axis x1y2 with linespoints linecolor rgb "red" title "angle"' >> $gpfile
#  echo 'plot filename using 1:( sqrt( $21 * $21 + $22 * $22 ) ) axis x1y1  title "ampl' >> $gpfile  
fi


if [ $component == "iy" ] && [ $what == "ind" ] ; then
  echo 'plot filename using 1:( sqrt( $24 * $24 + $25 * $25 ) ) axis x1y1 with linespoints linecolor rgb "blue" title "ampl" , ""   using 1:( (atan ($25 / $24)) / 3.1415 * 180 )  axis x1y2 with linespoints linecolor rgb "red" title "angle"' >> $gpfile
#  echo 'plot filename using 1:( sqrt( $21 * $21 + $22 * $22 ) ) axis x1y1  title "ampl' >> $gpfile  
fi

echo 'pause -1'  >> $gpfile

echo 'calling: gnuplot -e ' \"filename=\'$aedi\'\" $gpfile $component

gnuplot -e "filename='$aedi'" edi.gpl 

#\"filename=\'$aedi\'\" $gpfile 




#mkfs.ext3 -L boot -v /dev/sdc1
#mkfs.ext3 -L root -v /dev/sdc2
# extended
#mkfs.ext3 -L home -b 4096 -v /dev/sdc4
#mkfs.ext3 -L data -b 4096 -v /dev/sdc5
