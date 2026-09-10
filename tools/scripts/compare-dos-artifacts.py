#!/usr/bin/env python3
"""Compare DOS MZ load images or the emitted contents of bounded OMF objects.

OMF record definitions follow Open Watcom's pcobj.h and omfreloc.c:
https://github.com/open-watcom/open-watcom-v2/blob/master/bld/watcom/h/pcobj.h
https://github.com/open-watcom/open-watcom-v2/blob/master/bld/wl/c/omfreloc.c

OMF comparison ignores record partitioning, name-table indices, comments and
debug line/type records. It retains segment layout, initialized bytes, public
and external names, groups, and resolved FIXUPP threads. It is deliberately
strict about different instruction encodings and embedded relocation addends;
equal linked MZ images remain the stronger final check. Unsupported records,
iterated-data relocations, and malformed inputs fail instead of being ignored.
Conflicting overlapping data is also rejected, including compiler -d2 debug
sections that use this representation; compare linked MZ files in that case.
"""

import argparse
from collections import Counter
import hashlib
import json
from pathlib import Path
import struct
import sys


MAX_SEGMENT = 16 * 1024 * 1024
FIXUP_WIDTHS = {0: 1, 1: 2, 2: 2, 3: 4, 4: 1, 5: 2, 9: 4, 11: 6, 13: 4}


def sha(data):
    return hashlib.sha256(data).hexdigest()


def require(condition, message):
    if not condition:
        raise ValueError(message)


class Reader:
    def __init__(self, data):
        self.data = data
        self.offset = 0

    def take(self, size):
        require(size >= 0 and self.offset + size <= len(self.data), "Truncated record")
        result = self.data[self.offset:self.offset + size]
        self.offset += size
        return result

    def number(self, size=1):
        return int.from_bytes(self.take(size), "little")

    def index(self):
        first = self.number()
        return ((first & 0x7F) << 8) | self.number() if first & 0x80 else first

    def name(self):
        return self.take(self.number()).decode("latin1")

    def remaining(self):
        return len(self.data) - self.offset


def parse_mz(path):
    data = path.read_bytes()
    require(len(data) >= 28 and data[:2] == b"MZ", f"Not a DOS MZ executable: {path}")
    words = struct.unpack_from("<14H", data)
    _, last_page, pages, count, paragraphs, minimum, maximum, ss, sp, checksum, ip, cs, table, overlay = words
    require(pages > 0 and last_page < 512, "Invalid MZ page counts")
    declared_size = (pages - 1) * 512 + (last_page or 512)
    header_size = paragraphs * 16
    require(28 <= header_size <= declared_size <= len(data), "Invalid MZ image bounds")
    require(28 <= table and table + count * 4 <= header_size, "Invalid MZ relocation table")
    image = data[header_size:declared_size]
    require(len(image) <= 1024 * 1024, "MZ load image exceeds real-mode comparator bound")
    relocations = []
    relocation_entries = []
    for index in range(count):
        offset, segment = struct.unpack_from("<HH", data, table + 4 * index)
        address = segment * 16 + offset
        require(address + 2 <= len(image), "MZ relocation lies outside loaded image")
        relocations.append(address)
        relocation_entries.append((offset, segment))
    return {
        "file_sha256": sha(data),
        "file_bytes": len(data),
        "header_bytes": header_size,
        "checksum": checksum,
        "trailer_bytes": len(data) - declared_size,
        "trailer_sha256": sha(data[declared_size:]),
        "loader": {"minimum_paragraphs": minimum, "maximum_paragraphs": maximum,
                   "ss": ss, "sp": sp, "cs": cs, "ip": ip, "overlay": overlay},
        "image": image,
        "relocations": relocations,
        "relocation_entries": relocation_entries,
    }


def mz_summary(parsed):
    result = {key: value for key, value in parsed.items() if key != "image"}
    result["image_bytes"] = len(parsed["image"])
    result["image_sha256"] = sha(parsed["image"])
    return result


def differing_bytes(left, right, limit=16):
    result = []
    for offset in range(max(len(left), len(right))):
        a = left[offset] if offset < len(left) else None
        b = right[offset] if offset < len(right) else None
        if a != b:
            result.append({"offset": offset, "left": a, "right": b})
            if len(result) == limit:
                break
    return result


