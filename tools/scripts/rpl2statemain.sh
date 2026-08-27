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
merge_script="$script_dir/rpl2statemerge.sh"
game_dir="$script_dir"
combined_output_file="$game_dir/partitions_all.txt"

for ((partition = 0; partition < 10#$partition_count; partition++)); do
    "$worker_script" "$partition" "$partition_count" &
done

wait

"$merge_script" "$game_dir" "$partition_count" "$combined_output_file"
