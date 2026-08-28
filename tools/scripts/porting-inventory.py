#!/usr/bin/env python3
"""Generate the machine-readable Restunts assembly-port inventory."""

from __future__ import annotations

import argparse
import json
import re
import sys
from collections import Counter
from pathlib import Path


ASM_ONLY = "asm_only"
C_INACTIVE = "c_translation_inactive"
C_ASM_DEPENDENT = "c_active_asm_dependent"
PORTABLE_C = "portable_c"

STATE_DESCRIPTIONS = {
    ASM_ONLY: "No C implementation is present.",
    C_INACTIVE: "A C implementation exists, but the ported target still enters the assembly implementation.",
    C_ASM_DEPENDENT: "The active C implementation still reaches project assembly, contains inline assembly, or reads data supplied by dseg.asm.",
    PORTABLE_C: "The active implementation and its known C dependency closure use portable C storage and operations.",
}

IDENTIFIER = re.compile(r"[A-Za-z_$?@][A-Za-z0-9_$?@]*")
PROC_START = re.compile(
    r"^\s*([A-Za-z_$?@][A-Za-z0-9_$?@]*)\s+proc(?:\s+(near|far))?\b",
    re.IGNORECASE | re.MULTILINE,
)
PROC_END_TEMPLATE = r"^\s*{name}\s+endp\b"
EXTRN_PROC = re.compile(
    r"^\s*extrn\s+([A-Za-z_$?@][A-Za-z0-9_$?@]*):proc\b",
    re.IGNORECASE | re.MULTILINE,
)
DSEG_EXTRN = re.compile(
    r"^\s*extrn\s+([A-Za-z_$?@][A-Za-z0-9_$?@]*):",
    re.IGNORECASE | re.MULTILINE,
)
ASM_TRANSFER = re.compile(
    r"^\s*(?:jmp|call)\s+(?:(?:near|far)\s+ptr\s+)?"
    r"([A-Za-z_$?@][A-Za-z0-9_$?@]*)\b",
    re.IGNORECASE,
)
CALL = re.compile(r"\b([A-Za-z_][A-Za-z0-9_]*)\s*\(")
PORTED_REF = re.compile(r"\bported_[A-Za-z0-9_]+\b")
INLINE_ASM = re.compile(r"\b(?:__asm|asm)\s*\{")
TARGET_DEFINES = {"RESTUNTS_DOS", "__STDIO_H"}


def select_target_preprocessor(source: str) -> str:
    """Select the non-original DOS branches while preserving line numbers."""
    output: list[str] = []
    stack: list[dict[str, bool]] = []
    active = True

    def condition(text: str) -> bool:
        expression = text.strip()
        defined = re.fullmatch(r"defined\s*\(\s*(\w+)\s*\)", expression)
        not_defined = re.fullmatch(r"!\s*defined\s*\(\s*(\w+)\s*\)", expression)
        if defined:
            return defined.group(1) in TARGET_DEFINES
        if not_defined:
            return not_defined.group(1) not in TARGET_DEFINES
        return expression in TARGET_DEFINES

    for line in source.splitlines(keepends=True):
        directive = re.match(
            r"^\s*#\s*(ifdef|ifndef|if|elif|else|endif)\b(.*)$", line
        )
        if directive:
            operation = directive.group(1)
            argument = directive.group(2)
            if operation in ("ifdef", "ifndef", "if"):
                selected = condition(argument)
                if operation == "ifndef":
                    selected = not selected
                stack.append(
                    {
                        "parent_active": active,
                        "branch_taken": selected,
                        "active": active and selected,
                    }
                )
                active = stack[-1]["active"]
            elif operation == "elif":
                frame = stack[-1]
                selected = not frame["branch_taken"] and condition(argument)
                frame["branch_taken"] = frame["branch_taken"] or selected
                frame["active"] = frame["parent_active"] and selected
                active = frame["active"]
            elif operation == "else":
                frame = stack[-1]
                selected = not frame["branch_taken"]
                frame["branch_taken"] = True
                frame["active"] = frame["parent_active"] and selected
                active = frame["active"]
            else:
                stack.pop()
                active = stack[-1]["active"] if stack else True
            output.append("\n" if line.endswith("\n") else "")
        elif active:
            output.append(line)
        else:
            output.append("\n" if line.endswith("\n") else "")
    return "".join(output)


