#!/bin/bash

filename="$1"
SCRIPT_DIR="$(cd -- "$(dirname -- "${BASH_SOURCE[0]}")" && pwd)"
CONFIG="$SCRIPT_DIR/dosbox.proc.conf"
GAME_DIR="$SCRIPT_DIR/../../stunts"

wine cmd.exe /D /S /C "S: && cd S:\src\restunts && makerepldump"

run_dosbox_exe() {
    local exe="$1"
    local filename="$2"

    timeout --signal=KILL 10s \
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
            echo "timeout for $exe"
            return 124
            ;;

        *)
            echo "dosbox failure"
            return "$status"
            ;;
    esac
}

run_dosbox_exe "repldumo.exe" "$filename"
run_dosbox_exe "repldump.exe" "$filename"

base="${filename%.rpl}"
bin_file="${base}.BIN"
bni_file="${base}.BNI"

bin_exists=true
bni_exists=true

BINFILE="$GAME_DIR/$bin_file"
BNIFILE="$GAME_DIR/$bni_file"

# Check .BIN independently.
if [[ ! -f "$BINFILE" ]]; then
    echo "echo repldumo.exe hasn't produced output"
    bin_exists=false
fi

# Check .BNI independently.
if [[ ! -f "$BNIFILE" ]]; then
    echo "echo repldump.exe hasn't produced output"
    bni_exists=false
fi

# Only compare if both output files exist.
# cmp -s performs a byte-for-byte comparison without calculating hashes.
if [[ "$bin_exists" == true && "$bni_exists" == true ]]; then
    if ! cmp -s "$BINFILE" "$BNIFILE"; then
        echo "file mismatch"
    fi
fi