"""Runtime call facade for selfhost components.

This module defines the stable ABI boundary for invoking functions inside
Norscode programs through the runtime/VM.
"""

from __future__ import annotations

from dataclasses import dataclass, field
from pathlib import Path
from typing import Any

from norcode.compiler_core import compile_source
from norcode.vm import create_vm


@dataclass(frozen=True)
class RuntimeCallResult:
    ok: bool
    source_path: Path
    function_name: str
    args: list[Any]
    value: Any = None
    errors: list[str] = field(default_factory=list)



def _candidate_function_names(function_name: str) -> list[str]:
    if "." in function_name:
        return [function_name]
    return [function_name, f"__main__.{function_name}"]



def call_function(source_file: str, function_name: str, args: list[Any] | None = None) -> RuntimeCallResult:
    """Compile a Norscode file and call a named function through the active VM."""
    source_path = Path(source_file).expanduser().resolve()
    call_args = list(args or [])

    try:
        compile_result = compile_source(str(source_path))
        vm = create_vm(compile_result.bytecode)
    except Exception as exc:
        return RuntimeCallResult(
            ok=False,
            source_path=source_path,
            function_name=function_name,
            args=call_args,
            errors=[f"compile/runtime setup failed: {exc}"],
        )

    errors: list[str] = []
    for candidate in _candidate_function_names(function_name):
        try:
            value = vm.call_function(candidate, call_args)
            return RuntimeCallResult(
                ok=True,
                source_path=source_path,
                function_name=candidate,
                args=call_args,
                value=value,
            )
        except Exception as exc:
            errors.append(f"{candidate}: {exc}")

    return RuntimeCallResult(
        ok=False,
        source_path=source_path,
        function_name=function_name,
        args=call_args,
        errors=errors or ["function call failed"],
    )
