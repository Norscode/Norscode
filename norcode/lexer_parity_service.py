"""Lexer parity service for Norscode.

This service exports the current Python lexer token stream in a stable JSON-like
shape.  Future Norscode-native lexers must match this output before they can
replace the Python lexer in the compiler frontend.
"""

from __future__ import annotations

from dataclasses import asdict, dataclass
from pathlib import Path
from typing import Any

from norcode.frontend_service import lex_source_file, lex_source_text


@dataclass(frozen=True)
class LexerParityResult:
    source_path: Path | None
    tokens: list[dict[str, Any]]



def tokens_from_current_lexer_text(source: str) -> LexerParityResult:
    tokens = [asdict(token) for token in lex_source_text(source)]
    return LexerParityResult(source_path=None, tokens=tokens)



def tokens_from_current_lexer_file(source_file: str) -> LexerParityResult:
    source_path = Path(source_file).expanduser().resolve()
    tokens = [asdict(token) for token in lex_source_file(str(source_path))]
    return LexerParityResult(source_path=source_path, tokens=tokens)
