#!/bin/bash
: << 'COMMENT'
|------------------------------|------------------------------|
|         🎃 item              |        👇 explanation        |
|------------------------------|------------------------------|
|    needs root permission?    |              No              |
|------------------------------|------------------------------|
|          dependencies        |              No              |
|------------------------------|------------------------------|
|          fellows             |              No              |
|------------------------------|------------------------------|

Introduction of this script:
Given at least two parameters. The first parameter is treated as directory path
and the others are treated as file pattern. This script will delete all files
in given directory except the ones that matching one of given pattern.
COMMENT

# Function to filter and delete shared libraries not in the provided list
filter_files() {
    local target_dir="$1"
    shift
    local libraries_to_keep=("$@")

    # Check if the directory exists
    if [[ ! -d "${target_dir}" ]]; then
        echo "Filter libraries failed since directory '${target_dir}' doesn't exist."
        return 1
    fi

    # Iterate over all shared libraries in the directory
    for lib in "${target_dir}"/*; do
        # Check if the file is a regular file
        if [[ -f "${lib}" ]]; then
            local lib_name=$(basename "${lib}")
            local keep_lib=false

            # Check if the library is in the list to keep
            for keep in "${libraries_to_keep[@]}"; do
                # To support pattern matching, don't use quoted string for keep.
                if [[ "${lib_name}" == $keep ]]; then
                    keep_lib=true
                    break
                fi
            done

            # If the library is not in the list, delete it
            if [[ "${keep_lib}" == false ]]; then
                rm -f "${lib}"
            fi
        fi
    done
}

filter_files "$@"
