from __future__ import annotations

import subprocess


def _run(cmd: list[str]) -> subprocess.CompletedProcess[str]:
    return subprocess.run(cmd, text=True, capture_output=True, check=False)


def test_selfhost_build_command() -> None:
    result = _run(["./bin/nc", "selfhost-build"])
    assert result.returncode == 0, result.stderr
    assert "Selfhost build OK" in result.stdout


def test_selfhost_check_command() -> None:
    result = _run(["./bin/nc", "selfhost-check"])
    assert result.returncode == 0, result.stderr
    assert "Bootstrap check OK" in result.stdout
