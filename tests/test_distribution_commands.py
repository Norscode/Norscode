from __future__ import annotations

import subprocess


def _run(cmd: list[str]) -> subprocess.CompletedProcess[str]:
    return subprocess.run(cmd, text=True, capture_output=True, check=False)


def test_nc_version() -> None:
    result = _run(["./bin/nc", "--version"])
    assert result.returncode == 0, result.stderr
    assert "Norscode" in result.stdout
    assert "Python-fallback" not in result.stderr


def test_nc_help_is_native() -> None:
    result = _run(["./bin/nc", "--help"])
    assert result.returncode == 0, result.stderr
    assert "Python-fallback" not in result.stderr


def test_nc_doctor() -> None:
    result = _run(["./bin/nc", "--python-fallback", "doctor"])
    assert result.returncode == 0, result.stderr
    assert "Doctor OK: ja" in result.stdout
    assert "Python-fallback" in result.stderr


def test_nc_smoke() -> None:
    result = _run(["./bin/nc", "--python-fallback", "smoke"])
    assert result.returncode == 0, result.stderr
    assert "Release:" in result.stdout


def test_nc_smoke_requires_explicit_fallback() -> None:
    result = _run(["./bin/nc", "smoke"])
    assert result.returncode != 0
    assert "--python-fallback" in result.stderr


def test_nc_python_commands_require_explicit_fallback() -> None:
    result = _run(["./bin/nc", "doctor"])
    assert result.returncode != 0
    assert "--python-fallback" in result.stderr
