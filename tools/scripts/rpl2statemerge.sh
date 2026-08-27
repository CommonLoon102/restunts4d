#!/bin/bash

if (( $# < 2 || $# > 3 )); then
    echo "Usage: $0 <partition-dir> <partition-count> [output-file]" >&2
    exit 2
fi

partition_dir="$1"
partition_count="$2"

if [[ ! -d "$partition_dir" ]]; then
    echo "Partition directory does not exist: $partition_dir" >&2
    exit 2
fi

if [[ ! "$partition_count" =~ ^[1-9][0-9]*$ ]]; then
    echo "Partition count must be a positive integer." >&2
    exit 2
fi

combined_output_file="${3:-$partition_dir/partitions_all.txt}"

# Remove exact duplicate lines, then sort by the value of the input field.
{
    for ((partition = 0; partition < 10#$partition_count; partition++)); do
        partition_file="$partition_dir/partition_$partition.txt"
        if [[ -f "$partition_file" ]]; then
            cat -- "$partition_file"
        fi
    done
} |
    awk -F '|' '
        !seen[$0]++ {
            input_name = ""
            for (field = 1; field <= NF; field++) {
                if ($field ~ /^input=/) {
                    input_name = substr($field, 7)
                    break
                }
            }
            print input_name "\t" $0
        }
    ' |
    LC_ALL=C sort -s -t $'\t' -k1,1 |
    cut -f2- > "$combined_output_file"
