#!/usr/bin/env python3
"""Check the pixel-dump address model against the linked Borland oracle."""

import argparse
from pathlib import Path
import re
import struct


ROOT = Path(__file__).resolve().parents[2]
CONTEXT = ROOT / "src/restunts/pixldump/legacy_context.c"


def check_layout(executable):
    data = executable.read_bytes()
    if len(data) < 28 or data[:2] != b"MZ":
        raise ValueError("The renderer oracle is not a DOS MZ executable")
    header = struct.unpack_from("<14H", data)
    image = data[header[4] * 16:]
    entry = header[11] * 16 + header[10]
    startup = image[entry:entry + 40]
    # asmorig/seg010.asm: DOS version check, relocated dseg/endseg operands,
    # then the original CRT's fixed adjustment from MZ SP to its data segment.
    if (startup[:11] != bytes.fromhex("b430cd213c027302cd20bf")
            or startup[13:14] != b"\xbe"
            or startup[31:33] != b"\x81\xc4"):
        raise ValueError("The original CRT layout changed; review the pixel-dump address model")

    constants = dict(re.findall(r"^#define (PIXLDUMP_LEGACY_\w+) (\d+)U$",
                                CONTEXT.read_text(), re.MULTILINE))
    observed = {
        "PIXLDUMP_LEGACY_IMAGE_PARAGRAPHS": struct.unpack_from("<H", startup, 14)[0],
        "PIXLDUMP_LEGACY_INITIAL_STACK_POINTER":
            (header[8] + struct.unpack_from("<H", startup, 33)[0]) & 0xfffe,
    }
    for name, value in observed.items():
        expected = int(constants[name])
        if value != expected:
            raise ValueError(f"{name}: model {expected:#06x}, linked oracle {value:#06x}")

    # get_a_poly_info's fixed original offset and prologue independently check
    # the segment used when modeling far return addresses left by rendering.
    polygon_entry = int(constants["PIXLDUMP_LEGACY_POLYGON_CODE_PARAGRAPH"]) * 16 + 0x1296
    if image[polygon_entry:polygon_entry + 13] != bytes.fromhex("558bec83ec405756bf90012bf6"):
        raise ValueError("The modeled polygon code segment does not locate get_a_poly_info")


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--executable", type=Path, default=ROOT / "stunts/pixldumo.exe")
    args = parser.parse_args()
    try:
        check_layout(args.executable)
    except (OSError, ValueError, KeyError, struct.error) as error:
        parser.exit(1, f"Renderer legacy layout check failed: {error}\n")
    print("Linked Borland renderer matches the pixel-dump address model.")


if __name__ == "__main__":
    main()
