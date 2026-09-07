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

run_host_test test-race-flow legacy.c \
    "$test_source_dir/headless_data.c" "$test_source_dir/full_data.c" \
    -Wno-pointer-sign -Wno-missing-field-initializers
run_host_test test-car-menu menu_car.c \
    "$test_source_dir/legacy.c" "$test_source_dir/full_data.c" \
    "$test_source_dir/full_strings.c" "$test_source_dir/headless_data.c" \
    -Wno-pointer-sign -Wno-missing-field-initializers
run_host_test test-car-speed statecar.c \
    "$test_source_dir/math.c" "$test_source_dir/legacy.c"
run_host_test test-dashboard dashboard.c \
    "$test_source_dir/legacy.c" "$test_source_dir/full_data.c" \
    "$test_source_dir/full_strings.c" "$test_source_dir/headless_data.c" \
    -Wno-pointer-sign -Wno-missing-field-initializers
run_host_test test-frame-render math.c \
    "$test_source_dir/full_data.c" "$test_source_dir/headless_data.c" \
    "$test_source_dir/legacy.c" "$test_source_dir/heapsort.c" \
    -Wno-pointer-sign -Wno-sign-compare -Wno-missing-field-initializers -Wno-maybe-uninitialized
run_host_test test-intro-preview intro_render.c \
    "$test_source_dir/track_preview.c" "$test_source_dir/full_data.c" \
    "$test_source_dir/headless_data.c" "$test_source_dir/headless_trackdata.c" \
    "$test_source_dir/math.c" "$test_source_dir/legacy.c" "$test_source_dir/trkutil.c" \
    -DRESTUNTS_FULL -Wno-pointer-sign -Wno-missing-braces -Wno-missing-field-initializers
run_host_test test-end-hiscore highscore.c \
    "$test_source_dir/legacy.c" "$test_source_dir/full_data.c" \
    "$test_source_dir/full_strings.c" "$test_source_dir/headless_data.c" \
    -Wno-pointer-sign -Wno-missing-field-initializers
run_host_test test-gamestate-serialization stateio.c
run_host_test test-grip physics_grip.c \
    "$test_source_dir/math.c" "$test_source_dir/legacy.c" -Wno-sign-compare
run_host_test test-legacy-semantics legacy.c
run_host_test test-line-prepare shape3d_lines.c \
    "$test_source_dir/full_data.c" "$test_source_dir/math.c" "$test_source_dir/legacy.c" \
    -Wno-pointer-sign -Wno-unused-variable -Wno-missing-field-initializers
run_host_test test-matrix-semantics math.c
run_host_test test-math-boundaries math.c \
    "$test_source_dir/headless_data.c" "$test_source_dir/legacy.c"
run_host_test test-simulation-setup gameinit.c \
    "$test_source_dir/gamestep.c" "$test_source_dir/carsetup.c" \
    "$test_source_dir/math.c" "$test_source_dir/headless_data.c" \
    "$test_source_dir/legacy.c" -Wno-pointer-sign -Wno-sign-compare
run_host_test test-penalty-routing physics_grip.c -Wno-sign-compare
run_host_test test-wheel-suspension physics_collision.c
run_host_test test-route-points opponent.c "$test_source_dir/trkutil.c" -Wno-missing-braces
run_host_test test-memmgr-cache memmgr.c \
    -Wno-pointer-sign -Wno-unused-variable -Wno-missing-braces -Wno-missing-field-initializers
run_host_test test-opponent-tick opponent.c \
    "$test_source_dir/headless_data.c" "$test_source_dir/headless_trackdata.c" \
    "$test_source_dir/trkutil.c" "$test_source_dir/math.c" "$test_source_dir/legacy.c" \
    -Wno-missing-braces
run_host_test test-pixldump-md5 ../pixldump/md5.c
run_host_test test-polygon-edges shape3d_prerender.c \
    "$test_source_dir/full_data.c" "$test_source_dir/legacy.c" \
    -Wno-pointer-sign -Wno-unused-variable -Wno-missing-field-initializers
run_host_test test-replay-controls legacy.c \
    "$test_source_dir/headless_data.c" "$test_source_dir/full_data.c" "$test_source_dir/math.c" \
    -Wno-pointer-sign -Wno-missing-field-initializers
run_host_test test-replay-serialization replay.c
run_host_test test-resource-lookup resource.c
run_host_test test-shape2d-render shape2d.c \
    "$test_source_dir/shape2d_blit.c" "$test_source_dir/shape2d_resources.c" \
    "$test_source_dir/full_data.c" "$test_source_dir/resource.c" "$test_source_dir/legacy.c" \
    -Wno-pointer-sign -Wno-missing-braces -Wno-missing-field-initializers -Wno-unused-variable
run_host_test test-shape3d-raster shape3d_prerender.c \
    "$test_source_dir/shape3d_lines.c" "$test_source_dir/full_data.c" \
    "$test_source_dir/full_tables.c" "$test_source_dir/math.c" "$test_source_dir/legacy.c" \
    -Wno-pointer-sign -Wno-unused-variable -Wno-missing-field-initializers
run_host_test test-shape3d-queue shape3d.c \
    "$test_source_dir/full_data.c" "$test_source_dir/math.c" "$test_source_dir/legacy.c" \
    -Wno-pointer-sign -Wno-unused-variable -Wno-missing-field-initializers
run_host_test test-shape3d-vertices shape3d.c \
    -Wno-pointer-sign -Wno-unused-variable
run_host_test test-car-shape-lifetime shape3d_car.c \
    "$test_source_dir/shape3d_resources.c" "$test_source_dir/shape3d.c" \
    "$test_source_dir/full_data.c" "$test_source_dir/math.c" "$test_source_dir/legacy.c" \
    -Wno-pointer-sign -Wno-unused-variable -Wno-missing-field-initializers
run_host_test test-skybox-render skybox.c \
    "$test_source_dir/shape3d_lines.c" "$test_source_dir/full_data.c" \
    "$test_source_dir/math.c" "$test_source_dir/legacy.c" \
    -Wno-pointer-sign -Wno-unused-variable -Wno-missing-field-initializers
run_host_test test-simd-decoding simd.c
run_host_test test-track-editor legacy.c \
    "$test_source_dir/headless_data.c" "$test_source_dir/headless_trackdata.c" \
    "$test_source_dir/full_data.c" \
    -Wno-pointer-sign -Wno-missing-field-initializers -Wno-missing-braces
run_host_test test-track-object trackobj.c \
    "$test_source_dir/headless_data.c" "$test_source_dir/headless_trackdata.c" \
    "$test_source_dir/math.c" "$test_source_dir/legacy.c" "$test_source_dir/trkutil.c" \
    -Wno-missing-braces
run_host_test test-track-setup track_setup.c \
    "$test_source_dir/headless_data.c" "$test_source_dir/headless_trackdata.c" \
    "$test_source_dir/trkutil.c" "$test_source_dir/opponent.c" "$test_source_dir/legacy.c" \
    -Wno-missing-braces -Wno-type-limits -Wno-pointer-sign -Wno-unused-variable \
    -Wno-maybe-uninitialized
run_host_test test-track-resource-decoding trackres.c
run_host_test test-ui-dialog ui_dialog.c "$test_source_dir/legacy.c" -Wno-pointer-sign

echo "All host regression tests passed."
