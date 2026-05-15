"""Bytecode and VM service facade.

This module is the stable boundary between CLI/compiler services and the
current Python bytecode backend.  The long-term self-hosting migration can
replace this implementation with a Norscode VM/runtime while preserving the
same service contract.
"""

from __future__ import annotations

import json
from pathlib import Path
from typing import Any

from compiler.ast_bridge import load_source_as_program, read_ast
from compiler.bytecode_backend import BytecodeVM, compile_program_to_bytecode



def compile_source_file_to_bytecode(path: str, *, alias_map: dict[str, str] | None = None) -> dict[str, Any]:
    program, loaded_alias_map = load_source_as_program(path)
    merged_alias_map = dict(loaded_alias_map or {})
    if alias_map:
        merged_alias_map.update(alias_map)
    return compile_program_to_bytecode(program, alias_map=merged_alias_map)



def compile_ast_file_to_bytecode(path: str, *, alias_map: dict[str, str] | None = None) -> dict[str, Any]:
    program, loaded_alias_map = read_ast(Path(path).expanduser().resolve())
    merged_alias_map = dict(loaded_alias_map or {})
    if alias_map:
        merged_alias_map.update(alias_map)
    return compile_program_to_bytecode(program, alias_map=merged_alias_map)



def write_bytecode(bytecode: dict[str, Any], output: str) -> Path:
    out_path = Path(output).expanduser().resolve()
    out_path.write_text(json.dumps(bytecode, ensure_ascii=False, indent=2) + "\n", encoding="utf-8")
    return out_path



def load_bytecode_file(path: str) -> dict[str, Any]:
    payload = json.loads(Path(path).expanduser().resolve().read_text(encoding="utf-8"))
    if not isinstance(payload, dict):
        raise RuntimeError("Bytecode-filen må inneholde et JSON-objekt")
    return payload



def run_bytecode(bytecode: dict[str, Any], **vm_options: Any) -> Any:
    vm = BytecodeVM(bytecode, **vm_options)
    return vm.run()



def run_bytecode_file(path: str, **vm_options: Any) -> Any:
    return run_bytecode(load_bytecode_file(path), **vm_options)