def strip_comments_and_literals(source: str) -> str:
    """Blank comments and literals while preserving offsets and newlines."""
    result = list(source)
    i = 0
    state = "code"
    quote = ""
    while i < len(source):
        char = source[i]
        nxt = source[i + 1] if i + 1 < len(source) else ""
        if state == "code":
            if char == "/" and nxt == "/":
                result[i] = result[i + 1] = " "
                state = "line_comment"
                i += 2
                continue
            if char == "/" and nxt == "*":
                result[i] = result[i + 1] = " "
                state = "block_comment"
                i += 2
                continue
            if char in ('"', "'"):
                result[i] = " "
                quote = char
                state = "literal"
        elif state == "line_comment":
            if char == "\n":
                state = "code"
            else:
                result[i] = " "
        elif state == "block_comment":
            if char == "*" and nxt == "/":
                result[i] = result[i + 1] = " "
                state = "code"
                i += 2
                continue
            if char != "\n":
                result[i] = " "
        else:
            if char == "\\":
                result[i] = " "
                if i + 1 < len(source):
                    if source[i + 1] != "\n":
                        result[i + 1] = " "
                    i += 2
                    continue
            elif char == quote:
                result[i] = " "
                state = "code"
            elif char != "\n":
                result[i] = " "
        i += 1
    return "".join(result)


def matching_delimiter(source: str, start: int, opening: str, closing: str) -> int | None:
    depth = 0
    for index in range(start, len(source)):
        if source[index] == opening:
            depth += 1
        elif source[index] == closing:
            depth -= 1
            if depth == 0:
                return index
    return None


def find_function(source: str, name: str) -> tuple[int, int, int] | None:
    for match in re.finditer(r"\b" + re.escape(name) + r"\s*\(", source):
        open_paren = source.find("(", match.start())
        close_paren = matching_delimiter(source, open_paren, "(", ")")
        if close_paren is None:
            continue
        brace = close_paren + 1
        while brace < len(source) and source[brace].isspace():
            brace += 1
        if brace >= len(source) or source[brace] != "{":
            continue
        close_brace = matching_delimiter(source, brace, "{", "}")
        if close_brace is not None:
            return match.start(), brace + 1, close_brace
    return None


def discover_function_names(source: str) -> set[str]:
    names: set[str] = set()
    pattern = re.compile(
        r"(?m)^\s*(?:(?:static|extern|const|volatile|interrupt|far|near)\s+)*"
        r"(?:struct\s+\w+\s+|[A-Za-z_]\w*\s+|\*\s*)+"
        r"([A-Za-z_]\w*)\s*\("
    )
    for match in pattern.finditer(source):
        if find_function(source, match.group(1)) is not None:
            names.add(match.group(1))
    return names


def logical_name(asm_symbol: str, c_names: set[str]) -> str:
    if asm_symbol.startswith("ported_") and asm_symbol.endswith("_"):
        middle = asm_symbol[len("ported_") : -1]
        if middle in c_names:
            return middle
        if "_" + middle in c_names:
            return "_" + middle
        return middle
    return asm_symbol


def first_instructions(body: str, limit: int = 12) -> list[str]:
    instructions: list[str] = []
    for raw_line in body.splitlines():
        line = raw_line.split(";", 1)[0].strip()
        if not line or line.endswith(":") or re.match(r"^\w+\s*=", line):
            continue
        if re.match(r"^(?:db|dw|dd)\b", line, re.IGNORECASE):
            continue
        instructions.append(line)
        if len(instructions) == limit:
            break
    return instructions


