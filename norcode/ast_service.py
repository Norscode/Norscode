"""AST service facade for Norscode.

This module is the stable boundary for AST conversion and loading.  It keeps
AST representation details away from CLI, bytecode and runtime layers while the
compiler frontend is migrated toward self-hosting.
"""

from __future__ import annotations

import json
from dataclasses import dataclass
from pathlib import Path
from typing import Any

from compiler.ast_bridge import program_from_data, read_ast


@dataclass(frozen=True)
class AstDocument:
    program: Any
    alias_map: dict[str, str]
    source_path: Path | None = None
    raw: dict[str, Any] | None = None



def load_ast_file(path: str) -> AstDocument:
    source_path = Path(path).expanduser().resolve()
    program, alias_map = read_ast(source_path)
    raw = json.loads(source_path.read_text(encoding="utf-8"))
    return AstDocument(program=program, alias_map=alias_map, source_path=source_path, raw=raw)



def load_ast_payload(payload: dict[str, Any]) -> AstDocument:
    program, alias_map = program_from_data(payload)
    return AstDocument(program=program, alias_map=alias_map, raw=payload)



def write_ast_payload(payload: dict[str, Any], output: str) -> Path:
    out_path = Path(output).expanduser().resolve()
    out_path.write_text(json.dumps(payload, ensure_ascii=False, indent=2) + "\n", encoding="utf-8")
    return out_path
