#!/usr/bin/env python3
"""Run the DOS ABI regression in isolation and stop DOSBox with SIGKILL."""

import argparse
import math
import os
from pathlib import Path
import shutil
import subprocess
import tempfile
import time


def main():
    repository = Path(__file__).resolve().parents[2]
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument(
        "--executable",
        type=Path,
        default=repository / "src/restunts/tests/build/watcom/release/DOSPLAT.EXE",
    )
    parser.add_argument("--dosbox", default="dosbox-x")
    parser.add_argument("--timeout", type=float, default=30)
    args = parser.parse_args()
    if not args.executable.is_file():
        parser.error(f"DOS test executable is missing: {args.executable}")
    if not math.isfinite(args.timeout) or args.timeout <= 0:
        parser.error("--timeout must be finite and positive")

    with tempfile.TemporaryDirectory(prefix="restunts-dos-platform-") as temporary:
        directory = Path(temporary)
        shutil.copyfile(args.executable, directory / "DOSPLAT.EXE")
        environment = dict(os.environ, SDL_VIDEODRIVER="dummy", SDL_AUDIODRIVER="dummy")
        command = [
            args.dosbox,
            "-silent",
            "-conf",
            str(repository / "tools/scripts/dosbox.proc.conf"),
            "-c",
            f'mount c "{directory}"',
            "-c",
            "c:",
            "-c",
            "DOSPLAT.EXE > RESULT.TXT",
            "-c",
            "if errorlevel 1 echo FAILED > STATUS.TXT",
            "-c",
            "echo DONE > DONE.TXT",
        ]
        with (directory / "dosbox.log").open("w") as log:
            process = subprocess.Popen(
                command, cwd=directory, env=environment, stdout=log, stderr=subprocess.STDOUT
            )
            try:
                deadline = time.monotonic() + args.timeout
                while not (directory / "DONE.TXT").exists():
                    if process.poll() is not None or time.monotonic() >= deadline:
                        break
                    time.sleep(0.1)
            finally:
                if process.poll() is None:
                    process.kill()
                process.wait()

        result_path = directory / "RESULT.TXT"
        result = result_path.read_text(errors="replace") if result_path.exists() else ""
        status_path = directory / "STATUS.TXT"
        status = status_path.read_text(errors="replace") if status_path.exists() else ""
        success = (
            (directory / "DONE.TXT").exists()
            and not status.strip()
            and result.strip() == "DOS platform ABI checks passed"
        )
        if not success:
            print(result or "DOS platform test did not finish before DOSBox stopped.")
            print((directory / "dosbox.log").read_text(errors="replace"))
            return 1
        print(result.strip())
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