def compare_mz(left, right):
    order_equal = left["relocation_entries"] == right["relocation_entries"]
    multiset_equal = Counter(left["relocations"]) == Counter(right["relocations"])
    entries_equal = Counter(left["relocation_entries"]) == Counter(right["relocation_entries"])
    locations = set(left["relocations"]) | set(right["relocations"])
    overlap = any(address + 1 in locations for address in locations)
    # Additions to disjoint words commute. Duplicate entries are retained by
    # the multiset and add the load segment repeatedly. Partially overlapping
    # words require the original order because carries can change the result.
    # Also retain each raw offset:segment pair. Alternative aliases of the
    # same linear location can behave differently near segment/A20 wrap.
    relocation_equal = entries_equal and (order_equal or (multiset_equal and not overlap))
    image_equal = left["image"] == right["image"]
    loader_equal = left["loader"] == right["loader"]
    return {
        "equivalent": image_equal and loader_equal and relocation_equal,
        "file_bytes_equal": left["file_sha256"] == right["file_sha256"],
        "load_image_equal": image_equal,
        "loader_fields_equal": loader_equal,
        "relocation_order_equal": order_equal,
        "relocation_multiset_equal": multiset_equal,
        "relocation_entry_multiset_equal": entries_equal,
        "partially_overlapping_relocations": overlap,
        "relocation_semantics_equal": relocation_equal,
        "first_image_differences": differing_bytes(left["image"], right["image"]),
        "left": mz_summary(left),
        "right": mz_summary(right),
    }


