"""Check scenery repair without changing track geometry or replay inputs."""

import importlib.util
from pathlib import Path
import subprocess
import sys
import tempfile
import unittest
import zipfile


SCRIPT = Path(__file__).with_name("normalize-track-skybox.py")
SPEC = importlib.util.spec_from_file_location("skybox_normalizer", SCRIPT)
normalizer = importlib.util.module_from_spec(SPEC)
sys.dont_write_bytecode = True
SPEC.loader.exec_module(normalizer)


def make_replay(selector=255):
    data = bytearray((index * 13 + 7) % 256 for index in range(1828 + 123))
    data[24:26] = (123).to_bytes(2, "little")
    data[926] = selector
    return bytes(data)


class SkyboxTests(unittest.TestCase):
    def test_all_selectors_preserve_every_other_byte(self):
        for suffix, offset in ((".RPL", 926), (".TrK", 900)):
            for selector in range(256):
                replay = make_replay(selector)
                original = replay if suffix == ".RPL" else replay[26:1828]
                with self.subTest(suffix=suffix, selector=selector):
                    repaired = normalizer.normalize(original, suffix)
                    self.assertEqual(original[:offset], repaired[:offset])
                    self.assertEqual(original[offset + 1:], repaired[offset + 1:])
                    self.assertEqual(selector if selector < 5 else 0, repaired[offset])
                    self.assertEqual(repaired, normalizer.normalize(repaired, suffix))

    def test_valid_scenery_is_retained_with_each_fallback(self):
        for fallback in range(5):
            self.assertEqual(fallback, normalizer.normalize(make_replay(), ".rpl", fallback)[926])
            for selector in range(5):
                original = make_replay(selector)
                self.assertEqual(original, normalizer.normalize(original, ".rpl", fallback))
        for fallback in (-1, 5, 255):
            with self.assertRaises(ValueError):
                normalizer.normalize(make_replay(), ".rpl", fallback)

    def test_bad_sizes_and_formats_are_rejected(self):
        for data, suffix in ((b"", ".rpl"), (make_replay()[:-1], ".rpl"),
                             (make_replay() + b"x", ".rpl"), (bytes(1801), ".trk"),
                             (bytes(1803), ".trk"), (bytes(1802), ".bin")):
            with self.subTest(size=len(data), suffix=suffix), self.assertRaises(ValueError):
                normalizer.normalize(data, suffix)

    def run_cli(self, *arguments):
        return subprocess.run([sys.executable, str(SCRIPT), *map(str, arguments)],
                              capture_output=True, text=True)

    def test_copy_check_and_in_place_cache_invalidation(self):
        with tempfile.TemporaryDirectory() as temporary:
            root = Path(temporary)
            source = root / "source"
            source.mkdir()
            replay = source / "race.RpL"
            original = make_replay()
            replay.write_bytes(original)
            (source / "valid.rpl").write_bytes(make_replay(4))
            (source / "README.txt").write_text("unrelated")
            self.assertEqual(1, self.run_cli("--check", source).returncode)
            self.assertEqual(original, replay.read_bytes())
            copied = root / "copies"
            result = self.run_cli("--output-directory", copied, source)
            self.assertEqual(0, result.returncode, result.stderr)
            self.assertEqual(original, replay.read_bytes())
            self.assertEqual(normalizer.normalize(original, ".rpl"), (copied / replay.name).read_bytes())
            self.assertEqual(make_replay(4), (copied / "valid.rpl").read_bytes())
            self.assertFalse((copied / "README.txt").exists())
            self.assertEqual(2, self.run_cli("--output-directory", copied, source).returncode)
            self.assertEqual(0, self.run_cli("--check", copied).returncode)

            (source / "race.PDO").write_bytes(b"old cached rendering")
            (source / "RACE.pdo.pending").write_bytes(b"previous interrupted run")
            (source / "race.BIN").write_bytes(b"original physics")
            result = self.run_cli("--in-place", source)
            self.assertEqual(0, result.returncode, result.stderr)
            self.assertEqual(b"", (source / "RACE.pdo.pending").read_bytes())
            self.assertEqual(1, len(list(source.glob("*pending"))))
            self.assertFalse((source / "valid.PDO.pending").exists())
            self.assertEqual(b"original physics", (source / "race.BIN").read_bytes())
            self.assertEqual(normalizer.normalize(original, ".rpl"), replay.read_bytes())

    def test_validate_whole_batch_before_writing(self):
        with tempfile.TemporaryDirectory() as temporary:
            root = Path(temporary)
            replay = root / "first.rpl"
            replay.write_bytes(make_replay())
            (root / "second.trk").write_bytes(b"truncated")
            self.assertEqual(2, self.run_cli("--in-place", root).returncode)
            self.assertEqual(make_replay(), replay.read_bytes())
            self.assertFalse((root / "first.PDO.pending").exists())

    def test_copy_rejects_case_insensitive_collisions(self):
        with tempfile.TemporaryDirectory() as temporary:
            root = Path(temporary)
            source = root / "race.rpl"
            source.write_bytes(make_replay())
            copied = root / "copies"
            copied.mkdir()
            existing = copied / "RACE.RPL"
            existing.write_bytes(b"keep this file")
            self.assertEqual(2, self.run_cli("--output-directory", copied, source).returncode)
            self.assertEqual(b"keep this file", existing.read_bytes())
            self.assertEqual(1, len(list(copied.iterdir())))

    def test_golden_corpus_changes_only_the_unsupported_scenery_bytes(self):
        with zipfile.ZipFile(SCRIPT.parent / "rpls_golden/replays.zip") as archive:
            changed = 0
            for entry in archive.infolist():
                if not entry.filename.lower().endswith(".rpl"):
                    continue
                original = archive.read(entry)
                repaired = normalizer.normalize(original, ".rpl")
                self.assertEqual(original[:926] + original[927:], repaired[:926] + repaired[927:])
                self.assertLess(repaired[926], 5)
                changed += original != repaired
            self.assertGreater(changed, 0)


if __name__ == "__main__":
    unittest.main()
