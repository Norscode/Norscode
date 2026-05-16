"""Runtime service facade for Norscode.

This module is the public runtime boundary for the migration away from the
legacy Python monolith.  Today it delegates to the Python bytecode service.
Later it can delegate to a self-hosted Norscode VM/runtime without changing the
CLI or compiler service APIs.
"""

from __future__ import annotations

from dataclasses import dataclass, field
from typing import Any

from norcode.bytecode_service import run_bytecode, run_bytecode_file


@dataclass(frozen=True)
class RuntimeOptions:
    trace: bool = False
    max_steps: int = 5_000_000
    trace_focus: str | None = None
    repeat_limit: int = 0
    expr_probe: str | None = None
    expr_probe_log: str | None = None
    extra: dict[str, Any] = field(default_factory=dict)

    def to_vm_options(self) -> dict[str, Any]:
        options: dict[str, Any] = {
            "trace": self.trace,
            "max_steps": self.max_steps,
            "trace_focus": self.trace_focus,
            "repeat_limit": self.repeat_limit,
            "expr_probe": self.expr_probe,
            "expr_probe_log": self.expr_probe_log,
        }
        options.update(self.extra)
        return options



def run_compiled_bytecode(bytecode: dict[str, Any], options: RuntimeOptions | None = None) -> Any:
    runtime_options = options or RuntimeOptions()
    return run_bytecode(bytecode, **runtime_options.to_vm_options())



def run_compiled_bytecode_file(path: str, options: RuntimeOptions | None = None) -> Any:
    runtime_options = options or RuntimeOptions()
    return run_bytecode_file(path, **runtime_options.to_vm_options())
