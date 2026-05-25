from __future__ import annotations

import subprocess


def _run(cmd: list[str]) -> subprocess.CompletedProcess[str]:
    return subprocess.run(cmd, text=True, capture_output=True, check=False)


def test_ci_bootstrap_lane_is_native() -> None:
    result = _run(["./bin/nc", "ci", "--bootstrap-lane"])
    assert result.returncode == 0, result.stderr
    assert "Python-fallback" not in result.stderr
    assert "Selfhost bootstrap gate" in result.stdout or "OK" in result.stdout


def test_ci_python_fallback_lane_is_explicit() -> None:
    result = _run(["./bin/nc", "--python-fallback", "smoke"])
    assert result.returncode == 0, result.stderr
    assert "Python-fallback" in result.stderr
    assert "OK" in result.stdout
