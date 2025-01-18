 #!/usr/bin/zsh
 # call me with zsh mymake.sh - otherwise you get errors
 #
clear
build_dir="./build"
if [ "$1" = "clear" ]; then
   echo "Clearing build directory"
   if [ -d "$build_dir" ]; then
      echo "$build_dir found, make empty"
      rm -rf "$build_dir"
   fi
fi
# iterate over the sqltables directory
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
# iterate over the python_graphs directory
py_dir=source/python_graphs
# call the python scripts, except for embed_images.py
if [ -d "$py_dir" ]; then
   echo "$py_dir found, make PYTHON GRAPHS"
   cd $py_dir
   scripts=$(ls *.py)
   for i in ${=scripts}; do
     if [ ${i} != "embed_images.py" ]; then
       echo "-> executing " ${i}
       python ${i}
     fi
   done
   cd $base_dir
else
   echo "info: $py_dir NOT found."
fi
MAKE="$(which make)"
echo $(pwd)
$MAKE html
