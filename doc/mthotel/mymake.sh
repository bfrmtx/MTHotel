 #!/usr/bin/zsh
 # call me with zsh mymake.sh - otherwise you get errors
 #
clear
build_dir="./build"
if [ -d "$build_dir" ]; then
   echo "$build_dir found, make empty"
else
   echo "info: $build_dir NOT found."
fi
base_dir=$(pwd)
db_dir=source/sqltables
if [ -d "$db_dir" ]; then
   echo "$db_dir found, make SQL TABLES"
   cd $db_dir
   scripts=$(ls *.sh)
   for i in ${=scripts}; do
     echo "-> executing " ${i}
     zsh ${i}
   done
   cd $base_dir

else
   echo "info: $db_dir NOT found."
fi
MAKE="$(which make)"
echo $(pwd)
$MAKE html
