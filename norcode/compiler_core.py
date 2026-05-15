"""Compiler core orchestration for Norscode.

This module is the new high-level compiler pipeline boundary.  It coordinates
module loading, semantic analysis and bytecode generation without exposing the
legacy `main.py` orchestration layer to CLI commands.

Current implementation still relies on Python compiler internals underneath the
service facades.  The migration goal is to keep this API stable while replacing
those internals with self-hosted Norscode components.
"""

from __future__ import annotations

from dataclasses import dataclass
from pathlib import Path
from typing import Any

from compiler.bytecode_backend import compile_program_to_bytecode

from norcode.frontend_service import FrontendResult, analyze_source_file
from norcode.runtime_service import RuntimeOptions, run_compiled_bytecode


@dataclass(frozen=True)
class CompileResult:
    source_path: Path
    frontend: FrontendResult
    bytecode: dict[str, Any]



def compile_source(path: str) -> CompileResult:
    frontend = analyze_source_file(path)
    bytecode = compile_program_to_bytecode(frontend.program, alias_map=frontend.alias_map)
    return CompileResult(
        source_path=frontend.source_path,
        frontend=frontend,
        bytecode=bytecode,
    )



def run_source(path: str, options: RuntimeOptions | None = None) -> Any:
    result = compile_source(path)
    return run_compiled_bytecode(result.bytecode, options=options)
