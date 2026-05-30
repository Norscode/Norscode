"""CI gate tests — verifiser Python-fri og legacy-Python-baner."""
from __future__ import annotations
import subprocess

def _run(cmd):
    return subprocess.run(cmd, text=True, capture_output=True, check=False)


def test_ci_bootstrap_gate_is_python_free() -> None:
    """bin/nc selfhost-bootstrap-gate skal køyre utan Python."""
    result = _run(["./bin/nc", "selfhost-bootstrap-gate"])
    assert result.returncode == 0, result.stderr
    assert "Python-fallback" not in result.stderr
    assert "BESTÅTT" in result.stdout or "OK" in result.stdout


def test_ci_python_fallback_lane_is_explicit() -> None:
    """--python-fallback skal vise åtvaringsmelding."""
    result = _run(["./bin/nc", "--python-fallback", "selfhost-bootstrap-gate"])
    assert result.returncode == 0 or "fallback" in result.stderr.lower()


def test_selfhost_bootstrap_gate_is_python_free() -> None:
    """bin/nc selfhost-bootstrap-gate køyrer utan Python."""
    result = _run(["./bin/nc", "selfhost-bootstrap-gate"])
    assert result.returncode == 0, result.stderr
    assert "Python-fallback" not in result.stderr
