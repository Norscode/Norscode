"""VM abstraction layer for Norscode.

The current VM implementation is still the Python `compiler.bytecode_backend.BytecodeVM`.
This module defines the runtime-facing contract so a future VM written in
Norscode can replace the Python implementation behind the same boundary.
"""

from __future__ import annotations

from typing import Any, Protocol


class VMRuntime(Protocol):
    def run(self) -> Any:
        ...

    def get_trace_tail(self) -> list[str]:
        ...

    def dump_expr_probe(self) -> str:
        ...


class PythonBytecodeVMAdapter:
    """Adapter around the current Python bytecode VM."""

    def __init__(self, bytecode: dict[str, Any], **options: Any) -> None:
        from compiler.bytecode_backend import BytecodeVM

        self._vm = BytecodeVM(bytecode, **options)

    def run(self) -> Any:
        return self._vm.run()

    def get_trace_tail(self) -> list[str]:
        if hasattr(self._vm, "get_trace_tail"):
            return list(self._vm.get_trace_tail())
        return []

    def dump_expr_probe(self) -> str:
        if hasattr(self._vm, "dump_expr_probe"):
            return str(self._vm.dump_expr_probe())
        return ""



def create_vm(bytecode: dict[str, Any], **options: Any) -> VMRuntime:
    """Create the active VM implementation.

    This factory is the single switch point for replacing the Python VM with a
    self-hosted Norscode VM later.
    """
    return PythonBytecodeVMAdapter(bytecode, **options)
