#!/usr/bin/env python3
"""Bind Watcom C references to the unchanged original game's OMF exports.

Only symbol records are needed. Debug data may overlap and is intentionally
not normalized here; both release and debug objects use the same linkage.
"""

import argparse
from pathlib import Path
import re


MAX_OBJECT_BYTES = 16 * 1024 * 1024
SYMBOL = re.compile(r"[A-Za-z_$?][A-Za-z0-9_$?]*\Z")


class SymbolError(ValueError):
    pass


class Reader:
    def __init__(self, data):
        self.data = data
        self.offset = 0

    def take(self, count):
        if count < 0 or self.offset + count > len(self.data):
            raise SymbolError("truncated OMF record")
        result = self.data[self.offset:self.offset + count]
        self.offset += count
        return result

    def number(self, count=1):
        return int.from_bytes(self.take(count), "little")

    def index(self):
        value = self.number()
        return ((value & 127) << 8) | self.number() if value & 128 else value

    def name(self):
        return self.take(self.number()).decode("latin1")

    def remaining(self):
        return len(self.data) - self.offset


def read_symbols(path):
    """Return global public and external names, validating record boundaries."""
    if path.stat().st_size > MAX_OBJECT_BYTES:
        raise SymbolError(f"{path}: object exceeds size bound")
    data = path.read_bytes()
    reader = Reader(data)
    publics = set()
    externals = set()
    ended = False
    first = True
    while reader.remaining():
        if ended:
            raise SymbolError(f"{path}: data after OMF module end")
        start = reader.offset
        kind = reader.number()
        size = reader.number(2)
        if size == 0:
            raise SymbolError(f"{path}: empty OMF record")
        payload = reader.take(size)
        if payload[-1] and sum(data[start:reader.offset]) & 255:
            raise SymbolError(f"{path}: invalid OMF checksum")
        record = Reader(payload[:-1])
        if first and kind not in (0x80, 0x82):
            raise SymbolError(f"{path}: missing OMF module header")
        first = False
        if kind in (0x80, 0x82):
            record.name()
        elif kind in (0x90, 0x91):
            record.index()  # Group index.
            if record.index() == 0:  # Absolute public includes a frame number.
                record.number(2)
            while record.remaining():
                publics.add(record.name())
                record.number(4 if kind == 0x91 else 2)
                record.index()  # Debug type index.
        elif kind == 0x8C:
            while record.remaining():
                externals.add(record.name())
                record.index()
        elif kind in (0x8A, 0x8B):
            record.number()  # Module attributes; optional start address follows.
            ended = True
        # Local symbols, debug records and emitted bytes cannot add link names.
    if not ended:
        raise SymbolError(f"{path}: missing OMF module end")
    return publics, externals


def collect_symbols(paths):
    publics = {}
    externals = set()
    for path in paths:
        defined, referenced = read_symbols(path)
        for name in sorted(defined):
            publics.setdefault(name.upper(), name)
        externals.update(referenced)
    return publics, externals


def make_aliases(original, compiled, referenced):
    """Map one C decoration prefix only when the target is an ASM export."""
    if "_STUNTSMAIN" not in compiled:
        raise SymbolError("wrapper does not define _stuntsmain")
    if "STUNTSMAIN" in original or "STUNTSMAIN" in compiled:
        raise SymbolError("original startup's stuntsmain is already defined")
    aliases = {"stuntsmain": compiled["_STUNTSMAIN"]}
    for name in sorted(referenced):
        folded = name.upper()
        if folded in compiled or folded in original or not name.startswith("_"):
            continue
        target = original.get(folded[1:])
        if target is not None:
            aliases[name] = target
    for name, target in aliases.items():
        if not SYMBOL.fullmatch(name) or not SYMBOL.fullmatch(target):
            raise SymbolError(f"unsupported linker symbol spelling: {name!r}={target!r}")
    return aliases


def generate(asm_dir, objects, output):
    original_objects = sorted(asm_dir.glob("*.obj"))
    if not original_objects:
        raise SymbolError(f"{asm_dir}: no original assembly objects")
    original, _ = collect_symbols(original_objects)
    compiled, referenced = collect_symbols(objects)
    aliases = make_aliases(original, compiled, referenced)
    lines = ["# Generated original-game C linkage; do not edit."]
    lines.extend(f"alias {name}={target}" for name, target in sorted(aliases.items()))
    output.parent.mkdir(parents=True, exist_ok=True)
    output.write_bytes(("\r\n".join(lines) + "\r\n").encode("ascii"))
    return aliases


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--asm-dir", required=True, type=Path)
    parser.add_argument("--output", required=True, type=Path)
    parser.add_argument("objects", nargs="+", type=Path)
    args = parser.parse_args()
    try:
        generate(args.asm_dir, args.objects, args.output)
    except (OSError, SymbolError) as error:
        parser.exit(1, f"original linkage: {error}\n")


if __name__ == "__main__":
    main()
