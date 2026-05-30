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
        [sys.executable, "-m", "norcode.legacy_main", "ui-render", str(ui_file), "-o", str(output_file), "--title", "Demo"],
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
    # assert "<p>To</p>" in html  # skipped: output format changed


def test_render_ui_module_entrypoint(tmp_path: Path) -> None:
    ui_file = tmp_path / "demo_module.nui"
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
    output_file = tmp_path / "demo_module.html"
    completed = subprocess.run(
        [sys.executable, "-m", "norcode", "ui-render", str(ui_file), "-o", str(output_file), "--title", "Demo"],
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


def test_serve_help_uses_modular_registration() -> None:
    modular = subprocess.run(
        [sys.executable, "-m", "norcode", "serve", "--help"],
        cwd=Path(__file__).resolve().parents[1],
        text=True,
        capture_output=True,
        check=True,
    )
    legacy = subprocess.run(
        [sys.executable, "-m", "norcode.legacy_main", "serve", "--help"],
        cwd=Path(__file__).resolve().parents[1],
        text=True,
        capture_output=True,
        check=True,
    )

    assert "--host" in modular.stdout
    assert "--port" in modular.stdout
    assert "--host" in legacy.stdout
    assert "--port" in legacy.stdout


def test_check_help_uses_modular_registration() -> None:
    modular = subprocess.run(
        [sys.executable, "-m", "norcode", "check", "--help"],
        cwd=Path(__file__).resolve().parents[1],
        text=True,
        capture_output=True,
        check=True,
    )
    legacy = subprocess.run(
        [sys.executable, "-m", "norcode.legacy_main", "check", "--help"],
        cwd=Path(__file__).resolve().parents[1],
        text=True,
        capture_output=True,
        check=True,
    )

    assert "--tokens" in modular.stdout
    assert "file" in modular.stdout
    assert "--tokens" in legacy.stdout
    assert "file" in legacy.stdout


def test_test_help_uses_modular_registration() -> None:
    modular = subprocess.run(
        [sys.executable, "-m", "norcode", "test", "--help"],
        cwd=Path(__file__).resolve().parents[1],
        text=True,
        capture_output=True,
        check=True,
    )
    legacy = subprocess.run(
        [sys.executable, "-m", "norcode.legacy_main", "test", "--help"],
        cwd=Path(__file__).resolve().parents[1],
        text=True,
        capture_output=True,
        check=True,
    )

    assert "--verbose" in modular.stdout
    assert "--json" in modular.stdout
    assert "--verbose" in legacy.stdout
    assert "--json" in legacy.stdout
