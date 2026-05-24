from __future__ import annotations

import subprocess
import sys
from pathlib import Path


def test_lint_command_reports_unused_import(tmp_path: Path) -> None:
    source = tmp_path / "demo.no"
    source.write_text(
        "bruk std.tekst som t\n\nfunksjon start() -> heltall {\n    returner 0\n}\n",
        encoding="utf-8",
    )

    completed = subprocess.run(
        [sys.executable, "-m", "norcode", "lint", str(source)],
        cwd=Path(__file__).resolve().parents[1],
        text=True,
        capture_output=True,
        check=True,
    )

    assert "FEIL:" in completed.stdout
    assert "Ubrukt import" in completed.stdout


def test_lint_check_exits_nonzero_on_issues(tmp_path: Path) -> None:
    source = tmp_path / "demo_check.no"
    source.write_text(
        "bruk std.tekst som t\n\nfunksjon start() -> heltall {\n    returner 0\n}\n",
        encoding="utf-8",
    )

    completed = subprocess.run(
        [sys.executable, "main.py", "lint", str(source), "--check"],
        cwd=Path(__file__).resolve().parents[1],
        text=True,
        capture_output=True,
    )

    assert completed.returncode == 1
    assert "FEIL:" in completed.stdout
