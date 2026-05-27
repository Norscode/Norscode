from __future__ import annotations

import subprocess
import sys
from pathlib import Path


def test_format_command_writes_formatted_source(tmp_path: Path) -> None:
    source = tmp_path / "demo.no"
    source.write_text(
        "funksjon start() -> heltall {\n    la x=1+2\n    returner x\n}\n",
        encoding="utf-8",
    )

    completed = subprocess.run(
        [sys.executable, "-m", "norcode", "format", str(source)],
        cwd=Path(__file__).resolve().parents[1],
        text=True,
        capture_output=True,
        check=True,
    )

    assert "Formatert:" in completed.stdout
    assert source.read_text(encoding="utf-8") == (
        "funksjon start() -> heltall {\n"
        "    la x = 1 + 2\n"
        "    returner x\n"
        "}\n"
    )


def test_format_check_reports_unformatted_source(tmp_path: Path) -> None:
    source = tmp_path / "demo_check.no"
    source.write_text(
        "funksjon start() -> heltall {\n    la x=1+2\n    returner x\n}\n",
        encoding="utf-8",
    )

    completed = subprocess.run(
        [sys.executable, "-m", "norcode.legacy_main", "format", str(source), "--check"],
        cwd=Path(__file__).resolve().parents[1],
        text=True,
        capture_output=True,
    )

    assert completed.returncode == 1
    assert "Uformatert:" in completed.stdout
