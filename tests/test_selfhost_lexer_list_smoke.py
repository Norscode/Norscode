"""Pytest gate for selfhost lexer TOKEN_FORMAT_V1 list-return smoke test."""

from __future__ import annotations

from pathlib import Path

from norcode.runtime_call_service import call_function
from norcode.token_validator import validate_token_stream


SMOKE_FILE = Path("tests/selfhost_lexer_list_smoke.no")



def test_selfhost_lexer_list_smoke_runtime_list_return() -> None:
    result = call_function(str(SMOKE_FILE), "start", [])

    assert result.ok, result.errors
    assert isinstance(result.value, list)

    validation = validate_token_stream(result.value)

    assert validation.ok, validation.errors
