"""Exercise original-assembly linkage without requiring either compiler."""

import importlib.util
from pathlib import Path
import tempfile
import unittest


SCRIPT = Path(__file__).resolve().parents[1] / "generate-original-link-aliases.py"
SPEC = importlib.util.spec_from_file_location("original_link_aliases", SCRIPT)
ALIASES = importlib.util.module_from_spec(SPEC)
SPEC.loader.exec_module(ALIASES)


def name(value):
    data = value.encode("ascii")
    return bytes([len(data)]) + data


def record(kind, payload):
    data = bytes([kind]) + (len(payload) + 1).to_bytes(2, "little") + payload
    return data + bytes([-sum(data) & 255])


def module(publics=(), externals=(), extra=b""):
    data = record(0x80, name("test"))
    if publics:
        data += record(0x90, b"\x00\x01" + b"".join(
            name(symbol) + b"\x00\x00\x00" for symbol in publics))
    if externals:
        data += record(0x8C, b"".join(name(symbol) + b"\x00" for symbol in externals))
    return data + extra + record(0x8A, b"\x00")


class OriginalLinkAliasTests(unittest.TestCase):
    def parse(self, data):
        with tempfile.TemporaryDirectory() as directory:
            path = Path(directory) / "test.obj"
            path.write_bytes(data)
            return ALIASES.read_symbols(path)

    def test_global_symbols_and_debug_records(self):
        # Overlapping data is legal for the compiler's debug sections and
        # must not affect symbol extraction or require image normalization.
        debug = record(0xA0, b"\x01\x00\x00first") + record(0xA0, b"\x01\x00\x00again")
        publics, externals = self.parse(module(("_stuntsmain",), ("_state",), debug))
        self.assertEqual(publics, {"_stuntsmain"})
        self.assertEqual(externals, {"_state"})

    def test_absolute_wide_public_and_multibyte_indexes(self):
        payload = b"\x00\x00\x34\x12" + name("absolute") + b"\x78\x56\x34\x12\x80\x01"
        data = record(0x80, name("test")) + record(0x91, payload) + record(0x8A, b"\x00")
        self.assertEqual(self.parse(data)[0], {"absolute"})

    def test_local_symbols_do_not_participate_in_linkage(self):
        extra = record(0xB4, name("_local_reference") + b"\x00")
        extra += record(0xB6, b"\x00\x01" + name("_local_definition") + b"\x00\x00\x00")
        self.assertEqual(self.parse(module(extra=extra)), (set(), set()))

    def test_zero_checksum_is_permitted_by_omf(self):
        data = bytearray(module(("public",)))
        data[8] = 0  # THEADR checksum.
        self.assertEqual(self.parse(data)[0], {"public"})

    def test_bad_checksum_is_rejected(self):
        data = bytearray(module())
        data[4] ^= 1
        with self.assertRaisesRegex(ALIASES.SymbolError, "checksum"):
            self.parse(data)

    def test_truncation_and_missing_end_are_rejected(self):
        for data in (module()[:-1], record(0x80, name("test")), b"\x80\x10"):
            with self.subTest(data=data), self.assertRaises(ALIASES.SymbolError):
                self.parse(data)

    def test_missing_header_and_trailing_module_are_rejected(self):
        for data in (record(0x8A, b"\x00"), module() + module()):
            with self.subTest(data=data), self.assertRaises(ALIASES.SymbolError):
                self.parse(data)

    def test_only_original_exports_receive_decoration_aliases(self):
        original = {"INIT_MAIN": "INIT_MAIN", "STATE": "state", "_PRINTF": "_PRINTF"}
        compiled = {"_STUNTSMAIN": "_stuntsmain", "__MEMCPY": "__memcpy"}
        referenced = {"_init_main", "_state", "__printf", "__memcpy", "__U4D",
                      "_dos_file_write", "_pixldump_md5", "__state"}
        self.assertEqual(ALIASES.make_aliases(original, compiled, referenced), {
            "stuntsmain": "_stuntsmain", "_init_main": "INIT_MAIN",
            "_state": "state", "__printf": "_PRINTF"})

    def test_existing_compiled_and_original_definitions_take_precedence(self):
        original = {"FUNCTION": "function", "_EXISTING": "_existing", "EXISTING": "existing"}
        compiled = {"_STUNTSMAIN": "_stuntsmain", "_FUNCTION": "_function"}
        self.assertEqual(ALIASES.make_aliases(original, compiled, {"_function", "_existing"}),
                         {"stuntsmain": "_stuntsmain"})

    def test_startup_definition_is_required_and_collisions_are_rejected(self):
        cases = [({}, {}), ({"STUNTSMAIN": "stuntsmain"}, {"_STUNTSMAIN": "_stuntsmain"}),
                 ({}, {"STUNTSMAIN": "stuntsmain", "_STUNTSMAIN": "_stuntsmain"})]
        for original, compiled in cases:
            with self.subTest(original=original), self.assertRaises(ALIASES.SymbolError):
                ALIASES.make_aliases(original, compiled, set())

    def test_response_file_injection_is_rejected(self):
        with self.assertRaisesRegex(ALIASES.SymbolError, "spelling"):
            ALIASES.make_aliases({"STATE\nFILE": "state\nfile"},
                                 {"_STUNTSMAIN": "_stuntsmain"}, {"_state\nfile"})

    def test_generation_is_deterministic_and_preserves_inputs(self):
        with tempfile.TemporaryDirectory() as directory:
            root = Path(directory)
            asm = root / "asm"
            asm.mkdir()
            original = module(("state",), ("stuntsmain",))
            wrapper = module(("_stuntsmain",), ("_state", "__U4M"))
            (asm / "game.obj").write_bytes(original)
            (root / "wrapper.obj").write_bytes(wrapper)
            output = root / "generated" / "aliases.lnk"
            ALIASES.generate(asm, [root / "wrapper.obj"], output)
            first = output.read_bytes()
            ALIASES.generate(asm, [root / "wrapper.obj"], output)
            self.assertEqual(first, output.read_bytes())
            self.assertIn(b"alias _state=state\r\n", first)
            self.assertNotIn(b"__U4M", first)
            self.assertNotIn(b"\n", first.replace(b"\r\n", b""))
            self.assertEqual((asm / "game.obj").read_bytes(), original)
            self.assertEqual((root / "wrapper.obj").read_bytes(), wrapper)


if __name__ == "__main__":
    unittest.main()
