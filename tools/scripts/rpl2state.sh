#!/bin/bash

if [[ $# -ne 2 ]]; then
    echo "Usage: $0 <partition> <partition-count>" >&2
    exit 2
fi

partition="$1"
partition_count="$2"

if [[ ! "$partition" =~ ^[0-9]+$ ]]; then
    echo "Partition must be a non-negative integer." >&2
    exit 2
fi

if [[ ! "$partition_count" =~ ^[1-9][0-9]*$ ]]; then
    echo "Partition count must be a positive integer." >&2
    exit 2
fi

if (( 10#$partition >= 10#$partition_count )); then
    echo "Partition must be less than partition count." >&2
    exit 2
fi

script_dir=$(cd -- "$(dirname -- "${BASH_SOURCE[0]}")" && pwd)
GAME_DIR="$script_dir"
CONFIG="$GAME_DIR/dosbox.proc.conf"
output_file="$GAME_DIR/partition_$partition.txt"

# Seconds allowed per executable. A run competing for a core with more
# partitions than the machine has cores can outlast the limit without
# hanging, so it is overridable rather than fixed.
timeout_seconds="${REPLAY_TIMEOUT_SECONDS:-30}"

if [[ ! "$timeout_seconds" =~ ^[1-9][0-9]*$ ]]; then
    echo "REPLAY_TIMEOUT_SECONDS must be a positive integer." >&2
    exit 2
fi

files=()

# Target filenames end with a four-digit counter, such as 0000.rpl or
# sl0000.RPL. Calculate each file's partition from that counter at runtime.
shopt -s nullglob
for file in "$GAME_DIR"/*.[rR][pP][lL]; do
    filename=${file##*/}
    if [[ "$filename" =~ ([0-9]{4})\.[rR][pP][lL]$ ]]; then
        counter=${BASH_REMATCH[1]}
    else
        continue
    fi

    if (( 10#$counter % 10#$partition_count == 10#$partition )); then
        files+=("$file")
    fi
done
shopt -u nullglob

# Skip if there are no matching files
if (( ${#files[@]} == 0 )); then
    echo "No matching files found."
    exit 0
fi

# Log the same single-line error to both the console and the output file.
log_error() {
    local message="$1"
    echo "$message" | tee -a "$output_file" >&2
}

# Run one executable through DOSBox-X with a timeout.
run_dosbox_exe() {
    local exe="$1"
    local filename="$2"

    timeout --signal=KILL "${timeout_seconds}s" \
        dosbox-x \
        -silent \
        -conf "$CONFIG" \
        -c "mount c $GAME_DIR" \
        -c "c:" \
        -c "$exe $filename 1" \
        -c "exit" \
        >/dev/null 2>&1

    local status=$?

    case $status in
        0)
            return 0
            ;;

        124|137)
            log_error "ERROR|type=timeout|exe=$exe|input=$filename|timeout_seconds=$timeout_seconds"
            return 124
            ;;

        *)
            log_error "ERROR|type=dosbox_failure|exe=$exe|input=$filename|exit_code=$status"
            return "$status"
            ;;
    esac
}

total=${#files[@]}
processed=0

for file in "${files[@]}"; do
    filename=$(basename "$file")
    ((processed++))

    echo "Processing $processed/$total: $filename"

    # Run repldumo.exe separately.
    if ! run_dosbox_exe "repldumo.exe" "$filename"; then
        # If repldumo.exe fails or times out, do not run repldump.exe.
        continue
    fi

    # Run repldump.exe separately.
    if ! run_dosbox_exe "repldump.exe" "$filename"; then
        # If repldump.exe fails or times out, do not check the output files.
        continue
    fi

    # Output files have the same basename as the input,
    # but with .BIN and .BNI extensions.
    base="${file%.*}"
    bin_file="${base}.BIN"
    bni_file="${base}.BNI"

    bin_exists=true
    bni_exists=true

    # Check .BIN independently.
    if [[ ! -f "$bin_file" ]]; then
        log_error "ERROR|type=missing_output|input=$filename|output=$(basename "$bin_file")"
        bin_exists=false
    fi

    # Check .BNI independently.
    if [[ ! -f "$bni_file" ]]; then
        log_error "ERROR|type=missing_output|input=$filename|output=$(basename "$bni_file")"
        bni_exists=false
    fi

    # Only compare if both output files exist.
    # cmp -s performs a byte-for-byte comparison without calculating hashes.
    if [[ "$bin_exists" == true && "$bni_exists" == true ]]; then
        if ! cmp -s "$bin_file" "$bni_file"; then
            log_error "ERROR|type=file_mismatch|input=$filename|bin=$(basename "$bin_file")|bni=$(basename "$bni_file")"
        fi
    fi
done
