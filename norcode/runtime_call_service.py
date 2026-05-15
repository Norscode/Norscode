"""Runtime call facade for selfhost components.

This module defines the stable ABI boundary for invoking functions inside
Norscode programs through the runtime/VM.

Initially this is a placeholder facade used to stabilize the architecture
before the VM function-call ABI is fully implemented.
"""

from __future__ import annotations

from dataclasses import dataclass, field
from pathlib import Path
from typing import Any


@dataclass(frozen=True)
class RuntimeCallResult:
    ok: bool
    source_path: Path
    function_name: str
    args: list[Any]
    value: Any = None
    errors: list[str] = field(default_factory=list)



def call_function(source_file: str, function_name: str, args: list[Any] | None = None) -> RuntimeCallResult:
    """Future VM ABI entrypoint.

    Planned flow:

    source.no
        ↓
    compiler core
        ↓
    bytecode
        ↓
    runtime/VM
        ↓
    call function
        ↓
    structured return value
    """
    source_path = Path(source_file).expanduser().resolve()

    return RuntimeCallResult(
        ok=False,
        source_path=source_path,
        function_name=function_name,
        args=list(args or []),
        errors=["VM function-call ABI is not implemented yet"],
    )
