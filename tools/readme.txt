The tools directory contains the bundled assembler, make, DOSBox, and setup
scripts for the active Open Watcom 2 C compiler and linker.

Install the pinned toolchain from the repository root:
  Windows: powershell -ExecutionPolicy Bypass -File tools\scripts\install-open-watcom.ps1
  Linux:   tools/scripts/install-open-watcom.sh

Both installers verify the SHA256 in scripts/open-watcom.conf and extract into
ignored tools/watcom. A different or incomplete installation must be moved
aside before reinstalling.

setpath.bat puts watcom\binnt before bin on PATH, sets WATCOM to the pinned
installation, and selects its C headers. TASM32 and GNU Make remain bundled.
mount_stunts_to_s.bat maps the repository to S:, which the makefiles require.

The older bcc, wlink, tlink, include, and lib files are historical tools. The
current C builds use Open Watcom headers/runtime libraries under watcom.
The immutable pre-migration regression binaries live in oracles/borland.

See the root readme.md for build commands and regression validation.
