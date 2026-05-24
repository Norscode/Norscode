from __future__ import annotations

import subprocess
import sys
from pathlib import Path


def test_frontend_golden_example_renders_component_mode() -> None:
    completed = subprocess.run(
        [sys.executable, "-m", "norcode", "run", "examples/frontend_golden.no"],
        cwd=Path(__file__).resolve().parents[1],
        text=True,
        capture_output=True,
        check=True,
    )

    assert completed.stdout.endswith("</html>")
    assert "<title>Norscode Frontend</title>" in completed.stdout
    assert "<header class=\"app-header\">" in completed.stdout
    assert "<div class=\"hero\">" in completed.stdout
    assert "<section class=\"card panel\">" in completed.stdout
    assert "<ul class=\"list\">" in completed.stdout
    assert "<footer class=\"app-footer\">" in completed.stdout
