#!/bin/bash

if [[ $# -ne 1 ]]; then
    echo "Usage: $0 <partition-count>" >&2
    exit 2
fi

partition_count="$1"

if [[ ! "$partition_count" =~ ^[1-9][0-9]*$ ]]; then
    echo "Partition count must be a positive integer." >&2
    exit 2
fi

script_dir=$(cd -- "$(dirname -- "${BASH_SOURCE[0]}")" && pwd)
worker_script="$script_dir/rpl2state.sh"
game_dir="$script_dir"
combined_output_file="$game_dir/partitions_all.txt"

for ((partition = 0; partition < 10#$partition_count; partition++)); do
    "$worker_script" "$partition" "$partition_count" &
done

wait

# Remove exact duplicate lines, then sort by the value of the input field.
{
    for ((partition = 0; partition < 10#$partition_count; partition++)); do
        partition_file="$game_dir/partition_$partition.txt"
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
