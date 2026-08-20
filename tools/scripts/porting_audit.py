#!/usr/bin/env python3
"""Report and enforce the Restunts assembly-to-C migration boundary."""

from __future__ import annotations

import argparse
import csv
import html
import io
import json
import re
import sys
from collections import Counter
from pathlib import Path


SCHEMA_VERSION = 1
PORT_STATES = {
    "asm_only",
    "c_translation_inactive",
    "c_active_with_asm",
    "portable_c",
    "not_required",
}


def repository_root() -> Path:
    return Path(__file__).resolve().parents[2]


def repo_path(root: Path, path: Path) -> str:
    return path.resolve().relative_to(root.resolve()).as_posix()


def parse_status_html(path: Path) -> tuple[list[dict[str, object]], int]:
    routines: list[dict[str, object]] = []
    skipped_empty = 0

    document = path.read_text(encoding="utf-8")
    section_re = re.compile(
        r"<thead>.*?<th>((?:seg\d+)|dseg)</th>.*?</thead>\s*<tbody>(.*?)</tbody>",
        re.IGNORECASE | re.DOTALL,
    )
    row_re = re.compile(r"<tr>(.*?)</tr>", re.IGNORECASE | re.DOTALL)
    routine_re = re.compile(r'<a name="([^"]*)"></a>([^<]*)</td>')
    metrics_re = re.compile(
        r"(\d+) callers,\s*(\d+) calls,\s*(\d+) lines of code",
        re.IGNORECASE,
    )
    status_re = re.compile(r'class="status\s+(ported|pending|ignore)"')
    totals_re = re.compile(
        r"Total functions:\s*(\d+)\s*/\s*Ignored:\s*(\d+)\s*/\s*Ported:\s*(\d+)",
        re.IGNORECASE,
    )

    for section_match in section_re.finditer(document):
        segment = section_match.group(1).lower()
        body = section_match.group(2)
        body_start = section_match.start(2)
        for row_match in row_re.finditer(body):
            row = row_match.group(1)
            routine_match = routine_re.search(row)
            status_match = status_re.search(row)
            if not routine_match or not status_match:
                continue

            routine = html.unescape(routine_match.group(2)).strip()
            if not routine:
                skipped_empty += 1
                continue

            metrics_match = metrics_re.search(row)
            callers = calls = asm_lines = 0
            if metrics_match:
                callers, calls, asm_lines = (
                    int(metrics_match.group(1)),
                    int(metrics_match.group(2)),
                    int(metrics_match.group(3)),
                )

            legacy_status = status_match.group(1).lower()
            if legacy_status == "ported":
                port_state = "c_active_with_asm"
            elif legacy_status == "pending":
                port_state = "asm_only"
            else:
                port_state = "not_required"

            absolute_row_start = body_start + row_match.start()
            line_number = document.count("\n", 0, absolute_row_start) + 1
            routines.append(
                {
                    "segment": segment,
                    "routine": routine,
                    "legacy_status": legacy_status,
                    "port_state": port_state,
                    "callers": callers,
                    "calls": calls,
                    "asm_lines": asm_lines,
                    "source_evidence": "",
                    "note": "",
                    "status_line": line_number,
                }
            )

    if not routines:
        raise ValueError(f"no routines found in {path}")
    totals_match = totals_re.search(document)
    if totals_match:
        expected_total = int(totals_match.group(1))
        expected_ignored = int(totals_match.group(2))
        expected_ported = int(totals_match.group(3))
        actual_ignored = sum(
            1 for item in routines if item["legacy_status"] == "ignore"
        ) + skipped_empty
        actual_ported = sum(
            1 for item in routines if item["legacy_status"] == "ported"
        )
        if (
            len(routines) + skipped_empty != expected_total
            or actual_ignored != expected_ignored
            or actual_ported != expected_ported
        ):
            raise ValueError(
                "parsed status totals do not match the status.html footer: "
                f"total={len(routines) + skipped_empty}/{expected_total}, "
                f"ignored={actual_ignored}/{expected_ignored}, "
                f"ported={actual_ported}/{expected_ported}"
            )
    return routines, skipped_empty


def parse_custom_sources(root: Path, path: Path) -> dict[str, str]:
    result: dict[str, str] = {}
    current_source = ""
    source_re = re.compile(r"implementations in ([A-Za-z0-9_.-]+)", re.IGNORECASE)
    external_re = re.compile(r"^\s*extrn\s+([A-Za-z_][A-Za-z0-9_]*):proc", re.IGNORECASE)

    for line in path.read_text(encoding="utf-8").splitlines():
        source_match = source_re.search(line)
        if source_match:
            current_source = f"src/restunts/c/{source_match.group(1)}"
            continue
        external_match = external_re.search(line)
        if external_match and current_source:
            result[external_match.group(1)] = current_source
    return result