def collect_asm_procedures(root: Path) -> list[dict[str, object]]:
    procedures: list[dict[str, object]] = []
    for path in sorted((root / "src/restunts/asm").glob("seg*.asm")):
        source = path.read_text(encoding="latin-1")
        for match in PROC_START.finditer(source):
            symbol = match.group(1)
            end_match = re.compile(
                PROC_END_TEMPLATE.format(name=re.escape(symbol)),
                re.IGNORECASE | re.MULTILINE,
            ).search(source, match.end())
            end = end_match.start() if end_match else len(source)
            body = source[match.end() : end]
            procedures.append(
                {
                    "asm_symbol": symbol,
                    "segment": path.stem,
                    "asm_file": path.relative_to(root).as_posix(),
                    "asm_line": source.count("\n", 0, match.start()) + 1,
                    "distance": (match.group(2) or "unspecified").lower(),
                    "body": body,
                }
            )
    return procedures


def collect_c_functions(root: Path) -> dict[str, list[dict[str, object]]]:
    functions: dict[str, list[dict[str, object]]] = {}
    paths = sorted((root / "src/restunts/c").glob("*.c"))
    paths += sorted((root / "src/restunts/dos").glob("*.c"))
    paths += sorted((root / "src/restunts/repldump").glob("*.c"))
    for path in paths:
        raw = select_target_preprocessor(path.read_text(encoding="latin-1"))
        clean = strip_comments_and_literals(raw)
        for name in sorted(discover_function_names(clean)):
            location = find_function(clean, name)
            if location is None:
                continue
            start, body_start, body_end = location
            functions.setdefault(name, []).append(
                {
                    "file": path.relative_to(root).as_posix(),
                    "line": clean.count("\n", 0, start) + 1,
                    "body_line": clean.count("\n", 0, body_start) + 1,
                    "body": clean[body_start:body_end],
                }
            )
    return functions


def collect_custom_c_names(root: Path) -> set[str]:
    source = (root / "src/restunts/asm/custom.inc").read_text(encoding="latin-1")
    return set(EXTRN_PROC.findall(source))


def collect_dseg_names(root: Path) -> set[str]:
    source = (root / "src/restunts/asm/dseg.inc").read_text(encoding="latin-1")
    return set(DSEG_EXTRN.findall(source))


def redirect_target(body: str, c_names: set[str]) -> str | None:
    for instruction in first_instructions(body):
        match = ASM_TRANSFER.match(instruction)
        if match and instruction.lower().startswith("jmp") and match.group(1) in c_names:
            return match.group(1)
    return None


