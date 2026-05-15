"""Selfhost lexer runner boundary.

This module is the planned execution boundary for running the Norscode-native
lexer (`selfhost/lexer/lexer_m1.no`) through the Norscode runtime.

Today the runner is intentionally conservative: it verifies readiness and
returns a structured status instead of pretending the selfhost lexer is already
wired into the VM call ABI.  The next implementation step is to expose a stable
runtime entrypoint that can call `lex(kilde)` and collect the token list.
"""

from __future__ import annotations

from dataclasses import dataclass, field
from pathlib import Path
from typing import Any

from norcode.selfhost_lexer_service import DEFAULT_SELFHOST_LEXER, check_selfhost_lexer


@dataclass(frozen=True)
class SelfhostLexerRunResult:
    ok: bool
    ready: bool
    lexer_path: Path
    source_path: Path | None = None
    tokens: list[dict[str, Any]] = field(default_factory=list)
    errors: list[str] = field(default_factory=list)



def run_selfhost_lexer(source_file: str, lexer_path: str | Path = DEFAULT_SELFHOST_LEXER) -> SelfhostLexerRunResult:
    """Run the selfhost lexer once the VM call ABI is available.

    For now this function acts as a safe integration boundary.  It prevents the
    CLI and CI from depending on unfinished runtime behavior while still giving
    the project a stable API to target.
    """
    status = check_selfhost_lexer(lexer_path)
    source_path = Path(source_file).expanduser().resolve()

    if not status.ok:
        errors = []
        if not status.exists:
            errors.append("selfhost lexer file does not exist")
        for item in status.missing_functions:
            errors.append(f"missing function: {item}")
        for item in status.missing_token_markers:
            errors.append(f"missing token marker: {item}")
        return SelfhostLexerRunResult(
            ok=False,
            ready=False,
            lexer_path=status.path,
            source_path=source_path,
            errors=errors,
        )

    return SelfhostLexerRunResult(
        ok=False,
        ready=True,
        lexer_path=status.path,
        source_path=source_path,
        errors=["runtime call ABI for selfhost lexer is not connected yet"],
    )
