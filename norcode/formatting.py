"""Shared formatter helpers for CLI commands."""

from __future__ import annotations

import difflib
from pathlib import Path

from compiler.formatter import format_source


def resolve_source_path(source_file: str) -> Path:
    path = Path(source_file).expanduser().resolve()
    if not path.exists():
        raise RuntimeError(f"Fant ikke kildefil: {path}")
    return path


def format_program_file(source_file: str, check: bool = False, diff: bool = False) -> dict[str, object]:
    source_path = resolve_source_path(source_file)
    original = source_path.read_text(encoding="utf-8")
    formatted = format_source(original)
    changed = formatted != original
    diff_lines = None

    if diff:
        diff_lines = list(
            difflib.unified_diff(
                original.splitlines(),
                formatted.splitlines(),
                fromfile=str(source_path),
                tofile=f"{source_path} (formatted)",
                lineterm="",
            )
        )
        if diff_lines:
            print("\n".join(diff_lines))
        else:
            print(f"Ingen endringer for {source_path}")

    if check:
        return {
            "source": str(source_path),
            "changed": changed,
            "written": False,
            "diff": diff_lines,
        }

    if changed and not diff:
        source_path.write_text(formatted, encoding="utf-8")

    return {
        "source": str(source_path),
        "changed": changed,
        "written": changed and not diff,
        "diff": diff_lines,
    }
