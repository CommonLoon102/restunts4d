"""Plan deterministic replay shards with similar totals of recorded ticks."""

import argparse
import heapq
import json
from pathlib import Path
import re
import struct
import zipfile


TOLERANCE = 0.02
RESERVED_NAMES = {"CON", "PRN", "AUX", "NUL", "CLOCK$", "CONIN$", "CONOUT$"} | {
    f"{prefix}{number}" for prefix in ("COM", "LPT") for number in range(1, 10)
}


def read_replays(source):
    """Read only the 26-byte headers, from a directory or the golden ZIP."""
    ticks = {}
    identities = set()

    def add(name, header):
        basename = name[:-4]
        if not re.fullmatch(r"[A-Za-z0-9!#$'()@_`{}~-]{1,8}", basename):
            raise ValueError(f"Unsupported DOS 8.3 replay basename: {basename}")
        if basename.upper() in RESERVED_NAMES:
            raise ValueError(f"Reserved DOS device replay basename: {basename}")
        if name.lower() in identities:
            raise ValueError(f"Case-insensitive replay filename collision: {name}")
        if len(header) != 26:
            raise ValueError(f"Incomplete replay header: {name}")
        identities.add(name.lower())
        ticks[name] = struct.unpack_from("<H", header, 24)[0]

    if source.is_dir():
        for path in source.iterdir():
            if path.is_file() and path.suffix.lower() == ".rpl":
                with path.open("rb") as stream:
                    add(path.name, stream.read(26))
    else:
        with zipfile.ZipFile(source) as archive:
            for entry in archive.infolist():
                if "/" not in entry.filename and entry.filename.lower().endswith(".rpl"):
                    with archive.open(entry) as stream:
                        add(entry.filename, stream.read(26))
    if not ticks:
        raise ValueError(f"No replay files found in {source}.")
    return dict(sorted(ticks.items()))


def sample(replays, percentage):
    count = (len(replays) * percentage + 99) // 100
    return [replays[index * len(replays) // count] for index in range(count)]


def improve_balance(shards, totals, ticks, minimum, maximum):
    best = None
    best_gain = 0
    for source, source_replays in enumerate(shards):
        for destination, destination_replays in enumerate(shards):
            gap = totals[source] - totals[destination]
            if gap <= 0 or (totals[source] <= maximum and totals[destination] >= minimum):
                continue
            for index, replay in enumerate(source_replays):
                # Index -1 moves a replay; the other indices exchange two replays.
                for other in range(-1, len(destination_replays)):
                    delta = ticks[replay] - (0 if other < 0 else ticks[destination_replays[other]])
                    if not 0 < delta < gap:
                        continue
                    # Half the reduction in squared tick imbalance. A positive
                    # gain prevents cycles when indivisible replays cannot fit.
                    gain = delta * (gap - delta)
                    if gain > best_gain:
                        best_gain = gain
                        best = source, destination, index, other, delta
    if best is None:
        return False
    source, destination, index, other, delta = best
    if other < 0:
        shards[destination].append(shards[source].pop(index))
    else:
        shards[source][index], shards[destination][other] = (
            shards[destination][other], shards[source][index]
        )
    totals[source] -= delta
    totals[destination] += delta
    return True


def balance(replays, ticks, count):
    shards = [[] for _ in range(count)]
    totals = [0] * count
    queue = [(0, 0, index) for index in range(count)]
    for replay in sorted(replays, key=lambda name: (-ticks[name], name)):
        total, size, index = heapq.heappop(queue)
        shards[index].append(replay)
        totals[index] = total + ticks[replay]
        heapq.heappush(queue, (totals[index], size + 1, index))
    average = sum(totals) / count
    minimum, maximum = average * (1 - TOLERANCE), average * (1 + TOLERANCE)
    while any(total < minimum or total > maximum for total in totals):
        if not improve_balance(shards, totals, ticks, minimum, maximum):
            break
    return [sorted(shard) for shard in shards], totals


def create_plan(ticks, shard_count, percentage):
    if not 1 <= shard_count <= 2147483647:
        raise ValueError("shards must be an integer from 1 to 2147483647")
    if not 1 <= percentage <= 100:
        raise ValueError("renderer-test-percentage must be an integer from 1 to 100")
    replays = sorted(ticks)
    physics, physics_ticks = balance(replays, ticks, shard_count)
    renderer, renderer_ticks = balance(sample(replays, percentage), ticks, shard_count)
    return {
        "version": 1,
        "rendererTestPercentage": percentage,
        "shards": [
            {
                "physics": physics[index],
                "renderer": renderer[index],
                "physicsTicks": physics_ticks[index],
                "rendererTicks": renderer_ticks[index],
            }
            for index in range(shard_count)
        ],
    }


def describe_plan(plan):
    lines = ["| Phase | Minimum ticks | Maximum ticks | Mean ticks | Largest deviation |",
             "| --- | ---: | ---: | ---: | ---: |"]
    warnings = []
    for phase in ("physics", "renderer"):
        totals = [shard[phase + "Ticks"] for shard in plan["shards"]]
        average = sum(totals) / len(totals)
        deviation = max(abs(total - average) / average for total in totals) if average else 0
        lines.append(
            f"| {phase} | {min(totals)} | {max(totals)} | {average:.2f} | {deviation:.2%} |"
        )
        if deviation > TOLERANCE:
            warnings.append(
                f"Warning: {phase} exceeds the ±2% target; no improving replay move or swap "
                "remains. Fewer shards or more replays may be needed."
            )
    return "\n".join(lines) + "\n" + "".join(f"\n{warning}\n" for warning in warnings)


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--replays", type=Path, required=True, help="Replay directory or ZIP")
    parser.add_argument("--shards", type=int, required=True)
    parser.add_argument("--renderer-test-percentage", type=int, required=True)
    parser.add_argument("--output", type=Path, required=True)
    parser.add_argument("--summary", type=Path, help="Append a Markdown balance summary")
    args = parser.parse_args()
    try:
        plan = create_plan(read_replays(args.replays), args.shards, args.renderer_test_percentage)
    except (OSError, ValueError, zipfile.BadZipFile) as exception:
        parser.error(str(exception))
    args.output.write_text(json.dumps(plan, indent=2) + "\n", encoding="utf-8")
    summary = describe_plan(plan)
    print(summary, end="")
    if args.summary:
        with args.summary.open("a", encoding="utf-8") as stream:
            stream.write(summary)


if __name__ == "__main__":
    main()
