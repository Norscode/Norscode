"""Selfhost lexer runner boundary.

This module is the execution boundary for running the Norscode-native lexer
(`selfhost/lexer/lexer_m1.no`) through the Norscode runtime.

The runner routes through `runtime_call_service`, validates the returned token
stream, and returns structured diagnostics for the phase-1 QA suite.
"""

from __future__ import annotations

from dataclasses import dataclass, field
from pathlib import Path
from typing import Any

from norcode.runtime_call_service import call_function
from norcode.selfhost_lexer_service import DEFAULT_SELFHOST_LEXER, check_selfhost_lexer
from norcode.token_validator import validate_token_stream


@dataclass(frozen=True)
class SelfhostLexerRunResult:
    ok: bool
    ready: bool
    lexer_path: Path
    source_path: Path | None = None
    tokens: list[dict[str, Any]] = field(default_factory=list)
    errors: list[str] = field(default_factory=list)
    validation_errors: list[str] = field(default_factory=list)
    available_functions: list[str] = field(default_factory=list)
    candidate_functions: list[str] = field(default_factory=list)
    called_function: str | None = None



def _normalize_runtime_tokens(value: Any) -> list[dict[str, Any]]:
    if value is None:
        return []
    if not isinstance(value, list):
        return []
    result = []
    for item in value:
        if not isinstance(item, dict):
            continue
        if item.get("type") == "EOF" and item.get("value") == "":
            result.append({**item, "value": None})
        else:
            result.append(item)
    return result



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
    tokens = _normalize_runtime_tokens(runtime_result.value)
    validation = validate_token_stream(tokens)
    validation_errors = validation.errors if runtime_result.ok else []

    return SelfhostLexerRunResult(
        ok=runtime_result.ok and validation.ok,
        ready=True,
        lexer_path=status.path,
        source_path=source_path,
        tokens=tokens,
        errors=list(runtime_result.errors),
        validation_errors=list(validation_errors),
        available_functions=list(runtime_result.available_functions),
        candidate_functions=list(runtime_result.candidate_functions),
        called_function=runtime_result.function_name if runtime_result.ok else None,
    )
