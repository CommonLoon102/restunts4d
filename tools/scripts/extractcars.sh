#!/usr/bin/env bash

# This script is extracting all the car files from a zip file into the
# current directory. The zip files are from zak.stunts.hu, from each year
# of the contest. The script checks if there are any files with same name
# but different content. (There isn't, all cars are the same "version".)

set -euo pipefail

TARGET_DIR="$(pwd)"

command -v unzip >/dev/null 2>&1 || {
    echo "ERROR: 'unzip' is not installed." >&2
    exit 1
}

command -v sha256sum >/dev/null 2>&1 || {
    echo "ERROR: 'sha256sum' is not installed." >&2
    exit 1
}

shopt -s nullglob
zip_files=("$TARGET_DIR"/*.zip)

if (( ${#zip_files[@]} == 0 )); then
    echo "No .zip files found."
    exit 0
fi

for zip_file in "${zip_files[@]}"; do
    zip_name="$(basename "$zip_file")"

    echo "Processing: $zip_name"

    while IFS= read -r file; do
        [[ -z "$file" ]] && continue

        # ZIPs should contain files directly in the root,
        # not directories or files inside subdirectories.
        if [[ "$file" == */ ]]; then
            echo "ERROR: '$zip_name' contains a directory: $file" >&2
            exit 1
        fi

        if [[ "$file" == */* ]]; then
            echo "ERROR: '$zip_name' contains a file in a subdirectory: $file" >&2
            exit 1
        fi

        target="$TARGET_DIR/$file"

        if [[ -e "$target" ]]; then
            # The target already exists, so compare SHA-256 hashes.

            existing_hash="$(sha256sum "$target" | awk '{print $1}')"

            zip_hash="$(
                unzip -p "$zip_file" "$file" |
                sha256sum |
                awk '{print $1}'
            )"

            if [[ "$existing_hash" == "$zip_hash" ]]; then
                echo "  SKIP: $file (identical)"
            else
                echo
                echo "ERROR: File conflict!" >&2
                echo "  File:           $file" >&2
                echo "  ZIP:            $zip_name" >&2
                echo "  Existing SHA256: $existing_hash" >&2
                echo "  ZIP SHA256:      $zip_hash" >&2
                echo
                echo "Aborting." >&2
                exit 1
            fi
        else
            # File doesn't exist, so extract it.
            echo "  EXTRACT: $file"

            unzip -q -j "$zip_file" "$file" -d "$TARGET_DIR"
        fi

    done < <(unzip -Z1 "$zip_file")

    echo
done

echo "Done."

