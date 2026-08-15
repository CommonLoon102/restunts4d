#!/bin/bash

# This script copies desynced replays into a subfolder named desync.

# Check parameter
if [ "$#" -ne 1 ]; then
    echo "Usage: $0 <error_file.txt>"
    exit 1
fi

INPUT_FILE="$1"
DEST_DIR="./desync"

# Check input file exists
if [ ! -f "$INPUT_FILE" ]; then
    echo "Error: File not found: $INPUT_FILE"
    exit 1
fi

# Create destination directory
mkdir -p "$DEST_DIR"

# Process matching lines
grep 'type=file_mismatch' "$INPUT_FILE" |
while IFS= read -r line; do
    # Extract filename after input=
    filename=$(printf '%s\n' "$line" | sed -n 's/.*input=\([^|]*\).*/\1/p')

    if [ -n "$filename" ]; then
        if [ -f "$filename" ]; then
            echo "Copying: $filename"
            cp -- "$filename" "$DEST_DIR/"
        else
            echo "Warning: File not found: $filename" >&2
        fi
    fi
done

echo "Done. Files copied to: $DEST_DIR"
