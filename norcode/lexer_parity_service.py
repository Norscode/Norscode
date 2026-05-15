"""Lexer parity service for Norscode.

This service exports the current Python lexer token stream in a stable JSON-like
shape.  Future Norscode-native lexers must match this output before they can
replace the Python lexer in the compiler frontend.
"""

from __future__ import annotations

from dataclasses import asdict, dataclass, field
from pathlib import Path
from typing import Any

from norcode.frontend_service import lex_source_file, lex_source_text
from norcode.token_validator import validate_token_stream


@dataclass(frozen=True)
class LexerParityResult:
    source_path: Path | None
    tokens: list[dict[str, Any]]
    validation_ok: bool = True
    validation_errors: list[str] = field(default_factory=list)



def _validated_result(source_path: Path | None, tokens: list[dict[str, Any]]) -> LexerParityResult:
    validation = validate_token_stream(tokens)
    return LexerParityResult(
        source_path=source_path,
        tokens=tokens,
        validation_ok=validation.ok,
        validation_errors=list(validation.errors),
    )



def tokens_from_current_lexer_text(source: str) -> LexerParityResult:
    tokens = [asdict(token) for token in lex_source_text(source)]
    return _validated_result(None, tokens)



def tokens_from_current_lexer_file(source_file: str) -> LexerParityResult:
    source_path = Path(source_file).expanduser().resolve()
    tokens = [asdict(token) for token in lex_source_file(str(source_path))]
    return _validated_result(source_path, tokens)
