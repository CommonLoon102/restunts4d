#!/bin/bash

set -euo pipefail

test_script_dir="$(cd -- "$(dirname -- "${BASH_SOURCE[0]}")" && pwd)"
test_source_dir="$test_script_dir/../c"
test_build_dir="$(mktemp -d)"
test_compiler="${CC:-gcc}"

cleanup_test_build_dir() {
    rm -rf -- "$test_build_dir"
}
trap cleanup_test_build_dir EXIT

test_compile_flags=(
    -std=c99
    -O2
    -Wall
    -Wextra
    -ffunction-sections
    -fdata-sections
)
test_link_flags=(-Wl,--gc-sections)

run_host_test() {
    local test_name="$1"
    local source_file="$2"
    shift 2

    echo "Building $test_name"
    "$test_compiler" "${test_compile_flags[@]}" "$@" \
        "$test_script_dir/$test_name.c" "$test_source_dir/$source_file" \
        "${test_link_flags[@]}" -o "$test_build_dir/$test_name"

    echo "Running $test_name"
    "$test_build_dir/$test_name"
}

run_host_test test-gamestate-serialization stateio.c
run_host_test test-legacy-semantics legacy.c
run_host_test test-matrix-semantics math.c
run_host_test test-options options.c "$test_source_dir/strlib.c"
run_host_test test-penalty-route penaltyroute.c
run_host_test test-replay-serialization replay.c
run_host_test test-resource-lookup resource.c
run_host_test test-shape3d-vertices shape3d.c \
    -Wno-pointer-sign -Wno-unused-variable
run_host_test test-simd-decoding simd.c
run_host_test test-track-resource-decoding trackres.c

echo "All host regression tests passed."
