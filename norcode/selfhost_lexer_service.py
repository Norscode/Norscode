"""Selfhost lexer service for Norscode.

This service tracks the readiness of the Norscode-native lexer implementation.
It does not replace the Python lexer yet; it verifies that the selfhost lexer
source contains the required M1 building blocks before the runtime runner and
parity integration are enabled.
"""

from __future__ import annotations

from dataclasses import dataclass, field
from pathlib import Path


DEFAULT_SELFHOST_LEXER = Path("selfhost/lexer/lexer_m1.no")


REQUIRED_M1_FUNCTIONS = (
    "er_bokstav",
    "er_tall",
    "er_ident_tegn",
    "er_whitespace",
    "er_keyword",
    "token_type_for_ident",
    "les_ident",
    "les_tall",
    "les_tekst",
    "les_operator",
    "hopp_kommentar",
    "lex",
)


REQUIRED_M1_TOKEN_MARKERS = (
    "IDENT",
    "NUMBER",
    "STRING",
    "OP",
    "LPAREN",
    "RPAREN",
    "LBRACE",
    "RBRACE",
    "LBRACKET",
    "RBRACKET",
    "COMMA",
    "COLON",
    "EOF",
    "UNKNOWN",
)


@dataclass(frozen=True)
class SelfhostLexerStatus:
    path: Path
    exists: bool
    ok: bool
    missing_functions: list[str] = field(default_factory=list)
    missing_token_markers: list[str] = field(default_factory=list)



def check_selfhost_lexer(path: str | Path = DEFAULT_SELFHOST_LEXER) -> SelfhostLexerStatus:
    lexer_path = Path(path).expanduser()
    if not lexer_path.is_absolute():
        lexer_path = Path.cwd() / lexer_path
    lexer_path = lexer_path.resolve()

    if not lexer_path.exists():
        return SelfhostLexerStatus(
            path=lexer_path,
            exists=False,
            ok=False,
            missing_functions=list(REQUIRED_M1_FUNCTIONS),
            missing_token_markers=list(REQUIRED_M1_TOKEN_MARKERS),
        )

    source = lexer_path.read_text(encoding="utf-8")
    missing_functions = [name for name in REQUIRED_M1_FUNCTIONS if f"funksjon {name}" not in source]
    missing_token_markers = [marker for marker in REQUIRED_M1_TOKEN_MARKERS if f'"{marker}"' not in source]

    ok = not missing_functions and not missing_token_markers
    return SelfhostLexerStatus(
        path=lexer_path,
        exists=True,
        ok=ok,
        missing_functions=missing_functions,
        missing_token_markers=missing_token_markers,
    )
