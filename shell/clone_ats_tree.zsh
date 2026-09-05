#!/bin/zsh
# parameters for cloning ATS tree: 
# $1 - source tree
# $2 - destination directory (will be created in case it does not exist)

# we iterate over the source tree deeper and deeper;
# we call $prog -i <in_directory> -o <out_directory>
# at the end we have two same directory trees, one in the source location with *.ats files and one in the destination directory with *.atss files

prog=/usr/local/mthotel/bin/ats2survey
src=$1
if [ -z "$1" ] || [ -z "$2" ]; then
    echo "Usage: $0 <source tree> <destination directory>"
    exit 1
fi
dst=$2
mkdir -p $dst
# -i src/a/b/c -o dst/a/b (c will be created)
for dir in $(find $src -type d); do
    subdir=${dir#$src}
    mkdir -p "$dst/$subdir"
    upperdir=$(dirname "$dst/$subdir")
    $prog -i "$dir" -o "$upperdir"
done