class OMF:
    def __init__(self, ignore_case=False):
        self.ignore_case = ignore_case
        self.names = [None]
        self.segments = [None]
        self.groups = [None]
        self.externals = [None]
        self.publics = []
        self.fixups = []
        self.frame_threads = {}
        self.target_threads = {}
        self.last_data = None
        self.comments = Counter()
        self.record_counts = Counter()
        self.module_end = None
        self.total_segment_bytes = 0

    def canonical_name(self, name):
        if self.ignore_case:
            return name.translate(str.maketrans("abcdefghijklmnopqrstuvwxyz", "ABCDEFGHIJKLMNOPQRSTUVWXYZ"))
        return name

    def lookup(self, table, index):
        require(0 < index < len(table), f"Invalid OMF index {index}")
        return table[index]

    def segment_key(self, index):
        segment = self.lookup(self.segments, index)
        return (segment["name"], segment["class"], segment["overlay"])

    def reference(self, reader, method):
        if method == 0:
            return ("segment", self.segment_key(reader.index()))
        if method == 1:
            return ("group", self.lookup(self.groups, reader.index())["name"])
        if method == 2:
            return ("external", self.lookup(self.externals, reader.index()))
        if method == 3:
            return ("absolute", reader.number(2))
        if method in (4, 5, 6):
            return ("location", "target", "none")[method - 4],
        raise ValueError(f"Unsupported OMF reference method {method}")

    def fixdata(self, reader, wide):
        flags = reader.number()
        if flags & 0x80:
            key = (flags >> 4) & 3
            require(key in self.frame_threads, "Undefined OMF frame thread")
            frame = self.frame_threads[key]
        else:
            frame = self.reference(reader, (flags >> 4) & 7)
        if flags & 8:
            key = flags & 3
            require(key in self.target_threads, "Undefined OMF target thread")
            target = self.target_threads[key]
        else:
            target = self.reference(reader, flags & 3)
        displacement = 0 if flags & 4 else reader.number(4 if wide else 2)
        if frame == ("target",):
            frame = target
        return frame, target, displacement

    def iterated(self, reader, wide, depth=0):
        require(depth < 32, "Excessively nested LIDATA")
        repeat = reader.number(4 if wide else 2)
        blocks = reader.number(2)
        if blocks:
            parts = []
            length = 0
            for _ in range(blocks):
                part = self.iterated(reader, wide, depth + 1)
                length += len(part)
                require(length <= MAX_SEGMENT, "Oversized LIDATA block")
                parts.append(part)
            unit = b"".join(parts)
        else:
            unit = reader.take(reader.number())
        require(len(unit) * repeat <= MAX_SEGMENT, "Oversized LIDATA expansion")
        return unit * repeat

    def read_record(self, record_type, payload):
        self.record_counts[f"{record_type:02x}"] += 1
        reader = Reader(payload)
        wide = bool(record_type & 1)
        base = record_type & ~1
        if record_type in (0x80, 0x82, 0x8E, 0x94, 0x95):
            return  # Module source names and debug type/line records.
        if record_type == 0x88:
            reader.number()
            self.comments[f"{reader.number():02x}"] += 1
            return  # Report their classes; this is an emitted-object comparator.
        if record_type == 0x96:
            while reader.remaining():
                self.names.append(self.canonical_name(reader.name()))
        elif base == 0x98:
            attributes = reader.number()
            absolute = [reader.number(2), reader.number()] if attributes >> 5 == 0 else None
            length = reader.number(4 if wide else 2)
            if attributes & 2:
                length = 1 << (32 if wide else 16)
            require(length <= MAX_SEGMENT, "OMF segment exceeds comparator bound")
            self.total_segment_bytes += length
            require(self.total_segment_bytes <= MAX_SEGMENT, "OMF module exceeds comparator bound")
            name, segment_class, overlay = [self.lookup(self.names, reader.index()) for _ in range(3)]
            self.segments.append({"name": name, "class": segment_class, "overlay": overlay,
                                  "attributes": attributes, "absolute": absolute,
                                  "length": length, "data": bytearray(length),
                                  "initialized": bytearray(length)})
        elif record_type == 0x9A:
            name = self.lookup(self.names, reader.index())
            members = []
            while reader.remaining():
                require(reader.number() == 0xFF, "Unsupported OMF group component")
                members.append(self.segment_key(reader.index()))
            self.groups.append({"name": name, "segments": members})
        elif record_type in (0x8C, 0xB4):
            while reader.remaining():
                name = self.canonical_name(reader.name())
                reader.index()  # Debug type index.
                self.externals.append(("local" if record_type == 0xB4 else "global", name))
        elif base in (0x90, 0xB6):
            group = reader.index()
            segment = reader.index()
            frame = reader.number(2) if segment == 0 else None
            group_name = self.lookup(self.groups, group)["name"] if group else None
            segment_name = self.segment_key(segment) if segment else None
            while reader.remaining():
                name = self.canonical_name(reader.name())
                offset = reader.number(4 if wide else 2)
                reader.index()
                self.publics.append({"name": name, "local": base == 0xB6, "group": group_name,
                                     "segment": segment_name, "frame": frame, "offset": offset})
        elif base in (0xA0, 0xA2):
            index = reader.index()
            offset = reader.number(4 if wide else 2)
            segment = self.lookup(self.segments, index)
            if base == 0xA2:
                pieces = []
                total = 0
                while reader.remaining():
                    piece = self.iterated(reader, wide)
                    total += len(piece)
                    require(total <= MAX_SEGMENT, "Oversized LIDATA record")
                    pieces.append(piece)
                data = b"".join(pieces)
            else:
                data = reader.take(reader.remaining())
            require(offset + len(data) <= segment["length"], "OMF data exceeds segment")
            for position, value in enumerate(data, offset):
                require(not segment["initialized"][position] or segment["data"][position] == value,
                        "Conflicting overlapping OMF data")
                segment["data"][position] = value
                segment["initialized"][position] = 1
            self.last_data = (index, offset, len(data), base == 0xA2)
        elif base == 0x9C:
            while reader.remaining():
                first = reader.number()
                if first & 0x80:
                    require(self.last_data is not None, "FIXUPP without preceding data")
                    segment, start, length, iterated = self.last_data
                    require(not iterated, "Relocations within LIDATA are unsupported")
                    location = ((first & 3) << 8) | reader.number()
                    kind = (first >> 2) & 15
                    require(kind in FIXUP_WIDTHS and location + FIXUP_WIDTHS[kind] <= length,
                            "Unsupported or out-of-bounds OMF fixup location")
                    frame, target, displacement = self.fixdata(reader, wide)
                    if frame == ("location",):
                        frame = ("segment", self.segment_key(segment))
                    self.fixups.append({"segment": self.segment_key(segment),
                                        "offset": start + location, "kind": kind,
                                        "segment_relative": bool(first & 0x40), "frame": frame,
                                        "target": target, "displacement": displacement})
                elif first & 0x40:
                    self.frame_threads[first & 3] = self.reference(reader, (first >> 2) & 7)
                else:
                    self.target_threads[first & 3] = self.reference(reader, (first >> 2) & 3)
        elif base == 0x8A:
            flags = reader.number()
            require(not (flags & 0x40) or flags & 1, "Physical MODEND start address unsupported")
            start = self.fixdata(reader, wide) if flags & 0x40 else None
            self.module_end = {"main": bool(flags & 0x80), "start": start}
        else:
            raise ValueError(f"Unsupported OMF record 0x{record_type:02x}")
        require(reader.remaining() == 0, f"Unparsed bytes in OMF record 0x{record_type:02x}")

    def normalized(self):
        segments = []
        keys = []
        for index, segment in enumerate(self.segments[1:], 1):
            key = self.segment_key(index)
            require(key not in keys, f"Duplicate segment identity: {key}")
            keys.append(key)
            segments.append({k: bytes(v).hex() if k in ("data", "initialized") else v
                             for k, v in segment.items()})
        sort = lambda values: sorted(values, key=lambda value: json.dumps(value, sort_keys=True))
        occupied = set()
        overlapping = False
        for fixup in self.fixups:
            locations = {(fixup["segment"], fixup["offset"] + byte)
                         for byte in range(FIXUP_WIDTHS[fixup["kind"]])}
            overlapping |= bool(occupied & locations)
            occupied.update(locations)
        return {"segments": sort(segments), "segment_order": keys,
                "groups": sort(self.groups[1:]),
                "externals": sort(self.externals[1:]), "publics": sort(self.publics),
                "fixups": self.fixups if overlapping else sort(self.fixups),
                "overlapping_fixups": overlapping, "module_end": self.module_end}


