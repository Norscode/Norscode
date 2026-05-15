"""Parser parity service for Norscode.

This service is the bridge between the current Python parser and future
self-hosted parser implementations.  It produces stable AST v1 payloads and
validates them so parity checks can be added without coupling to parser internals.
"""

from __future__ import annotations

from dataclasses import dataclass
from pathlib import Path
from typing import Any

from norcode.ast_validator import AstValidationResult, validate_ast_payload
from norcode.parser_service import parse_file


@dataclass(frozen=True)
class ParserParityResult:
    source_path: Path
    payload: dict[str, Any]
    validation: AstValidationResult



def ast_payload_from_current_parser(source_file: str) -> ParserParityResult:
    parse_result = parse_file(source_file)
    program = parse_result.program

    functions: list[dict[str, Any]] = []
    for fn in getattr(program, "functions", []) or []:
        functions.append(
            {
                "type": "function",
                "name": getattr(fn, "name", ""),
                "module_name": getattr(fn, "module_name", None) or "__main__",
                "params": [
                    {"name": getattr(param, "name", ""), "type": str(getattr(param, "type", ""))}
                    for param in getattr(fn, "params", []) or []
                ],
                "return_type": str(getattr(fn, "return_type", "")),
                "body": {"type": "block", "statements": []},
            }
        )

    imports = [
        {"module_name": getattr(item, "module_name", ""), "alias": getattr(item, "alias", None)}
        for item in getattr(program, "imports", []) or []
    ]

    payload = {
        "format": "norscode-ast-v1",
        "alias_map": {},
        "imports": imports,
        "functions": functions,
    }
    validation = validate_ast_payload(payload)
    return ParserParityResult(
        source_path=parse_result.source_path or Path(source_file).expanduser().resolve(),
        payload=payload,
        validation=validation,
    )
