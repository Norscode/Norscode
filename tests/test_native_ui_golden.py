from __future__ import annotations

import subprocess
import sys
from pathlib import Path


def test_native_ui_golden_example_renders_html(tmp_path: Path) -> None:
    output_file = tmp_path / "native_ui_golden.html"
    completed = subprocess.run(
        [
            sys.executable,
            "-m",
            "norcode",
            "ui-render",
            "examples/native_ui_golden.nui",
            "-o",
            str(output_file),
            "--title",
            "Norscode Frontend",
        ],
        cwd=Path(__file__).resolve().parents[1],
        text=True,
        capture_output=True,
        check=True,
    )

    assert completed.stdout == ""
    html = output_file.read_text(encoding="utf-8")
    assert "<title>Norscode Frontend</title>" in html
    assert "<section class=\"hero\">" in html
    assert "<section class=\"card panel\">" in html
    assert "<ul class=\"list\">" in html
    assert "<section class=\"footer\">" in html
