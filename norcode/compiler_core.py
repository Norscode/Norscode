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

# LAZY IMPORT: from compiler.bytecode_backend import compile_program_to_bytecode

from norcode.bytecode_service import write_bytecode
from norcode.frontend_service import FrontendResult, analyze_source_file
from norcode.runtime_service import RuntimeOptions, run_compiled_bytecode


@dataclass(frozen=True)
class CompileResult:
    source_path: Path
    frontend: FrontendResult
    bytecode: dict[str, Any]


@dataclass(frozen=True)
class BuildResult:
    source_path: Path
    output_path: Path
    compile_result: CompileResult



def default_bytecode_output_path(source_path: Path) -> Path:
    return source_path.with_suffix(".ncb.json")



def compile_source(path: str) -> CompileResult:
    frontend = analyze_source_file(path)
    bytecode = compile_program_to_bytecode(frontend.program, alias_map=frontend.alias_map)
    return CompileResult(
        source_path=frontend.source_path,
        frontend=frontend,
        bytecode=bytecode,
    )



def build_bytecode_file(path: str, output: str | None = None) -> BuildResult:
    compile_result = compile_source(path)
    output_path = Path(output).expanduser().resolve() if output else default_bytecode_output_path(compile_result.source_path)
    written_path = write_bytecode(compile_result.bytecode, str(output_path))
    return BuildResult(
        source_path=compile_result.source_path,
        output_path=written_path,
        compile_result=compile_result,
    )



def run_source(path: str, options: RuntimeOptions | None = None) -> Any:
    result = compile_source(path)
    return run_compiled_bytecode(result.bytecode, options=options)
