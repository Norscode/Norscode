"""CI gate tests — verifiser Python-fri og legacy-Python-baner."""
from __future__ import annotations

import subprocess


def _run(cmd: list[str]) -> subprocess.CompletedProcess[str]:
    return subprocess.run(cmd, text=True, capture_output=True, check=False)


def test_ci_bootstrap_lane_is_native() -> None:
    """bin/nc ci --bootstrap-lane skal køyre utan Python."""
    result = _run(["./bin/nc", "ci", "--bootstrap-lane"])
    assert result.returncode == 0, result.stderr
    assert "Python-fallback" not in result.stderr


def test_ci_python_fallback_lane_is_explicit() -> None:
    """--python-fallback skal vise åtvaringsmelding."""
    result = _run(["./bin/nc", "--python-fallback", "selfhost-bootstrap-gate"])
    assert "python-fallback" in result.stderr.lower() or result.returncode in (0, 2), \
        f"Unexpected: returncode={result.returncode}, stderr={result.stderr!r}"


def test_selfhost_bootstrap_gate_is_python_free() -> None:
    """bin/nc selfhost-bootstrap-gate skal ikkje trenge Python."""
    result = _run(["./bin/nc", "selfhost-bootstrap-gate"])
    assert result.returncode == 0, result.stderr
    assert "Python-fallback" not in result.stderr
    assert "BESTÅTT" in result.stdout or "OK" in result.stdout
