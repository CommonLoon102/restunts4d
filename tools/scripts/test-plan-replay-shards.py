"""Check replay shard planning without running DOSBox or extracting the corpus."""

import importlib.util
import json
from pathlib import Path
import struct
import subprocess
import sys
import tempfile
import unittest
import zipfile


SCRIPT = Path(__file__).with_name("plan-replay-shards.py")
GOLDEN = SCRIPT.parent / "rpls_golden" / "replays.zip"
SPEC = importlib.util.spec_from_file_location("replay_plan", SCRIPT)
planner = importlib.util.module_from_spec(SPEC)
sys.dont_write_bytecode = True
SPEC.loader.exec_module(planner)


class PlannerTests(unittest.TestCase):
    def assert_coverage_and_totals(self, plan, ticks, percentage, opponents=()):
        eligible = sorted(name for name in ticks if plan["target"] == 0 or name in opponents)
        for phase, expected in (
            ("physics", sorted(ticks)),
            ("renderer", planner.sample(eligible, percentage)),
        ):
            actual = sorted(name for shard in plan["shards"] for name in shard[phase])
            self.assertEqual(expected, actual)
            for shard in plan["shards"]:
                self.assertEqual(sum(ticks[name] for name in shard[phase]), shard[phase + "Ticks"])

    def assert_balanced(self, plan):
        for phase in ("physics", "renderer"):
            totals = [shard[phase + "Ticks"] for shard in plan["shards"]]
            average = sum(totals) / len(totals)
            for total in totals:
                self.assertLessEqual(abs(total - average), average * 0.02)

    def test_tick_balance_can_require_different_replay_counts(self):
        ticks = {f"s{index}.rpl": 100 for index in range(10)} | {"long.rpl": 1000}
        plan = planner.create_plan(ticks, 2, 100)
        self.assert_balanced(plan)
        self.assertEqual([1, 10], sorted(len(shard["physics"]) for shard in plan["shards"]))
        self.assert_coverage_and_totals(plan, ticks, 100)
        self.assertEqual(plan, planner.create_plan(dict(reversed(list(ticks.items()))), 2, 100))

    def test_moves_and_swaps_repair_longest_first_assignment(self):
        for weights in ([800, 700, 600, 500, 400], [300, 300, 200, 200, 200]):
            with self.subTest(weights=weights):
                ticks = {f"r{index}.rpl": weight for index, weight in enumerate(weights)}
                plan = planner.create_plan(ticks, 2, 100)
                self.assert_balanced(plan)
                self.assert_coverage_and_totals(plan, ticks, 100)

    def test_indivisible_and_zero_tick_replays_keep_exactly_once_coverage(self):
        for weights in ([], [0] * 5, [1000, 1, 1], [8, 4, 4, 4, 4, 4]):
            for count in (1, 2, 20):
                with self.subTest(weights=weights, count=count):
                    ticks = {f"r{index}.rpl": weight for index, weight in enumerate(weights)}
                    plan = planner.create_plan(ticks, count, 100)
                    self.assertEqual(count, len(plan["shards"]))
                    self.assert_coverage_and_totals(plan, ticks, 100)
                    self.assertEqual(plan, planner.create_plan(ticks, count, 100))
                    if not any(weights):
                        sizes = [len(shard["physics"]) for shard in plan["shards"]]
                        self.assertLessEqual(max(sizes) - min(sizes), 1)
        self.assertIn("Warning: physics exceeds", planner.describe_plan(
            planner.create_plan({"long.rpl": 1000, "short.rpl": 1}, 2, 100)))

    def test_sampling_precedes_sharding_and_does_not_depend_on_shard_count(self):
        ticks = {f"r{index:02}.rpl": index * 100 for index in range(37)}
        expected = ["r00.rpl", "r07.rpl", "r14.rpl", "r22.rpl", "r29.rpl"]
        for count in (1, 3, 20):
            plan = planner.create_plan(ticks, count, 13)
            actual = sorted(name for shard in plan["shards"] for name in shard["renderer"])
            self.assertEqual(expected, actual)
            self.assert_coverage_and_totals(plan, ticks, 13)

    def test_golden_corpus_balances_default_shards_even_with_small_renderer_samples(self):
        ticks, _ = planner.read_replays(GOLDEN)
        for percentage in (1, 2, 3, 5, 100):
            with self.subTest(percentage=percentage):
                plan = planner.create_plan(ticks, 20, percentage)
                self.assert_balanced(plan)
                self.assert_coverage_and_totals(plan, ticks, percentage)

    def test_directory_and_archive_use_headers_not_file_size_or_frequency(self):
        with tempfile.TemporaryDirectory() as temporary:
            directory = Path(temporary)
            expected = {"Alpha.RpL": 0x1234, "maximum.rpl": 65535, "zero.rpl": 0}
            with zipfile.ZipFile(directory / "replays.zip", "w") as archive:
                for name, ticks in expected.items():
                    data = bytearray(100000 if ticks == 0 else 26)
                    struct.pack_into("<HH", data, 22, 10, ticks)
                    data[6] = 0 if name == "zero.rpl" else 6
                    (directory / name).write_bytes(data)
                    archive.writestr(name, data)
                archive.writestr("nested/ignored.rpl", b"")
                archive.writestr("readme.txt", b"")
            for source in (directory, directory / "replays.zip"):
                ticks, opponents = planner.read_replays(source)
                self.assertEqual(expected, ticks)
                self.assertEqual({"Alpha.RpL", "maximum.rpl"}, opponents)

    def test_bad_headers_names_and_collisions_are_rejected(self):
        for entries in (
            [("short.rpl", bytes(25))],
            [("toolong99.rpl", bytes(26))],
            [("CON.rpl", bytes(26))],
            [("race.rpl", bytes(26)), ("RACE.RPL", bytes(26))],
            [("nested/ignored.rpl", bytes(26))],
        ):
            with self.subTest(entries=[name for name, _ in entries]):
                with tempfile.TemporaryDirectory() as temporary:
                    path = Path(temporary) / "replays.zip"
                    with zipfile.ZipFile(path, "w") as archive:
                        for name, data in entries:
                            archive.writestr(name, data)
                    with self.assertRaises(ValueError):
                        planner.read_replays(path)

    def test_invalid_settings_are_rejected(self):
        for count, percentage in ((0, 100), (-1, 100), (2, 0), (2, 101)):
            with self.assertRaises(ValueError):
                planner.create_plan({"race.rpl": 100}, count, percentage)

    def test_opponents_are_filtered_before_sampling_and_balancing(self):
        ticks = {f"r{index:02}.rpl": 100 if index % 2 else 60000 for index in range(10)}
        opponents = {f"r{index:02}.rpl" for index in (1, 3, 5, 7, 9)}
        for count in (1, 3, 20):
            plan = planner.create_plan(ticks, count, 40, target=1, opponents=opponents)
            self.assertEqual(["r01.rpl", "r05.rpl"], sorted(
                name for shard in plan["shards"] for name in shard["renderer"]))
            self.assert_coverage_and_totals(plan, ticks, 40, opponents)
            player_plan = planner.create_plan(ticks, count, 40)
            self.assertEqual([shard["physics"] for shard in player_plan["shards"]],
                             [shard["physics"] for shard in plan["shards"]])

    def test_no_opponents_leaves_empty_renderer_shards_and_full_physics_coverage(self):
        ticks = {"solo.rpl": 100}
        for count in (1, 20):
            plan = planner.create_plan(ticks, count, 100, target=1)
            self.assert_coverage_and_totals(plan, ticks, 100)
            self.assertTrue(all(not shard["renderer"] for shard in plan["shards"]))
            self.assertNotIn("Warning: renderer", planner.describe_plan(plan))

    def test_golden_opponent_selection_is_independent_of_shard_count(self):
        ticks, opponents = planner.read_replays(GOLDEN)
        self.assertTrue(opponents)
        self.assertLess(len(opponents), len(ticks))
        for percentage in (1, 50, 100):
            for count in (1, 20):
                plan = planner.create_plan(ticks, count, percentage, target=1, opponents=opponents)
                self.assert_coverage_and_totals(plan, ticks, percentage, opponents)

    def test_invalid_targets_are_rejected(self):
        for target in (-1, 2):
            with self.assertRaisesRegex(ValueError, "target must be"):
                planner.create_plan({"race.rpl": 100}, 1, 100, target=target)

    def test_command_writes_the_json_contract_and_balance_summary(self):
        with tempfile.TemporaryDirectory() as temporary:
            output = Path(temporary) / "shard-plan.json"
            summary = Path(temporary) / "summary.md"
            result = subprocess.run(
                [sys.executable, str(SCRIPT), "--replays", str(GOLDEN), "--shards", "20",
                 "--renderer-test-percentage", "5", "--output", str(output),
                 "--summary", str(summary)],
                capture_output=True, text=True, check=True, timeout=30,
            )
            plan = json.loads(output.read_text())
            self.assertEqual(1, plan["version"])
            self.assertEqual(5, plan["rendererTestPercentage"])
            self.assertEqual(0, plan["target"])
            self.assertEqual(20, len(plan["shards"]))
            self.assert_balanced(plan)
            self.assertEqual(result.stdout, summary.read_text())
            self.assertIn("Largest deviation", result.stdout)
            subprocess.run(
                [sys.executable, str(SCRIPT), "--replays", str(GOLDEN), "--shards", "20",
                 "--renderer-test-percentage", "100", "--target", "1", "--output", str(output)],
                capture_output=True, text=True, check=True, timeout=30,
            )
            plan = json.loads(output.read_text())
            self.assertEqual(1, plan["target"])
            ticks, opponents = planner.read_replays(GOLDEN)
            self.assert_coverage_and_totals(plan, ticks, 100, opponents)


if __name__ == "__main__":
    unittest.main()
