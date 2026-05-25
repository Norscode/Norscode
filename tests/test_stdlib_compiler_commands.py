from __future__ import annotations

import subprocess


def _run(cmd: list[str]) -> subprocess.CompletedProcess[str]:
    return subprocess.run(cmd, text=True, capture_output=True, check=False)


def test_selfhost_stdlib_suite_command() -> None:
    result = _run(["./bin/nc", "selfhost-stdlib-suite"])
    assert result.returncode == 0, result.stderr
    assert "Selfhost stdlib suite: 1/1 OK" in result.stdout

