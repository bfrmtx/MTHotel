#!/bin/zsh
# Render the Quarto report
# : h for head, one above
# 1. where the script is located like /xxx/pages/mobile/quarto
# and index.qmd as well _quarto.yml
set -euo pipefail

project_dir=${0:A:h}

sync_media_assets() {
    if [[ ! -e "${project_dir}/media" ]]; then
        echo "No media directory found at ${project_dir}/media. Skipping media sync."
        return
    fi

    mkdir -p "${output_dir}/media"
    echo "Syncing media assets into ${output_dir}/media..."
    rsync -aL --delete "${project_dir}/media/" "${output_dir}/media/"
}

# 2. where the output should be, like /xxx/pages/mobile/doc
# we do not want to render into the project dir, 
mydir='html'
# mydir can not be empty, otherwise we will delete the project dir
if [[ -z "${mydir}" ]]; then
    echo "Output directory name (mydir) is empty. Refusing to proceed." >&2
    exit 1
fi
output_dir="${project_dir:h}/${mydir}"
echo "project dir: ${project_dir}"
echo "output dir: ${output_dir}"
# smart check: output and project must share parent and differ only in leaf dir
project_parent="${project_dir:h:A}"
output_parent="${output_dir:h:A}"
project_leaf="${project_dir:t}"
output_leaf="${output_dir:t}"

if [[ "${project_parent}" != "${output_parent}" ]]; then
    echo "Refusing to proceed: parent directories differ." >&2
    echo "project parent: ${project_parent}" >&2
    echo "output  parent: ${output_parent}" >&2
    exit 1
fi

if [[ "${project_leaf}" == "${output_leaf}" ]]; then
    echo "Refusing to proceed: project and output leaf dirs are identical (${project_leaf})." >&2
    exit 1
fi

if [[ "${output_leaf}" != "${mydir}" ]]; then
    echo "Refusing to proceed: output leaf (${output_leaf}) does not match mydir (${mydir})." >&2
    exit 1
fi

# final check
dangerous_dirs=(
    /html
    /srv/html
    /var/html
    /var/www/html
    /srv/www/html
    /usr/share/nginx/html
    /
    /home
)

if [[ -d "${output_dir}" ]]; then
    echo "Output directory ${output_dir} exists. Attempting to delete it..."

    for dangerous_dir in "${dangerous_dirs[@]}"; do
        if [[ "${output_dir}" == "${dangerous_dir}" ]]; then
            echo "Refusing to delete dangerous directory: ${output_dir}" >&2
            exit 1
        fi
    done
    echo "continue to delete ${output_dir} [Y/n]?"
    read -r answer
    if [[ "${answer}" == "n" || "${answer}" == "N" ]]; then
        echo "Aborting deletion of ${output_dir}."
        exit 1
    fi
    echo "Deleting existing ${mydir} directory: ${output_dir}"
    rm -rf -- "${output_dir}"
fi

# Render the Quarto report finally
echo "Rendering Quarto report..."
cd "${project_dir}"
quarto render
# sync_media_assets
echo "done"
