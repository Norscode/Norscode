"""Distribution command tests — verifiser nc-vm-baserte distribusjonskommandoar."""
from __future__ import annotations

import subprocess


def _run(cmd: list[str]) -> subprocess.CompletedProcess[str]:
    return subprocess.run(cmd, text=True, capture_output=True, check=False)


def test_nc_version() -> None:
    """bin/nc --version skal fungere utan Python."""
    result = _run(["./bin/nc", "--version"])
    assert result.returncode == 0, result.stderr
    assert "Norscode" in result.stdout


def test_nc_help_is_native() -> None:
    """bin/nc help skal bruke native (Python-fri) bane."""
    result = _run(["./bin/nc", "help"])
    assert result.returncode == 0, result.stderr
    assert "Python-fallback" not in result.stderr


def test_nc_run_is_python_free() -> None:
    """bin/nc run skal ikkje trenge Python."""
    import tempfile, os
    with tempfile.NamedTemporaryFile(suffix=".no", delete=False, mode="w") as f:
        f.write('funksjon start() { skriv("nc-run-test") }\n')
        tmp = f.name
    try:
        result = _run(["./bin/nc", "run", tmp])
        assert result.returncode == 0, result.stderr
        assert "Python-fallback" not in result.stderr
        assert "nc-run-test" in result.stdout
    finally:
        os.unlink(tmp)


def test_nc_test_is_python_free() -> None:
    """bin/nc test skal ikkje trenge Python."""
    result = _run(["./bin/nc", "test"])
    assert result.returncode == 0, result.stderr
    assert "Python-fallback" not in result.stderr
    assert "bestått" in result.stdout or "OK" in result.stdout


def test_nc_commands_shows_python_free_commands() -> None:
    """bin/nc commands skal vise Python-fri kommandoliste."""
    result = _run(["./bin/nc", "commands"])
    assert result.returncode == 0, result.stderr
    assert "run" in result.stdout
    assert "test" in result.stdout


def test_nc_doctor_is_python_free() -> None:
    """bin/nc doctor skal kjøre utan Python."""
    result = _run(["./bin/nc", "doctor"])
    assert result.returncode == 0, result.stderr
    assert "Python-fallback" not in result.stderr
    assert "OK" in result.stdout or "✓" in result.stdout

def test_nc_python_commands_require_explicit_fallback() -> None:
    """Kommandoar utan nc-vm-støtte skal krevje --legacy-python-fallback."""
    result = _run(["./bin/nc", "serve"])
    assert result.returncode != 0
    assert "python-fallback" in result.stderr.lower()
