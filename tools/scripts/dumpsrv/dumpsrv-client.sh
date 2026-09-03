#!/bin/sh

set -eu

usage() {
    echo "Usage: DUMPSRV_API_KEY=<secret> $0 [--physics-tests true|false] [--renderer-tests true|false] [service-url]" >&2
    echo "Example: DUMPSRV_API_KEY=<secret> $0 --physics-tests false http://server:8080/process" >&2
}

service_url=
physics_tests=true
renderer_tests=true

while [ "$#" -gt 0 ]; do
    case "$1" in
        --physics-tests)
            if [ "$#" -lt 2 ]; then
                echo "--physics-tests requires true or false." >&2
                usage
                exit 2
            fi
            physics_tests=$2
            shift 2
            ;;
        --physics-tests=*)
            physics_tests=${1#*=}
            shift
            ;;
        --renderer-tests)
            if [ "$#" -lt 2 ]; then
                echo "--renderer-tests requires true or false." >&2
                usage
                exit 2
            fi
            renderer_tests=$2
            shift 2
            ;;
        --renderer-tests=*)
            renderer_tests=${1#*=}
            shift
            ;;
        -h|--help)
            usage
            exit 0
            ;;
        -*)
            echo "Unknown option: $1" >&2
            usage
            exit 2
            ;;
        *)
            if [ -n "$service_url" ]; then
                echo "Only one service URL may be provided." >&2
                usage
                exit 2
            fi
            service_url=$1
            shift
            ;;
    esac
done

case "$physics_tests" in
    true|false)
        ;;
    *)
        echo "--physics-tests must be true or false." >&2
        usage
        exit 2
        ;;
esac

case "$renderer_tests" in
    true|false)
        ;;
    *)
        echo "--renderer-tests must be true or false." >&2
        usage
        exit 2
        ;;
esac

service_url=${service_url:-${DUMPSRV_URL:-}}
api_key=${DUMPSRV_API_KEY:-}

if [ -z "$service_url" ]; then
    echo "Set DUMPSRV_URL or provide the service URL as the first argument." >&2
    usage
    exit 2
fi

if [ -z "$api_key" ]; then
    echo "DUMPSRV_API_KEY is not set." >&2
    exit 2
fi

if ! command -v curl >/dev/null 2>&1; then
    echo "curl is required." >&2
    exit 2
fi

script_directory=$(CDPATH= cd -- "$(dirname -- "$0")" && pwd)
repldump_file=$script_directory/REPLDUMP.EXE
pixldump_file=$script_directory/PIXLDUMP.EXE
output_file=$script_directory/partitions_all.txt
temporary_output=

cleanup() {
    if [ -n "$temporary_output" ] && [ -f "$temporary_output" ]; then
        rm -f -- "$temporary_output"
    fi
}

trap cleanup 0 HUP INT TERM

for input_file in "$repldump_file" "$pixldump_file"; do
    if [ ! -f "$input_file" ]; then
        echo "Upload file not found: $input_file" >&2
        exit 2
    fi
done

temporary_output=$(mktemp "$script_directory/.partitions_all.txt.XXXXXX")

if ! curl \
    --silent \
    --show-error \
    --fail \
    --connect-timeout 10 \
    --max-time 2100 \
    --request POST \
    --header "X-API-Key: $api_key" \
    --form "repldump=@$repldump_file;type=application/octet-stream;filename=REPLDUMP.EXE" \
    --form "pixldump=@$pixldump_file;type=application/octet-stream;filename=PIXLDUMP.EXE" \
    --form-string "physics_tests=$physics_tests" \
    --form-string "renderer_tests=$renderer_tests" \
    --output "$temporary_output" \
    "$service_url"; then
    echo "Request failed; the existing result was not changed." >&2
    exit 1
fi

mv -- "$temporary_output" "$output_file"
temporary_output=

echo "Saved result to $output_file"
