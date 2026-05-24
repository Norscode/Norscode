"""AST format validator for Norscode AST v1.

The validator is intentionally lightweight.  It verifies the stable structural
contract documented in `docs/AST_FORMAT_V1.md` without tying callers to the
legacy Python AST classes.
"""

from __future__ import annotations

from dataclasses import dataclass, field
from typing import Any

AST_FORMAT_V1 = "norscode-ast-v1"


@dataclass(frozen=True)
class AstValidationResult:
    ok: bool
    errors: list[str] = field(default_factory=list)



def _is_object(value: Any) -> bool:
    return isinstance(value, dict)



def _is_list(value: Any) -> bool:
    return isinstance(value, list)



def validate_ast_payload(payload: dict[str, Any]) -> AstValidationResult:
    errors: list[str] = []

    if not _is_object(payload):
        return AstValidationResult(False, ["AST payload må være et objekt"])

    if payload.get("format") != AST_FORMAT_V1:
        errors.append(f"format må være {AST_FORMAT_V1!r}")

    alias_map = payload.get("alias_map", {})
    if not _is_object(alias_map):
        errors.append("alias_map må være et objekt")

    imports = payload.get("imports", [])
    if not _is_list(imports):
        errors.append("imports må være en liste")
    else:
        for index, item in enumerate(imports):
            if not _is_object(item):
                errors.append(f"imports[{index}] må være et objekt")
                continue
            if not isinstance(item.get("module_name"), str):
                errors.append(f"imports[{index}].module_name må være tekst")
            alias = item.get("alias")
            if alias is not None and not isinstance(alias, str):
                errors.append(f"imports[{index}].alias må være tekst eller null")

    functions = payload.get("functions", [])
    if not _is_list(functions):
        errors.append("functions må være en liste")
    else:
        for index, fn in enumerate(functions):
            _validate_function(fn, f"functions[{index}]", errors)

    return AstValidationResult(ok=not errors, errors=errors)



def _validate_function(fn: Any, path: str, errors: list[str]) -> None:
    if not _is_object(fn):
        errors.append(f"{path} må være et objekt")
        return
    if fn.get("type") not in (None, "function"):
        errors.append(f"{path}.type må være 'function' hvis feltet finnes")
    if not isinstance(fn.get("name"), str):
        errors.append(f"{path}.name må være tekst")
    module_name = fn.get("module_name", "__main__")
    if module_name is not None and not isinstance(module_name, str):
        errors.append(f"{path}.module_name må være tekst eller null")
    if not _is_list(fn.get("params", [])):
        errors.append(f"{path}.params må være en liste")
    else:
        for index, param in enumerate(fn.get("params", [])):
            if not _is_object(param):
                errors.append(f"{path}.params[{index}] må være et objekt")
                continue
            if not isinstance(param.get("name"), str):
                errors.append(f"{path}.params[{index}].name må være tekst")
            if not isinstance(param.get("type"), str):
                errors.append(f"{path}.params[{index}].type må være tekst")
    if "return_type" in fn and not isinstance(fn.get("return_type"), str):
        errors.append(f"{path}.return_type må være tekst")
    body = fn.get("body")
    if body is not None:
        _validate_block(body, f"{path}.body", errors)



def _validate_block(block: Any, path: str, errors: list[str]) -> None:
    if not _is_object(block):
        errors.append(f"{path} må være et objekt")
        return
    if block.get("type") not in (None, "block"):
        errors.append(f"{path}.type må være 'block' hvis feltet finnes")
    statements = block.get("statements", [])
    if not _is_list(statements):
        errors.append(f"{path}.statements må være en liste")