def apply_overrides(
    root: Path, routines: list[dict[str, object]], override_path: Path
) -> None:
    by_key = {
        (str(item["segment"]), str(item["routine"])): item for item in routines
    }
    seen: set[tuple[str, str]] = set()

    with override_path.open(encoding="utf-8", newline="") as handle:
        rows = csv.DictReader(
            (line for line in handle if not line.startswith("#")), delimiter="\t"
        )
        required = {"segment", "routine", "port_state", "source_evidence", "note"}
        if rows.fieldnames is None or set(rows.fieldnames) != required:
            raise ValueError(
                f"{override_path} must have exactly these columns: "
                + ", ".join(sorted(required))
            )

        for row in rows:
            key = (row["segment"].strip(), row["routine"].strip())
            if key in seen:
                raise ValueError(f"duplicate override for {key[0]}:{key[1]}")
            seen.add(key)
            if key not in by_key:
                raise ValueError(f"override refers to unknown routine {key[0]}:{key[1]}")
            state = row["port_state"].strip()
            if state not in PORT_STATES:
                raise ValueError(f"invalid port state {state!r} for {key[0]}:{key[1]}")

            source = row["source_evidence"].strip()
            if source:
                source_path = source.split(":", 1)[0]
                if not (root / source_path).is_file():
                    raise ValueError(
                        f"source evidence for {key[0]}:{key[1]} does not exist: "
                        f"{source_path}"
                    )

            by_key[key]["port_state"] = state
            by_key[key]["source_evidence"] = source
            by_key[key]["note"] = row["note"].strip()


def fill_source_evidence(
    routines: list[dict[str, object]],
    definitions: dict[str, list[str]],
    custom_sources: dict[str, str],
) -> None:
    for item in routines:
        if item["source_evidence"]:
            continue
        routine = str(item["routine"])
        if routine in definitions:
            item["source_evidence"] = ",".join(definitions[routine])
        elif routine in custom_sources:
            item["source_evidence"] = custom_sources[routine]


def mask_if_zero_blocks(text: str) -> str:
    output: list[str] = []
    active = True
    stack: list[tuple[bool, bool]] = []

    for line in text.splitlines(keepends=True):
        directive = re.match(r"^\s*#\s*(if|ifdef|ifndef|else|elif|endif)\b(.*)", line)
        if directive:
            kind = directive.group(1)
            expression = directive.group(2).strip()
            if kind in {"if", "ifdef", "ifndef"}:
                is_if_zero = kind == "if" and re.fullmatch(r"0+[uUlL]*", expression) is not None
                stack.append((active, is_if_zero))
                if is_if_zero:
                    active = False
            elif kind in {"else", "elif"} and stack:
                parent_active, is_if_zero = stack[-1]
                if is_if_zero:
                    active = parent_active
                else:
                    # Unknown compile-time conditions are conservatively scanned
                    # in every branch.
                    active = parent_active
            elif kind == "endif" and stack:
                active, _ = stack.pop()

            output.append("\n" if line.endswith("\n") else "")
            continue

        if active:
            output.append(line)
        else:
            output.append("\n" if line.endswith("\n") else "")
    return "".join(output)


def mask_comments_and_literals(text: str) -> str:
    output = list(text)
    index = 0
    state = "code"

    while index < len(text):
        char = text[index]
        next_char = text[index + 1] if index + 1 < len(text) else ""

        if state == "code":
            if char == "/" and next_char == "*":
                output[index] = output[index + 1] = " "
                state = "block_comment"
                index += 2
                continue
            if char == "/" and next_char == "/":
                output[index] = output[index + 1] = " "
                state = "line_comment"
                index += 2
                continue
            if char == '"':
                output[index] = " "
                state = "string"
            elif char == "'":
                output[index] = " "
                state = "character"
        elif state == "block_comment":
            if char == "*" and next_char == "/":
                output[index] = output[index + 1] = " "
                state = "code"
                index += 2
                continue
            if char != "\n":
                output[index] = " "
        elif state == "line_comment":
            if char == "\n":
                state = "code"
            else:
                output[index] = " "
        elif state in {"string", "character"}:
            if char == "\\" and next_char:
                if char != "\n":
                    output[index] = " "
                if next_char != "\n":
                    output[index + 1] = " "
                index += 2
                continue
            delimiter = '"' if state == "string" else "'"
            if char == delimiter:
                output[index] = " "
                state = "code"
            elif char != "\n":
                output[index] = " "
        index += 1

    return "".join(output)


def active_c_text(path: Path) -> str:
    raw = path.read_text(encoding="utf-8", errors="replace")
    return mask_comments_and_literals(mask_if_zero_blocks(raw))


