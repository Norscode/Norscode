from __future__ import annotations

import json
import subprocess


def test_fuzz_command_reports_expected_negative_cases() -> None:
    result = subprocess.run(
        ["python3", "-m", "norcode", "fuzz", "--json"],
        text=True,
        capture_output=True,
        check=False,
    )

    assert result.returncode == 0, result.stderr
    payload = json.loads(result.stdout)
    assert payload["ok"] is True
    assert len(payload["parser_cases"]) == 5
    assert len(payload["runtime_cases"]) == 1
    assert payload["parser_failures"] == 0
    assert payload["runtime_failures"] == 0


def test_fuzz_help_is_modular() -> None:
    result = subprocess.run(
        ["python3", "-m", "norcode", "fuzz", "--help"],
        text=True,
        capture_output=True,
        check=False,
    )
    assert result.returncode == 0, result.stderr
    assert "Skriv fuzz-resultat som JSON" in result.stdout
