from __future__ import annotations

import subprocess
import sys
from pathlib import Path


def test_frontend_css_class_families_stay_stable() -> None:
    repo_root = Path(__file__).resolve().parents[1]
    output_file = repo_root / "tmp-native-ui-golden.html"

    component = subprocess.run(
        [sys.executable, "-m", "norcode", "run", "examples/frontend_golden.no"],
        cwd=repo_root,
        text=True,
        capture_output=True,
        check=True,
    ).stdout

    native_ui_output = subprocess.run(
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
        cwd=repo_root,
        text=True,
        capture_output=True,
        check=True,
    )

    native_ui = output_file.read_text(encoding="utf-8")

    assert "<header class=\"app-header\">" in component
    assert "<main class=\"app-main\">" in component
    assert "<footer class=\"app-footer\">" in component
    assert "<div class=\"hero\">" in component
    assert "<section class=\"card panel\">" in component
    assert "<ul class=\"list\">" in component

    assert "<section class=\"hero\">" in native_ui
    assert "<section class=\"card panel\">" in native_ui
    assert "<ul class=\"list\">" in native_ui
    assert "<section class=\"footer\">" in native_ui

    if output_file.exists():
        output_file.unlink()
