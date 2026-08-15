#!/bin/sh

set -eu

usage() {
    echo "Usage: DUMPSRV_API_KEY=<secret> $0 <service-url>" >&2
    echo "Example: DUMPSRV_API_KEY=<secret> $0 http://server:8080/process" >&2
}

if [ "$#" -gt 1 ]; then
    usage
    exit 2
fi

service_url=${1:-${DUMPSRV_URL:-}}
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
input_file=$script_directory/REPLDUMP.EXE
output_file=$script_directory/partitions_all.txt
temporary_output=

cleanup() {
    if [ -n "$temporary_output" ] && [ -f "$temporary_output" ]; then
        rm -f -- "$temporary_output"
    fi
}

trap cleanup 0 HUP INT TERM

if [ ! -f "$input_file" ]; then
    echo "Upload file not found: $input_file" >&2
    exit 2
fi

temporary_output=$(mktemp "$script_directory/.partitions_all.txt.XXXXXX")

if ! curl \
    --silent \
    --show-error \
    --fail \
    --connect-timeout 10 \
    --max-time 2100 \
    --request POST \
    --header "X-API-Key: $api_key" \
    --header "Content-Type: application/octet-stream" \
    --data-binary "@$input_file" \
    --output "$temporary_output" \
    "$service_url"; then
    echo "Request failed; the existing result was not changed." >&2
    exit 1
fi

mv -- "$temporary_output" "$output_file"
temporary_output=

echo "Saved result to $output_file"