def find_c_definitions(root: Path) -> dict[str, list[str]]:
    definitions: dict[str, list[str]] = {}
    candidate_re = re.compile(r"\b([A-Za-z_][A-Za-z0-9_]*)\s*\(")
    control_words = {"if", "for", "while", "switch", "sizeof"}

    for path in sorted((root / "src/restunts/c").glob("*.c")):
        text = active_c_text(path)
        for match in candidate_re.finditer(text):
            name = match.group(1)
            if name in control_words:
                continue

            open_paren = text.find("(", match.start(1) + len(name))
            depth = 0
            index = open_paren
            while index < len(text):
                if text[index] == "(":
                    depth += 1
                elif text[index] == ")":
                    depth -= 1
                    if depth == 0:
                        break
                index += 1
            if depth != 0:
                continue

            after = index + 1
            while after < len(text) and text[after].isspace():
                after += 1
            if after >= len(text) or text[after] != "{":
                continue

            line = text.count("\n", 0, match.start(1)) + 1
            evidence = f"{repo_path(root, path)}:{line}"
            definitions.setdefault(name, []).append(evidence)
    return definitions


def token_locations(text: str, pattern: re.Pattern[str]) -> list[tuple[int, str]]:
    result = []
    for match in pattern.finditer(text):
        line = text.count("\n", 0, match.start()) + 1
        result.append((line, match.group(0)))
    return result


def scan_blockers(
    root: Path, link_inputs: list[Path]
) -> dict[str, list[dict[str, object]]]:
    blockers: dict[str, list[dict[str, object]]] = {
        "inline_asm": [],
        "preserved_asm_symbols": [],
        "asm_link_inputs": [],
    }
    inline_re = re.compile(r"(?<![A-Za-z0-9_])(?:__asm|_asm|asm)(?![A-Za-z0-9_])")
    preserved_re = re.compile(r"\bported_[A-Za-z_][A-Za-z0-9_]*")

    c_root = root / "src/restunts/c"
    for path in sorted(c_root.rglob("*")):
        if path.suffix.lower() not in {".c", ".h"} or not path.is_file():
            continue
        text = active_c_text(path)
        for line, token in token_locations(text, inline_re):
            blockers["inline_asm"].append(
                {"path": repo_path(root, path), "line": line, "token": token}
            )
        for line, token in token_locations(text, preserved_re):
            blockers["preserved_asm_symbols"].append(
                {"path": repo_path(root, path), "line": line, "token": token}
            )

    asm_object_re = re.compile(
        r"\b(?:seg\d{3}|segments|dseg)\.(?:obj|asm)\b", re.IGNORECASE
    )
    seen_link_tokens: set[tuple[str, str]] = set()
    for path in link_inputs:
        relative = repo_path(root, path)
        for line_number, line in enumerate(
            path.read_text(encoding="utf-8").splitlines(), start=1
        ):
            for match in asm_object_re.finditer(line):
                key = (relative, match.group(0).lower())
                if key in seen_link_tokens:
                    continue
                seen_link_tokens.add(key)
                blockers["asm_link_inputs"].append(
                    {
                        "path": relative,
                        "line": line_number,
                        "token": match.group(0),
                    }
                )
    return blockers


def sanitize_tsv(value: object) -> str:
    sanitized = str(value).replace("\t", " ").replace("\r", " ").replace("\n", " ")
    return sanitized if sanitized else "-"


def inventory_tsv(routines: list[dict[str, object]]) -> str:
    columns = [
        "segment",
        "routine",
        "legacy_status",
        "port_state",
        "callers",
        "calls",
        "asm_lines",
        "source_evidence",
        "note",
    ]
    output = io.StringIO()
    writer = csv.writer(output, delimiter="\t", lineterminator="\n")
    writer.writerow(columns)
    for item in routines:
        writer.writerow([sanitize_tsv(item[column]) for column in columns])
    return output.getvalue()


def build_report(
    routines: list[dict[str, object]],
    blockers: dict[str, list[dict[str, object]]],
    skipped_empty: int,
) -> dict[str, object]:
    states = Counter(str(item["port_state"]) for item in routines)
    legacy = Counter(str(item["legacy_status"]) for item in routines)
    asm_lines = Counter()
    for item in routines:
        asm_lines[str(item["port_state"])] += int(item["asm_lines"])

    remaining = sum(
        count
        for state, count in states.items()
        if state not in {"portable_c", "not_required"}
    )
    blocker_count = sum(len(items) for items in blockers.values())
    return {
        "schema_version": SCHEMA_VERSION,
        "summary": {
            "tracked_routines": len(routines),
            "skipped_empty_status_rows": skipped_empty,
            "remaining_required_routines": remaining,
            "port_states": dict(sorted(states.items())),
            "legacy_statuses": dict(sorted(legacy.items())),
            "asm_lines_by_port_state": dict(sorted(asm_lines.items())),
            "source_blockers": blocker_count,
            "c_only_ready": remaining == 0 and blocker_count == 0,
        },
        "blockers": blockers,
        "routines": routines,
    }


