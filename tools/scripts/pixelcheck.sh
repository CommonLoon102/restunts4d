#!/bin/bash

set -u

print_usage() {
    echo "Usage:" >&2
    echo "  $0 <replay-file> <true|false> <camera: 1-4> <target: 0-1>" >&2
    echo "  $0 <replay-file> <true|false> <camera: 1-4> <target: 0-1> <frame>" >&2
}

if (( $# != 4 && $# != 5 )); then
    print_usage
    exit 2
fi

filename="$1"
rebuild_exes="$2"
camera_number="$3"
target_number="$4"
bmp_mode=false
frame_argument=""
frame_number=""

if (( $# == 5 )); then
    bmp_mode=true
    frame_argument="$5"
fi

case "$rebuild_exes" in
    true|false)
        ;;
    *)
        print_usage
        exit 2
        ;;
esac

case "$target_number" in
    0|1)
        ;;
    *)
        echo "Target must be 0 (player) or 1 (opponent)." >&2
        exit 2
        ;;
esac

if [[ "$bmp_mode" == true ]]; then
    if [[ ! "$frame_argument" =~ ^[0-9]{1,5}$ ]]; then
        echo "Frame must be an integer from 0 to 65535." >&2
        exit 2
    fi
    frame_number=$((10#$frame_argument))
    if (( frame_number > 65535 )); then
        echo "Frame must be an integer from 0 to 65535." >&2
        exit 2
    fi
fi

case "$camera_number" in
    1|2|3|4)
        ;;
    *)
        echo "Camera must be 1 (F1), 2 (F2), 3 (F3), or 4 (F4)." >&2
        exit 2
        ;;
esac

script_dir=$(cd -- "$(dirname -- "${BASH_SOURCE[0]}")" && pwd)
config="$script_dir/dosbox.proc.conf"
game_dir="$script_dir/../../stunts"
timeout_seconds="${PIXELDUMP_TIMEOUT_SECONDS:-120}"

if [[ ! "$timeout_seconds" =~ ^[1-9][0-9]*$ ]]; then
    echo "PIXELDUMP_TIMEOUT_SECONDS must be a positive integer." >&2
    exit 2
fi

base="${filename%.[rR][pP][lL]}"
if [[ "$bmp_mode" == true ]]; then
    original_output="$game_dir/$base.$camera_number.$target_number.$frame_number.PDO.bmp"
    ported_output="$game_dir/$base.$camera_number.$target_number.$frame_number.PDD.bmp"
else
    original_output="$game_dir/$base.PDO"
    ported_output="$game_dir/$base.PDD"
fi

if [[ ! -f "$game_dir/$filename" ]]; then
    echo "Replay not found: $game_dir/$filename" >&2
    exit 2
fi

rm -f -- "$original_output" "$ported_output"

if [[ "$rebuild_exes" == true ]]; then
    if ! wine cmd.exe /D /S /C \
        "S: && cd S:\src\restunts && call makepixldump.bat"; then
        echo "PIXLDUMP build failed." >&2
        exit 1
    fi
fi

run_dosbox_exe() {
    local executable="$1"
    local command="$executable $filename $camera_number $target_number"

    if [[ "$bmp_mode" == true ]]; then
        command+=" $frame_argument"
    fi

    timeout --signal=KILL "${timeout_seconds}s" \
        dosbox-x \
        -silent \
        -conf "$config" \
        -c "mount c $game_dir" \
        -c "c:" \
        -c "$command" \
        -c "exit" \
        >/dev/null 2>&1

    local status=$?
    case $status in
        0)
            return 0
            ;;
        124|137)
            echo "$executable timed out after $timeout_seconds seconds." >&2
            return 1
            ;;
        *)
            echo "$executable failed with exit code $status." >&2
            return 1
            ;;
    esac
}

run_dosbox_exe "pixldumo.exe" || exit 1
run_dosbox_exe "pixldump.exe" || exit 1

if [[ ! -f "$original_output" ]]; then
    echo "PIXLDUMO.EXE did not produce $original_output." >&2
    exit 1
fi

if [[ ! -f "$ported_output" ]]; then
    echo "PIXLDUMP.EXE did not produce $ported_output." >&2
    exit 1
fi

if ! cmp -s -- "$original_output" "$ported_output"; then
    echo "Pixel output mismatch: $original_output != $ported_output" >&2
    exit 1
fi

echo "Pixel outputs match: $original_output"
