"""Exercise the POSIX client with a controlled curl process and temporary files."""

import json
import os
from pathlib import Path
import shutil
import subprocess
import tempfile
import unittest


CLIENT = Path(__file__).resolve().parents[1] / "dumpsrv-client.sh"


class ClientTests(unittest.TestCase):
    def setUp(self):
        self.temporary = tempfile.TemporaryDirectory(prefix="dumpsrv-client-tests-")
        self.addCleanup(self.temporary.cleanup)
        self.directory = Path(self.temporary.name)
        self.client = self.directory / "dumpsrv-client.sh"
        shutil.copy2(CLIENT, self.client)
        for name in ("REPLDUMP.EXE", "PIXLDUMP.EXE"):
            (self.directory / name).write_bytes(b"test executable")
        self.arguments = self.directory / "curl-arguments.json"
        self.output = self.directory / "partitions_all.txt"
        curl = self.directory / "curl"
        curl.write_text(
            "#!/usr/bin/env python3\n"
            "import json, os, pathlib, sys\n"
            "args = sys.argv[1:]\n"
            "pathlib.Path(os.environ['CURL_ARGUMENTS']).write_text(json.dumps(args))\n"
            "pathlib.Path(args[args.index('--output') + 1]).write_text('new report')\n"
            "sys.exit(int(os.environ.get('CURL_STATUS', '0')))\n",
            encoding="utf-8",
        )
        curl.chmod(0o755)
        self.environment = dict(
            os.environ,
            PATH=str(self.directory) + os.pathsep + os.environ["PATH"],
            DUMPSRV_API_KEY="test-key",
            DUMPSRV_URL="http://example.invalid/process",
            CURL_ARGUMENTS=str(self.arguments),
            CURL_STATUS="0",
        )

    def invoke(self, *arguments, status=0):
        result = subprocess.run(
            ["sh", str(self.client), *arguments],
            env=self.environment,
            text=True,
            capture_output=True,
            timeout=10,
            check=False,
        )
        self.assertEqual(status, result.returncode, result.stderr)
        return result

    def captured(self):
        return json.loads(self.arguments.read_text(encoding="utf-8"))

    def test_default_timeout_and_request_contract(self):
        self.invoke()
        arguments = self.captured()
        self.assertEqual("1860", arguments[arguments.index("--max-time") + 1])
        self.assertEqual("10", arguments[arguments.index("--connect-timeout") + 1])
        self.assertEqual("POST", arguments[arguments.index("--request") + 1])
        self.assertIn("X-API-Key: test-key", arguments)
        self.assertIn("physics_tests=true", arguments)
        self.assertIn("renderer_tests=true", arguments)
        self.assertEqual("http://example.invalid/process", arguments[-1])
        self.assertEqual("new report", self.output.read_text())

    def test_timeout_forms_and_option_positions(self):
        cases = (
            (("--timeout-seconds", "3600"), "3600"),
            (("--timeout-seconds=75",), "75"),
            (("http://other.invalid/process", "--timeout-seconds", "0009"), "0009"),
        )
        for options, expected in cases:
            with self.subTest(options=options):
                self.invoke(*options)
                arguments = self.captured()
                self.assertEqual(expected, arguments[arguments.index("--max-time") + 1])

    def test_invalid_timeouts_do_not_send_a_request(self):
        cases = (
            ("--timeout-seconds",),
            ("--timeout-seconds=",),
            ("--timeout-seconds", "0"),
            ("--timeout-seconds=000",),
            ("--timeout-seconds=-1",),
            ("--timeout-seconds=1.2",),
            ("--timeout-seconds=oops",),
        )
        self.output.write_text("existing report")
        for options in cases:
            with self.subTest(options=options):
                self.invoke(*options, status=2)
                self.assertFalse(self.arguments.exists())
                self.assertEqual("existing report", self.output.read_text())

    def test_transfer_failures_preserve_report_and_remove_temporary_output(self):
        for status in (22, 28):
            with self.subTest(curl_status=status):
                self.environment["CURL_STATUS"] = str(status)
                self.output.write_text("existing report")
                self.invoke("--timeout-seconds=10", status=1)
                self.assertEqual("existing report", self.output.read_text())
                self.assertEqual([], list(self.directory.glob(".partitions_all.txt.*")))


if __name__ == "__main__":
    unittest.main()
