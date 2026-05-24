from __future__ import annotations

import subprocess
import sys
from pathlib import Path


def test_native_ui_reports_indent_errors_without_python_traceback(tmp_path: Path) -> None:
    ui_file = tmp_path / "bad_indent.nui"
    ui_file.write_text("    side:\n        kort:\n            tittel \"Feil\"\n", encoding="utf-8")

    output_file = tmp_path / "bad_indent.html"
    completed = subprocess.run(
        [
            sys.executable,
            "-m",
            "norcode",
            "ui-render",
            str(ui_file),
            "-o",
            str(output_file),
        ],
        cwd=Path(__file__).resolve().parents[1],
        text=True,
        capture_output=True,
    )

    assert completed.returncode != 0
    assert completed.stdout == ""
    assert "Traceback" not in completed.stderr
    assert "UIFeil: toppnivå må ikke være innrykket" in completed.stderr


def test_native_ui_reports_nested_indent_errors_without_python_traceback(tmp_path: Path) -> None:
    ui_file = tmp_path / "bad_nested.nui"
    ui_file.write_text(
        """
side:
    hero:
          tittel "Feil"
""".lstrip(),
        encoding="utf-8",
    )

    completed = subprocess.run(
        [
            sys.executable,
            "-m",
            "norcode",
            "ui-render",
            str(ui_file),
        ],
        cwd=Path(__file__).resolve().parents[1],
        text=True,
        capture_output=True,
    )

    assert completed.returncode != 0
    assert completed.stdout == ""
    assert "Traceback" not in completed.stderr
    assert "UIFeil: uventet innrykk" in completed.stderr
