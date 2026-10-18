#!/bin/zsh
# parameters for cloning ATS tree: 
# $1 - data tree 
# find all *.atss and .json files, move the up ONE directory
# wait for user confirmation before proceeding
# delete all directories starting with "meas" after moving files

src=$1
if [ -z "$1" ]; then
    echo "Usage: $0 <data tree>"
    exit 1
fi

echo "This will move all *.atss and .json files up one directory in $src"
read "response?Do you want to continue? (y/n) "
if [[ "$response" != "y" ]]; then
    echo "Operation cancelled."
    exit 0
fi

# move all *.atss and .json files up one directory
find "$src" -type f \( -name "*.atss" -o -name "*.json" \) -exec sh -c '
    for file; do
        mv "$file" "$(dirname "$file")/../"
    done
' sh {} +

# delete all directories starting with "meas"
echo "Deleting all directories starting with 'meas' in $src"
read "response?Do you want to continue with deletion? (y/n) "
if [[ "$response" != "y" ]]; then
    echo "Deletion cancelled."
    exit 0
fi
find "$src" -type d -name "meas*" -exec rmdir  {} \;