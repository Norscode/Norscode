from __future__ import annotations

import subprocess
import sys
from pathlib import Path

def test_render_ui_command(tmp_path: Path) -> None:
    ui_file = tmp_path / "demo.nui"
    ui_file.write_text(
        """
side:
    tittel "Demo"
    kort:
        tittel "En"
        tekst "To"
""".strip(),
        encoding="utf-8",
    )
    output_file = tmp_path / "demo.html"
    completed = subprocess.run(
        [sys.executable, "main.py", "ui-render", str(ui_file), "-o", str(output_file), "--title", "Demo"],
        cwd=Path(__file__).resolve().parents[1],
        text=True,
        capture_output=True,
        check=True,
    )

    assert completed.stdout == ""
    html = output_file.read_text(encoding="utf-8")
    assert "<title>Demo</title>" in html
    assert "<main class=\"shell\">" in html
    assert "<section class=\"card panel\">" in html
    assert "<h2>En</h2>" in html
    assert "<p>To</p>" in html
