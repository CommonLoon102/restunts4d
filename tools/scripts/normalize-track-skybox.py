"""Validate or replace unsupported Stunts track/replay skybox selectors.

Only the scenery byte changes. Use copies for renderer comparisons; physics
regressions should continue to consume the original replay corpus.
"""

import argparse
from pathlib import Path


TRACK_SIZE = 1802
TRACK_SKYBOX_OFFSET = 900
REPLAY_HEADER_SIZE = 26
SKYBOX_COUNT = 5


def skybox_offset(data, suffix):
    if suffix.lower() == ".trk":
        expected = TRACK_SIZE
        offset = TRACK_SKYBOX_OFFSET
    elif suffix.lower() == ".rpl":
        if len(data) < REPLAY_HEADER_SIZE:
            raise ValueError("Incomplete 26-byte replay header")
        expected = REPLAY_HEADER_SIZE + TRACK_SIZE + int.from_bytes(data[24:26], "little")
        offset = REPLAY_HEADER_SIZE + TRACK_SKYBOX_OFFSET
    else:
        raise ValueError("Expected a .trk or a 26-byte-header .rpl file")
    if len(data) != expected:
        raise ValueError(f"Unexpected file size: {len(data)} bytes; expected {expected}")
    return offset


def normalize(data, suffix, replacement=0):
    if replacement not in range(SKYBOX_COUNT):
        raise ValueError("Replacement skybox must be between 0 and 4")
    offset = skybox_offset(data, suffix)
    if data[offset] < SKYBOX_COUNT:
        return data
    # Bit 3 bypasses image loading without clearing stale pointers/heights.
    # Other unsupported values can index outside the five resource names.
    return data[:offset] + bytes([replacement]) + data[offset + 1:]


def discover(inputs):
    files = {}
    for source in inputs:
        paths = sorted(source.iterdir()) if source.is_dir() else [source]
        for path in paths:
            if source.is_dir() and (not path.is_file() or path.suffix.lower() not in (".trk", ".rpl")):
                continue
            files[path.resolve()] = path
    if not files:
        raise ValueError("No track or replay files found")
    return list(files.values())


def invalidate_renderer_cache(path):
    name = path.with_suffix(".PDO.pending").name
    matches = [entry for entry in path.parent.iterdir() if entry.name.lower() == name.lower()]
    if len(matches) > 1:
        raise ValueError(f"Duplicate DOS cache marker: {name}")
    marker = matches[0] if matches else path.with_name(name)
    marker.write_bytes(b"")


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("inputs", type=Path, nargs="+", help="Files or directories (not recursive)")
    mode = parser.add_mutually_exclusive_group(required=True)
    mode.add_argument("--check", action="store_true", help="Report invalid selectors without writing")
    mode.add_argument("--in-place", action="store_true", help="Repair prepared copies in place")
    mode.add_argument("--output-directory", type=Path, help="Copy inputs here; refuse to overwrite files")
    parser.add_argument("--skybox", type=int, choices=range(SKYBOX_COUNT), default=0,
                        help="Fallback: 0 desert, 1 tropical, 2 alpine, 3 city, 4 country (default: 0)")
    args = parser.parse_args()
    try:
        prepared = []
        destinations = set()
        if args.output_directory and args.output_directory.is_dir():
            destinations = {path.name.lower() for path in args.output_directory.iterdir()}
        for path in discover(args.inputs):
            data = path.read_bytes()
            try:
                updated = normalize(data, path.suffix, args.skybox)
            except ValueError as error:
                raise ValueError(f"{path}: {error}") from error
            if args.output_directory:
                destination = args.output_directory / path.name
                if destination.exists() or destination.name.lower() in destinations:
                    raise ValueError(f"Output already exists or has a duplicate DOS name: {destination}")
                destinations.add(destination.name.lower())
            prepared.append((path, data, updated))

        changed = 0
        if args.output_directory:
            args.output_directory.mkdir(parents=True, exist_ok=True)
        for path, data, updated in prepared:
            if updated != data:
                offset = skybox_offset(data, path.suffix)
                if args.in_place:
                    if path.suffix.lower() == ".rpl":
                        # The runner must regenerate any PDO cached for the old input.
                        invalidate_renderer_cache(path)
                    with path.open("r+b") as stream:
                        stream.seek(offset)
                        stream.write(updated[offset:offset + 1])
            if args.output_directory:
                with (args.output_directory / path.name).open("xb") as stream:
                    stream.write(updated)
            if updated != data:
                changed += 1
                action = "INVALID" if args.check else "NORMALIZE"
                print(f"{action}|type=skybox|input={path.name}|offset={offset}|"
                      f"from={data[offset]}|to={args.skybox}")
        print(f"Checked {len(prepared)} files; {changed} unsupported skybox selectors.")
        return int(args.check and changed != 0)
    except (OSError, ValueError) as error:
        parser.error(str(error))


if __name__ == "__main__":
    raise SystemExit(main())
