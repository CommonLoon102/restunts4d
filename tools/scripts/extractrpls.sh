#!/bin/bash

# This script extracts all the replay files downloaded from zak.stunts.hu
# into the directory where the script file is being located. It is partitioning
# the replay files into 8 partitions so they can be processed in parallel later.

# Directory where this script is located
TARGET_DIR="$(cd "$(dirname "$0")" && pwd)"

# Counter for extracted .rpl files
COUNTER=0

# Process every .zip file in the target directory
for zipfile in "$TARGET_DIR"/*.zip; do

    # Skip if there are no .zip files
    [ -e "$zipfile" ] || continue

    echo "Processing: $(basename "$zipfile")"

    # Create a temporary directory for this ZIP
    TEMP_DIR=$(mktemp -d)

    # Extract ZIP
    if unzip -q "$zipfile" -d "$TEMP_DIR"; then

        # Find all .rpl files recursively
        while IFS= read -r -d '' rplfile; do

            # Four-digit counter
            NUMBER=$(printf "%04d" "$COUNTER")

            # Remainder when dividing counter by 8
            REMAINDER=$((COUNTER % 8))

            # Generate filename: 0000_0.rpl, 0001_1.rpl, ...
            NEW_NAME="$TARGET_DIR/${NUMBER}_${REMAINDER}.rpl"

            cp -f "$rplfile" "$NEW_NAME"

            echo "  $(basename "$rplfile") -> $(basename "$NEW_NAME")"

            ((COUNTER++))

        done < <(find "$TEMP_DIR" -type f -iname "*.rpl" -print0)

        echo "  Extracted .rpl files."

    else
        echo "  ERROR: Failed to unzip $(basename "$zipfile")"
    fi

    # Remove temporary files
    #rm -rf "$TEMP_DIR"

done

echo "Done. Extracted $COUNTER .rpl files."

