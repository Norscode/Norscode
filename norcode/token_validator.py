"""Token stream validator for Norscode token format v1.

The validator enforces the stable token contract documented in
`docs/TOKEN_FORMAT_V1.md`.  It is intentionally implementation-neutral so it can
validate output from the Python lexer, selfhost lexer, IDE lexers and future
incremental lexers.
"""

from __future__ import annotations

from dataclasses import dataclass, field
from typing import Any


@dataclass(frozen=True)
class TokenValidationResult:
    ok: bool
    errors: list[str] = field(default_factory=list)



def validate_token_stream(tokens: Any) -> TokenValidationResult:
    errors: list[str] = []

    if not isinstance(tokens, list):
        return TokenValidationResult(False, ["token stream må være en liste"])

    if not tokens:
        errors.append("token stream må ikke være tom")
        return TokenValidationResult(False, errors)

    for index, token in enumerate(tokens):
        _validate_token(token, index, errors)

    last = tokens[-1]
    if not isinstance(last, dict) or last.get("type") != "EOF":
        errors.append("token stream må slutte med EOF-token")

    return TokenValidationResult(ok=not errors, errors=errors)



def _validate_token(token: Any, index: int, errors: list[str]) -> None:
    path = f"tokens[{index}]"
    if not isinstance(token, dict):
        errors.append(f"{path} må være et objekt")
        return

    token_type = token.get("type")
    if not isinstance(token_type, str) or not token_type:
        errors.append(f"{path}.type må være ikke-tom tekst")

    if "value" not in token:
        errors.append(f"{path}.value mangler")
    else:
        value = token.get("value")
        if value is not None and not isinstance(value, (str, int, float, bool)):
            errors.append(f"{path}.value må være tekst, tall, bool eller null")

    line = token.get("line")
    if not isinstance(line, int) or line < 1:
        errors.append(f"{path}.line må være heltall >= 1")

    column = token.get("column")
    if not isinstance(column, int) or column < 1:
        errors.append(f"{path}.column må være heltall >= 1")
