#!/usr/bin/env python3
"""Exercise loader and OMF normalization boundaries without an assembler."""

import importlib.util
from pathlib import Path
import struct
import tempfile
import unittest


SPEC = importlib.util.spec_from_file_location(
    "compare_dos_artifacts", Path(__file__).with_name("compare-dos-artifacts.py")
)
ARTIFACTS = importlib.util.module_from_spec(SPEC)
SPEC.loader.exec_module(ARTIFACTS)


def record(kind, payload):
    data = bytes([kind]) + struct.pack("<H", len(payload) + 1) + payload
    return data + bytes([-sum(data) & 255])


def name(text):
    data = text.encode("ascii")
    return bytes([len(data)]) + data


def omf(data=b"abcdefgh", data_records=None, fixups=b"", attributes=0x68, public_offset=0,
        uppercase_names=False):
    symbol = lambda text: name(text.upper() if uppercase_names else text)
    records = [record(0x80, name("module.asm")),
               record(0x96, symbol("") + symbol("Text") + symbol("Code")),
               record(0x98, bytes([attributes]) + struct.pack("<H", len(data)) + b"\2\3\1"),
               record(0x8C, symbol("external") + b"\0"),
               record(0x90, b"\0\1" + symbol("entry") + struct.pack("<H", public_offset) + b"\0")]
    if data_records is None:
        records.append(record(0xA0, b"\1\0\0" + data))
    else:
        records.extend(data_records)
    if fixups:
        records.append(record(0x9C, fixups))
    records.append(record(0x8A, b"\0"))
    return b"".join(records)


