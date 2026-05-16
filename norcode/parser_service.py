"""Parser service facade for Norscode.

This is the stable parser boundary used during the migration from the Python
parser to a self-hosted Norscode parser.  The service returns a small result
object and keeps parser implementation details out of CLI and runtime layers.
"""

from __future__ import annotations

from dataclasses import dataclass
from pathlib import Path
from typing import Any

from compiler.lexer import Lexer
from compiler.parser import Parser


@dataclass(frozen=True)
class ParseResult:
    source_path: Path | None
    program: Any



def parse_text(source: str) -> ParseResult:
    program = Parser(Lexer(source)).parse()
    return ParseResult(source_path=None, program=program)



def parse_file(path: str) -> ParseResult:
    source_path = Path(path).expanduser().resolve()
    program = Parser(Lexer(source_path.read_text(encoding="utf-8"))).parse()
    return ParseResult(source_path=source_path, program=program)
