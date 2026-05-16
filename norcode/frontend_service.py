"""Frontend service facade for lexer/parser/semantic migration.

This module is the stable boundary for the compiler frontend.  It currently
wraps the Python lexer/parser/semantic pipeline, but the goal is to replace
those internals with Norscode implementations while keeping this API stable.
"""

from __future__ import annotations

from dataclasses import dataclass
from pathlib import Path
from typing import Any

from compiler.lexer import Lexer

from norcode.module_service import load_module_graph
from norcode.parser_service import parse_text
from norcode.semantic_service import SemanticResult, analyze_program


@dataclass(frozen=True)
class TokenInfo:
    type: str
    value: Any
    line: int
    column: int


@dataclass(frozen=True)
class FrontendResult:
    source_path: Path
    program: Any
    alias_map: dict[str, str]
    semantic: SemanticResult



def lex_source_text(source: str) -> list[TokenInfo]:
    lexer = Lexer(source)
    tokens: list[TokenInfo] = []
    while True:
        token = lexer.next_token()
        tokens.append(TokenInfo(token.typ, token.value, token.line, token.column))
        if token.typ == "EOF":
            break
    return tokens



def lex_source_file(path: str) -> list[TokenInfo]:
    source_path = Path(path).expanduser().resolve()
    return lex_source_text(source_path.read_text(encoding="utf-8"))



def parse_source_text(source: str) -> Any:
    return parse_text(source).program



def parse_source_file(path: str) -> Any:
    source_path = Path(path).expanduser().resolve()
    return parse_source_text(source_path.read_text(encoding="utf-8"))



def analyze_source_file(path: str) -> FrontendResult:
    module_graph = load_module_graph(path)
    semantic = analyze_program(module_graph.program)
    return FrontendResult(
        source_path=module_graph.source_path,
        program=module_graph.program,
        alias_map=module_graph.alias_map,
        semantic=semantic,
    )
