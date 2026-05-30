"""CI gate tests — verifiser Python-fri baner."""
from __future__ import annotations
import subprocess


def _run(cmd):
    return subprocess.run(cmd, text=True, capture_output=True, check=False)


def test_ci_bootstrap_gate_is_python_free() -> None:
    """bin/nc selfhost-bootstrap-gate skal køyre utan Python."""
    result = _run(["./bin/nc", "selfhost-bootstrap-gate"])
    assert result.returncode == 0, result.stderr
    assert "Python-fallback" not in result.stderr


def test_legacy_flag_is_silently_ignored() -> None:
    """--legacy-python-fallback vert stille ignorert (ikkje lenger støtta)."""
    result = _run(["./bin/nc", "--legacy-python-fallback", "selfhost-bootstrap-gate"])
    assert result.returncode == 0, result.stderr
    assert "Python-fallback" not in result.stderr


def test_selfhost_bootstrap_gate_is_python_free() -> None:
    """bin/nc selfhost-bootstrap-gate køyrer utan Python."""
    result = _run(["./bin/nc", "selfhost-bootstrap-gate"])
    assert result.returncode == 0, result.stderr
    assert "Python-fallback" not in result.stderr