def print_summary(report: dict[str, object], root: Path) -> None:
    summary = report["summary"]
    assert isinstance(summary, dict)
    print("Restunts ASM-to-C porting inventory")
    print(f"  tracked routines: {summary['tracked_routines']}")
    print(f"  ignored empty status rows: {summary['skipped_empty_status_rows']}")
    print(f"  required routines remaining: {summary['remaining_required_routines']}")
    print("  states:")
    states = summary["port_states"]
    line_counts = summary["asm_lines_by_port_state"]
    assert isinstance(states, dict)
    assert isinstance(line_counts, dict)
    for state in sorted(PORT_STATES):
        print(
            f"    {state}: {states.get(state, 0)} routines, "
            f"{line_counts.get(state, 0)} ASM listing lines"
        )

    print("  current C-only blockers:")
    blockers = report["blockers"]
    assert isinstance(blockers, dict)
    for kind, items in blockers.items():
        print(f"    {kind}: {len(items)}")
        for item in items[:3]:
            print(f"      {item['path']}:{item['line']} {item['token']}")
        if len(items) > 3:
            print(f"      ... {len(items) - 3} more")
    print(f"  C-only ready: {'yes' if summary['c_only_ready'] else 'no'}")
    print(
        "  inventory: "
        + repo_path(root, root / "src/restunts/porting/inventory.tsv")
    )


def main() -> int:
    root = repository_root()
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument(
        "--format", choices=("summary", "tsv", "json"), default="summary"
    )
    parser.add_argument(
        "--write-inventory",
        action="store_true",
        help="regenerate the checked-in TSV inventory",
    )
    parser.add_argument(
        "--check-inventory",
        action="store_true",
        help="fail when the checked-in TSV does not match its inputs",
    )
    parser.add_argument(
        "--require-c-only",
        action="store_true",
        help="fail unless all required routines and source/link audits are C-only",
    )
    parser.add_argument(
        "--link-input",
        action="append",
        default=[],
        metavar="PATH",
        help=(
            "makefile, linker script, or linker map to scan for Restunts ASM "
            "objects; repeat to scan more than one"
        ),
    )
    args = parser.parse_args()

    status_path = root / "src/restunts/status.html"
    override_path = root / "src/restunts/porting/overrides.tsv"
    inventory_path = root / "src/restunts/porting/inventory.tsv"
    custom_path = root / "src/restunts/asm/custom.inc"

    try:
        routines, skipped_empty = parse_status_html(status_path)
        custom_sources = parse_custom_sources(root, custom_path)
        apply_overrides(root, routines, override_path)
        definitions = find_c_definitions(root)
        fill_source_evidence(routines, definitions, custom_sources)
    except (OSError, ValueError) as error:
        print(f"porting audit error: {error}", file=sys.stderr)
        return 2

    generated_inventory = inventory_tsv(routines)
    if args.write_inventory:
        inventory_path.write_text(generated_inventory, encoding="utf-8", newline="")

    failed = False
    if args.check_inventory:
        try:
            current_inventory = inventory_path.read_text(encoding="utf-8")
        except OSError as error:
            print(f"porting audit error: {error}", file=sys.stderr)
            return 2
        if current_inventory != generated_inventory:
            print(
                "porting inventory is stale; run "
                "tools/scripts/porting_audit.py --write-inventory",
                file=sys.stderr,
            )
            failed = True

    if args.link_input:
        link_inputs = [
            (root / path).resolve() if not Path(path).is_absolute() else Path(path)
            for path in args.link_input
        ]
    else:
        link_inputs = [
            root / "src/restunts/dos/makefile",
            root / "src/restunts/repldump/makefile",
        ]
    missing_link_inputs = [path for path in link_inputs if not path.is_file()]
    if missing_link_inputs:
        for path in missing_link_inputs:
            print(f"porting audit error: link input does not exist: {path}", file=sys.stderr)
        return 2

    blockers = scan_blockers(root, link_inputs)
    report = build_report(routines, blockers, skipped_empty)

    if args.format == "tsv":
        sys.stdout.write(generated_inventory)
    elif args.format == "json":
        json.dump(report, sys.stdout, indent=2, sort_keys=True)
        sys.stdout.write("\n")
    else:
        print_summary(report, root)

    if args.require_c_only:
        summary = report["summary"]
        assert isinstance(summary, dict)
        if not summary["c_only_ready"]:
            print("C-only audit failed: migration is not complete.", file=sys.stderr)
            failed = True
    return 1 if failed else 0


if __name__ == "__main__":
    raise SystemExit(main())