def build_inventory(root: Path) -> dict[str, object]:
    asm_procedures = collect_asm_procedures(root)
    c_functions = collect_c_functions(root)
    c_names = set(c_functions)
    custom_c_names = collect_custom_c_names(root)
    dseg_names = collect_dseg_names(root)

    for procedure in asm_procedures:
        procedure["name"] = logical_name(str(procedure["asm_symbol"]), c_names)
        procedure["redirect"] = redirect_target(str(procedure["body"]), c_names)
        name = str(procedure["name"])
        candidate = str(procedure["redirect"] or name)
        if candidate not in c_functions and "_" + name in c_functions:
            candidate = "_" + name
        procedure["c_name"] = candidate if candidate in c_functions else None
    c_blockers: dict[str, set[str]] = {}
    c_calls: dict[str, set[str]] = {}

    for name, definitions in c_functions.items():
        blockers: set[str] = set()
        calls: set[str] = set()
        for definition in definitions:
            body = str(definition["body"])
            tokens = set(IDENTIFIER.findall(body))
            calls.update(CALL.findall(body))
            if INLINE_ASM.search(body):
                blockers.add("inline_asm")
            if PORTED_REF.search(body):
                blockers.add("ported_asm_symbol")
            if tokens & dseg_names:
                blockers.add("dseg_data")
        c_blockers[name] = blockers
        c_calls[name] = calls & c_names

    active_c: set[str] = {"stuntsmain"} & c_names
    for procedure in asm_procedures:
        redirect = procedure["redirect"]
        if redirect is not None:
            active_c.add(str(redirect))
            continue
        if procedure["name"] == "stuntsmain":
            continue
        for line in str(procedure["body"]).splitlines():
            transfer = ASM_TRANSFER.match(line.split(";", 1)[0])
            if transfer and transfer.group(1) in c_names:
                active_c.add(transfer.group(1))

    changed = True
    while changed:
        expanded = active_c | {
            callee for caller in active_c for callee in c_calls[caller]
        }
        changed = expanded != active_c
        active_c = expanded

    asm_only_names = {
        str(item["name"])
        for item in asm_procedures
        if item["c_name"] is None or str(item["c_name"]) not in active_c
    }
    for name in c_names:
        definitions = c_functions[name]
        calls = set().union(*(set(CALL.findall(str(item["body"]))) for item in definitions))
        if calls & asm_only_names:
            c_blockers[name].add("asm_call")

    changed = True
    while changed:
        changed = False
        for name in c_names:
            if c_blockers[name]:
                continue
            if any(c_blockers[callee] for callee in c_calls[name]):
                c_blockers[name].add("transitive_asm_dependency")
                changed = True

    output_procedures: list[dict[str, object]] = []
    for procedure in asm_procedures:
        c_name = procedure["c_name"]
        active = c_name is not None and str(c_name) in active_c
        if c_name is None:
            state = ASM_ONLY
            blockers = ["missing_c_translation"]
        elif not active:
            state = C_INACTIVE
            blockers = ["assembly_entry_active"]
        else:
            blockers = sorted(c_blockers[str(c_name)])
            state = C_ASM_DEPENDENT if blockers else PORTABLE_C

        c_locations = []
        if c_name is not None:
            c_locations = [
                {"file": item["file"], "line": item["line"]}
                for item in c_functions[str(c_name)]
            ]
        output_procedures.append(
            {
                "name": procedure["name"],
                "asm_symbol": procedure["asm_symbol"],
                "segment": procedure["segment"],
                "asm_file": procedure["asm_file"],
                "asm_line": procedure["asm_line"],
                "distance": procedure["distance"],
                "state": state,
                "c_symbol": c_name,
                "c_locations": c_locations,
                "activation": (
                    "assembly_trampoline"
                    if procedure["redirect"] is not None
                    else "ported_entry_point"
                    if active
                    else "assembly"
                ),
                "blockers": blockers,
            }
        )

    output_procedures.sort(key=lambda item: (str(item["segment"]), int(item["asm_line"])))
    state_counts = Counter(str(item["state"]) for item in output_procedures)

    inline_asm_locations: list[dict[str, object]] = []
    ported_refs: list[dict[str, object]] = []
    dseg_references: dict[str, list[dict[str, object]]] = {}
    for name in sorted(active_c):
        for definition in c_functions[name]:
            body = str(definition["body"])
            file = str(definition["file"])
            body_line = int(definition["body_line"])
            for match in INLINE_ASM.finditer(body):
                inline_asm_locations.append(
                    {
                        "file": file,
                        "line": body_line + body.count("\n", 0, match.start()),
                    }
                )
            for match in PORTED_REF.finditer(body):
                ported_refs.append(
                    {
                        "symbol": match.group(0),
                        "file": file,
                        "line": body_line + body.count("\n", 0, match.start()),
                    }
                )
            for match in IDENTIFIER.finditer(body):
                symbol = match.group(0)
                if symbol not in dseg_names:
                    continue
                locations = dseg_references.setdefault(symbol, [])
                line = body_line + body.count("\n", 0, match.start())
                if (
                    not locations
                    or locations[-1]["file"] != file
                    or locations[-1]["line"] != line
                ):
                    locations.append({"file": file, "line": line})

    inline_asm_locations = list(
        {
            (str(item["file"]), int(item["line"])): item
            for item in inline_asm_locations
        }.values()
    )
    inline_asm_locations.sort(
        key=lambda item: (str(item["file"]), int(item["line"]))
    )
    ported_refs = list(
        {
            (str(item["symbol"]), str(item["file"]), int(item["line"])): item
            for item in ported_refs
        }.values()
    )
    ported_refs.sort(
        key=lambda item: (str(item["symbol"]), str(item["file"]), int(item["line"]))
    )

    makefiles = [
        root / "src/restunts/dos/makefile",
        root / "src/restunts/repldump/makefile",
    ]
    linked_asm_objects: set[str] = set()
    for path in makefiles:
        source = path.read_text(encoding="latin-1")
        linked_asm_objects.update(
            re.findall(r"\b(?:seg\d+|segments|dseg)\.obj\b", source, re.IGNORECASE)
        )

    return {
        "schema_version": 1,
        "state_definitions": STATE_DESCRIPTIONS,
        "summary": {
            "procedure_count": len(output_procedures),
            "states": {state: state_counts.get(state, 0) for state in STATE_DESCRIPTIONS},
            "dseg_symbol_count": len(dseg_names),
            "referenced_dseg_symbol_count": len(dseg_references),
            "inline_asm_block_count": len(inline_asm_locations),
            "ported_asm_reference_count": len(ported_refs),
            "linked_project_asm_object_count": len(linked_asm_objects),
        },
        "c_only_gate": {
            "passes": not (
                linked_asm_objects
                or inline_asm_locations
                or ported_refs
                or dseg_references
            ),
            "linked_project_asm_objects": sorted(linked_asm_objects),
            "inline_asm": inline_asm_locations,
            "ported_asm_references": ported_refs,
            "dseg_source": "src/restunts/asm/dseg.asm",
            "dseg_references": [
                {"symbol": symbol, "locations": dseg_references[symbol]}
                for symbol in sorted(dseg_references)
            ],
        },
        "procedures": output_procedures,
        "c_translation_exports": sorted(custom_c_names),
    }