def mz(image=b"\0" * 16, relocations=(0, 4), minimum=0, trailer=b"", checksum=0):
    header_size = 64
    total = header_size + len(image)
    pages = (total + 511) // 512
    words = [0x5A4D, total % 512, pages, len(relocations), header_size // 16,
             minimum, 0xFFFF, 1, 0x100, checksum, 0, 0, 28, 0]
    header = bytearray(struct.pack("<14H", *words).ljust(header_size, b"\0"))
    for index, address in enumerate(relocations):
        struct.pack_into("<HH", header, 28 + index * 4, address & 15, address // 16)
    return header + image + trailer


class ComparisonTests(unittest.TestCase):
    def setUp(self):
        self.temporary = tempfile.TemporaryDirectory()
        self.directory = Path(self.temporary.name)

    def tearDown(self):
        self.temporary.cleanup()

    def parse(self, kind, data, name="input", **kwargs):
        path = self.directory / name
        path.write_bytes(data)
        return getattr(ARTIFACTS, "parse_" + kind)(path, **kwargs)

    def compare(self, kind, left, right):
        return getattr(ARTIFACTS, "compare_" + kind)(
            self.parse(kind, left, "left"), self.parse(kind, right, "right")
        )

    def test_mz_checks_loaded_bytes_and_allocation(self):
        self.assertFalse(self.compare("mz", mz(), mz(image=b"\1" + b"\0" * 15))["equivalent"])
        self.assertFalse(self.compare("mz", mz(), mz(minimum=1))["equivalent"])

    def test_mz_disjoint_relocation_order_commutes(self):
        result = self.compare("mz", mz(), mz(relocations=(4, 0)))
        self.assertTrue(result["equivalent"])
        self.assertFalse(result["relocation_order_equal"])

    def test_mz_preserves_relocation_multiplicity(self):
        self.assertFalse(self.compare("mz", mz(), mz(relocations=(0, 4, 4)))["equivalent"])

    def test_mz_overlapping_relocation_order_is_not_ignored(self):
        result = self.compare("mz", mz(relocations=(0, 1)), mz(relocations=(1, 0)))
        self.assertTrue(result["partially_overlapping_relocations"])
        self.assertFalse(result["equivalent"])

    def test_mz_preserves_raw_segment_offset_alias_representation(self):
        original = mz(image=b"\0" * 32, relocations=(16,))
        alias = bytearray(original)
        struct.pack_into("<HH", alias, 28, 16, 0)
        result = self.compare("mz", original, alias)
        self.assertTrue(result["relocation_multiset_equal"])
        self.assertFalse(result["relocation_entry_multiset_equal"])
        self.assertFalse(result["equivalent"])

    def test_mz_nonloaded_trailer_and_checksum_are_reported(self):
        result = self.compare("mz", mz(), mz(trailer=b"debug", checksum=42))
        self.assertTrue(result["equivalent"])
        self.assertFalse(result["file_bytes_equal"])
        self.assertEqual(result["right"]["trailer_bytes"], 5)

    def test_mz_rejects_out_of_bounds_relocation_and_header(self):
        with self.assertRaises(ValueError):
            self.parse("mz", mz(relocations=(15,)))
        with self.assertRaises(ValueError):
            self.parse("mz", mz()[:50])

    def test_omf_partition_does_not_change_segment(self):
        split = [record(0xA0, b"\1\0\0abcd"), record(0xA0, b"\1\4\0efgh")]
        self.assertTrue(self.compare("omf", omf(), omf(data_records=split))["equivalent"])

    def test_omf_lidata_and_ledata_have_equal_initialized_bytes(self):
        repeated = record(0xA2, b"\1\0\0" + struct.pack("<HH", 4, 0) + b"\2AB")
        self.assertTrue(self.compare("omf", omf(b"ABABABAB"),
                                     omf(b"ABABABAB", data_records=[repeated]))["equivalent"])

    def test_omf_resolves_fixup_threads(self):
        direct = b"\xc4\0\x56\1"
        threaded = b"\x08\1\x54\xc4\0\x8c"
        self.assertTrue(self.compare("omf", omf(fixups=direct),
                                     omf(fixups=threaded))["equivalent"])

    def test_omf_retains_fixup_displacement_and_segment_attributes(self):
        direct = b"\xc4\0\x56\1"
        displaced = b"\xc4\0\x52\1\1\0"
        self.assertFalse(self.compare("omf", omf(fixups=direct),
                                      omf(fixups=displaced))["equivalent"])
        self.assertFalse(self.compare("omf", omf(), omf(attributes=0x48))["equivalent"])

    def test_omf_preserves_overlapping_fixup_order(self):
        first = b"\xc4\0\x56\1"
        overlapping = b"\xc0\1\x56\1"
        separate = b"\xc4\4\x56\1"
        self.assertFalse(self.compare("omf", omf(fixups=first + overlapping),
                                      omf(fixups=overlapping + first))["equivalent"])
        self.assertTrue(self.compare("omf", omf(fixups=first + separate),
                                     omf(fixups=separate + first))["equivalent"])

    def test_omf_ignore_case_changes_names_only_and_reports_unmatched_segments(self):
        lower, upper = omf(), omf(uppercase_names=True)
        strict = self.compare("omf", lower, upper)
        self.assertFalse(strict["equivalent"])
        self.assertEqual(len(strict["left_only_segments"]), 1)
        self.assertEqual(len(strict["right_only_segments"]), 1)
        left = self.parse("omf", lower, "left", ignore_case=True)
        right = self.parse("omf", upper, "right", ignore_case=True)
        self.assertTrue(ARTIFACTS.compare_omf(left, right)["equivalent"])
        changed = self.parse("omf", omf(data=b"ABCDEFGH", uppercase_names=True),
                             "changed", ignore_case=True)
        self.assertFalse(ARTIFACTS.compare_omf(left, changed)["equivalent"])

    def test_omf_retains_public_offsets_and_initialization(self):
        self.assertFalse(self.compare("omf", omf(), omf(public_offset=1))["equivalent"])
        uninitialized = omf(b"\0" * 8, data_records=[])
        self.assertFalse(self.compare("omf", omf(b"\0" * 8), uninitialized)["equivalent"])

    def test_omf_retains_segment_declaration_order(self):
        prefix = record(0x80, name("module")) + record(
            0x96, name("") + name("ALPHA") + name("BETA") + name("CODE")
        )
        alpha = record(0x98, b"\x68\x08\0\2\4\1")
        beta = record(0x98, b"\x68\x08\0\3\4\1")
        end = record(0x8A, b"\0")
        result = self.compare("omf", prefix + alpha + beta + end, prefix + beta + alpha + end)
        self.assertEqual(result["different_fields"], ["segment_order"])

    def test_omf_unsupported_and_malformed_records_fail(self):
        with self.assertRaises(ValueError):
            self.parse("omf", record(0xB0, b""))
        broken = bytearray(omf())
        broken[4] ^= 1
        with self.assertRaises(ValueError):
            self.parse("omf", broken)
        with self.assertRaises(ValueError):
            self.parse("omf", omf()[:-1])
        with self.assertRaises(ValueError):
            self.parse("omf", omf(fixups=b"\xc4\0\x8c"))

    def test_omf_lidata_relocations_fail_explicitly(self):
        repeated = record(0xA2, b"\1\0\0" + struct.pack("<HH", 4, 0) + b"\2AB")
        with self.assertRaises(ValueError):
            self.parse("omf", omf(data_records=[repeated], fixups=b"\xc4\0\x56\1"))

    def test_omf_bounds_lidata_expansion_before_allocation(self):
        nested = struct.pack("<HH", 65535, 1) + struct.pack("<HH", 65535, 0) + b"\1X"
        repeated = record(0xA2, b"\1\0\0" + nested)
        with self.assertRaisesRegex(ValueError, "Oversized LIDATA"):
            self.parse("omf", omf(data_records=[repeated]))


if __name__ == "__main__":
    unittest.main()
