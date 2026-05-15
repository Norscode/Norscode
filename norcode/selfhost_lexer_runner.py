"""Selfhost lexer runner boundary.

This module is the planned execution boundary for running the Norscode-native
lexer (`selfhost/lexer/lexer_m1.no`) through the Norscode runtime.

The runner now routes through `runtime_call_service`, which becomes the stable
ABI boundary between selfhost compiler components and the VM/runtime.
"""

from __future__ import annotations

from dataclasses import dataclass, field
from pathlib import Path
from typing import Any

from norcode.runtime_call_service import call_function
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

    runtime_result = call_function(str(status.path), "lex", [source_path.read_text(encoding="utf-8")])

    return SelfhostLexerRunResult(
        ok=runtime_result.ok,
        ready=True,
        lexer_path=status.path,
        source_path=source_path,
        tokens=runtime_result.value or [],
        errors=list(runtime_result.errors),
    )