def parse_arguments() -> argparse.Namespace:
    parser = argparse.ArgumentParser()
    parser.add_argument(
        "--root",
        type=Path,
        default=Path(__file__).resolve().parents[2],
        help="repository root",
    )
    parser.add_argument("--write", type=Path, help="write the JSON inventory")
    parser.add_argument(
        "--check-generated",
        type=Path,
        help="fail if this generated inventory is stale",
    )
    parser.add_argument(
        "--check-c-only",
        action="store_true",
        help="fail while any project assembly dependency remains",
    )
    return parser.parse_args()


def main() -> int:
    arguments = parse_arguments()
    root = arguments.root.resolve()
    inventory = build_inventory(root)
    serialized = json.dumps(inventory, indent=2, sort_keys=True) + "\n"

    if arguments.write:
        destination = arguments.write
        if not destination.is_absolute():
            destination = root / destination
        destination.write_text(serialized, encoding="utf-8")
    elif not arguments.check_generated and not arguments.check_c_only:
        sys.stdout.write(serialized)

    if arguments.check_generated:
        expected = arguments.check_generated
        if not expected.is_absolute():
            expected = root / expected
        if not expected.exists() or expected.read_text(encoding="utf-8") != serialized:
            print(f"stale porting inventory: {expected}", file=sys.stderr)
            return 1

    if arguments.check_c_only and not inventory["c_only_gate"]["passes"]:
        gate = inventory["c_only_gate"]
        print(
            "C-only gate failed: "
            f"{len(gate['linked_project_asm_objects'])} linked ASM objects, "
            f"{len(gate['inline_asm'])} inline ASM blocks, "
            f"{len(gate['ported_asm_references'])} ported ASM references, and "
            f"{len(gate['dseg_references'])} referenced dseg symbols remain.",
            file=sys.stderr,
        )
        return 1
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
