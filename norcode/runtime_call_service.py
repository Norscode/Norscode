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
    available_functions: list[str] = field(default_factory=list)



def _bytecode_function_names(bytecode: dict[str, Any]) -> list[str]:
    functions = bytecode.get("functions", {})
    if not isinstance(functions, dict):
        return []
    return sorted(str(name) for name in functions.keys())



def _candidate_function_names(function_name: str, available_functions: list[str]) -> list[str]:
    candidates: list[str] = []

    def add(name: str) -> None:
        if name and name not in candidates:
            candidates.append(name)

    if "." in function_name:
        add(function_name)
    else:
        add(function_name)
        add(f"__main__.{function_name}")
        for name in available_functions:
            if name.endswith(f".{function_name}"):
                add(name)

    for name in available_functions:
        if name == function_name:
            add(name)

    return candidates



def call_function(source_file: str, function_name: str, args: list[Any] | None = None) -> RuntimeCallResult:
    """Compile a Norscode file and call a named function through the active VM."""
    source_path = Path(source_file).expanduser().resolve()
    call_args = list(args or [])
    available_functions: list[str] = []

    try:
        compile_result = compile_source(str(source_path))
        available_functions = _bytecode_function_names(compile_result.bytecode)
        vm = create_vm(compile_result.bytecode)
    except Exception as exc:
        return RuntimeCallResult(
            ok=False,
            source_path=source_path,
            function_name=function_name,
            args=call_args,
            errors=[f"compile/runtime setup failed: {exc}"],
            available_functions=available_functions,
        )

    errors: list[str] = []
    candidates = _candidate_function_names(function_name, available_functions)
    for candidate in candidates:
        try:
            value = vm.call_function(candidate, call_args)
            return RuntimeCallResult(
                ok=True,
                source_path=source_path,
                function_name=candidate,
                args=call_args,
                value=value,
                available_functions=available_functions,
            )
        except Exception as exc:
            errors.append(f"{candidate}: {exc}")

    return RuntimeCallResult(
        ok=False,
        source_path=source_path,
        function_name=function_name,
        args=call_args,
        errors=errors or ["function call failed"],
        available_functions=available_functions,
    )