def parse_omf(path, ignore_case=False):
    data = path.read_bytes()
    reader = Reader(data)
    module = OMF(ignore_case)
    while reader.remaining():
        require(module.module_end is None, "Trailing bytes after OMF MODEND")
        start = reader.offset
        record_type = reader.number()
        size = reader.number(2)
        require(size >= 1, "Empty OMF record")
        payload = reader.take(size)
        # OMF explicitly allows a zero checksum byte to mean not supplied.
        require(payload[-1] == 0 or sum(data[start:reader.offset]) & 255 == 0,
                "Invalid OMF record checksum")
        module.read_record(record_type, payload[:-1])
    require(module.module_end is not None, "Missing OMF MODEND")
    return {"file_sha256": sha(data), "file_bytes": len(data),
            "ascii_names_ignore_case": ignore_case,
            "records": dict(module.record_counts), "ignored_comment_classes": dict(module.comments),
            "normalized": module.normalized()}


def compare_omf(left, right):
    differences = []
    for field in left["normalized"]:
        if left["normalized"][field] != right["normalized"][field]:
            differences.append(field)
    segment_differences = []
    def by_name(parsed):
        return {(s["name"], s["class"], s["overlay"]): s
                for s in parsed["normalized"]["segments"]}
    a, b = by_name(left), by_name(right)
    for key in sorted(a.keys() & b.keys()):
        if a[key] != b[key]:
            segment_differences.append({"segment": key,
                "left_length": a[key]["length"], "right_length": b[key]["length"],
                "first_data_differences": differing_bytes(bytes.fromhex(a[key]["data"]),
                                                          bytes.fromhex(b[key]["data"]))})
    return {"equivalent": not differences, "comparison_scope": "emitted OMF; comments/debug ignored",
            "different_fields": differences, "segment_differences": segment_differences,
            "left_only_segments": sorted(a.keys() - b.keys()),
            "right_only_segments": sorted(b.keys() - a.keys()),
            "left": left, "right": right}


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("kind", choices=("mz", "omf"))
    parser.add_argument("left", type=Path)
    parser.add_argument("right", type=Path)
    parser.add_argument("--output", type=Path, help="Write full JSON, including normalized OMF data")
    parser.add_argument("--ignore-case", action="store_true",
                        help="OMF only: compare ASCII names case-insensitively; retain exact data bytes")
    args = parser.parse_args()
    if args.ignore_case and args.kind != "omf":
        parser.error("--ignore-case is only valid for OMF comparisons")
    try:
        if args.kind == "mz":
            result = compare_mz(parse_mz(args.left), parse_mz(args.right))
        else:
            result = compare_omf(parse_omf(args.left, args.ignore_case),
                                 parse_omf(args.right, args.ignore_case))
    except (ValueError, OSError, struct.error) as error:
        print(f"Comparison failed: {error}", file=sys.stderr)
        return 2
    if args.output:
        args.output.parent.mkdir(parents=True, exist_ok=True)
        args.output.write_text(json.dumps(result, indent=2) + "\n")
    brief = {key: value for key, value in result.items() if key not in ("left", "right")}
    print(json.dumps(brief, indent=2))
    return 0 if result["equivalent"] else 1


if __name__ == "__main__":
    sys.exit(main())
