from __future__ import annotations

import subprocess
import sys
from pathlib import Path


def test_frontend_example_runs_through_modular_norcode_cli() -> None:
    completed = subprocess.run(
        [sys.executable, "-m", "norcode", "run", "examples/frontend.no"],
        cwd=Path(__file__).resolve().parents[1],
        text=True,
        capture_output=True,
        check=True,
    )

    assert "text/html; charset=utf-8" in completed.stdout
    assert "<title>Norscode Frontend</title>" in completed.stdout
    assert "<header class=\"app-header\">" in completed.stdout
    assert "<nav class=\"nav\">" in completed.stdout
    assert "<section class=\"card panel\">" in completed.stdout
    assert "<footer class=\"app-footer\">" in completed.stdout
