"""Runtime kernel configuration for Norscode.

The runtime kernel is the narrow policy layer between high-level services and
VM implementations.  It decides which VM backend is active and carries runtime
capabilities that are useful during the migration from Python to a self-hosted
Norscode runtime.
"""

from __future__ import annotations

from dataclasses import dataclass
from typing import Any


@dataclass(frozen=True)
class RuntimeKernel:
    name: str = "python-bytecode"
    self_hosted: bool = False
    supports_trace: bool = True
    supports_expr_probe: bool = True
    supports_web_runtime: bool = True

    def describe(self) -> dict[str, Any]:
        return {
            "name": self.name,
            "self_hosted": self.self_hosted,
            "supports_trace": self.supports_trace,
            "supports_expr_probe": self.supports_expr_probe,
            "supports_web_runtime": self.supports_web_runtime,
        }


_ACTIVE_KERNEL = RuntimeKernel()



def active_runtime_kernel() -> RuntimeKernel:
    return _ACTIVE_KERNEL



def runtime_capabilities() -> dict[str, Any]:
    return active_runtime_kernel().describe()
