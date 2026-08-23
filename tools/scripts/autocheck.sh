#!/bin/bash

if (( $# < 1 || $# > 2 )); then
    echo "Usage: $0 <replay-file> [true|false]" >&2
    exit 2
fi

filename="$1"
rebuild_exes="${2:-true}"

case "$rebuild_exes" in
    true|false)
        ;;
    *)
        echo "Usage: $0 <replay-file> [true|false]" >&2
        exit 2
        ;;
esac

SCRIPT_DIR="$(cd -- "$(dirname -- "${BASH_SOURCE[0]}")" && pwd)"
CONFIG="$SCRIPT_DIR/dosbox.proc.conf"
GAME_DIR="$SCRIPT_DIR/../../stunts"

base="${filename%.[rR][pP][lL]}"
BINFILE="$GAME_DIR/$base.BIN"
BNIFILE="$GAME_DIR/$base.BNI"

rm -f -- "$BINFILE" "$BNIFILE"

if [[ "$rebuild_exes" == true ]]; then
    wine cmd.exe /D /S /C "S: && cd S:\src\restunts && makerepldump"
fi

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

failed=false
run_dosbox_exe "repldumo.exe" "$filename" || failed=true
run_dosbox_exe "repldump.exe" "$filename" || failed=true

bin_exists=true
bni_exists=true

if [[ ! -f "$BINFILE" ]]; then
	echo "repldumo.exe hasn't produced output"
	bin_exists=false
	failed=true
fi

if [[ ! -f "$BNIFILE" ]]; then
	echo "repldump.exe hasn't produced output"
	bni_exists=false
	failed=true
fi

if [[ "$bin_exists" == true && "$bni_exists" == true ]]; then
	if ! cmp -s "$BINFILE" "$BNIFILE"; then
		echo "file mismatch"
		failed=true
	fi
fi

if [[ "$failed" == true ]]; then
	exit 1
fi
