#!/usr/bin/env bash
# Install the pinned Windows and Linux tools without running the GUI installer.
set -euo pipefail

script_dir=$(cd -- "$(dirname -- "${BASH_SOURCE[0]}")" && pwd)
repo_dir=$(cd -- "$script_dir/../.." && pwd)
# This repository-owned file also supplies the PowerShell installer's pin.
source "$script_dir/open-watcom.conf"

destination="$repo_dir/tools/watcom"
marker="$destination/restunts-toolchain.version"
if [[ -f "$marker" ]] && [[ $(tr -d '\r\n' < "$marker") == "$OW2_RELEASE $OW2_SHA256" ]] \
    && [[ -f "$destination/binnt/wcc.exe" && -f "$destination/binnt/wlink.exe" ]] \
    && [[ -x "$destination/binl64/wcc" && -x "$destination/binl64/wlink" ]]; then
    echo "Open Watcom 2 $OW2_RELEASE is already installed in $destination"
    exit 0
fi
if [[ -e "$destination" ]]; then
    echo "Refusing to replace $destination; move it aside before installing." >&2
    exit 1
fi

staging=$(mktemp -d "$repo_dir/tools/.watcom-install.XXXXXXXX")
trap 'rm -rf -- "$staging"' EXIT
archive="$staging/ow-snapshot.tar.xz"
url="https://github.com/open-watcom/open-watcom-v2/releases/download/$OW2_RELEASE/ow-snapshot.tar.xz"
curl --fail --location --retry 3 --output "$archive" "$url"
printf '%s  %s\n' "$OW2_SHA256" "$archive" | sha256sum --check --status
mkdir "$staging/watcom"
tar -xJf "$archive" -C "$staging/watcom" \
    ./binnt ./binl64 ./h ./lib286 ./license.txt ./readme.txt
test -f "$staging/watcom/binnt/wcc.exe"
test -f "$staging/watcom/binnt/wlink.exe"
test -x "$staging/watcom/binl64/wcc"
test -x "$staging/watcom/binl64/wlink"
printf '%s %s\n' "$OW2_RELEASE" "$OW2_SHA256" > "$staging/watcom/restunts-toolchain.version"
mv -- "$staging/watcom" "$destination"
echo "Installed Open Watcom 2 $OW2_RELEASE in $destination"
