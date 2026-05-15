"""Module loading service facade for Norscode.

This module isolates import/module loading from the compiler frontend.  Today it
wraps the Python ModuleLoader.  During self-hosting it becomes the replacement
point for a Norscode-native module resolver and package loader.
"""

from __future__ import annotations

from dataclasses import dataclass
from pathlib import Path
from typing import Any

from compiler.loader import ModuleLoader


@dataclass(frozen=True)
class ModuleLoadResult:
    source_path: Path
    program: Any
    alias_map: dict[str, str]



def load_module_graph(path: str) -> ModuleLoadResult:
    source_path = Path(path).expanduser().resolve()
    loader = ModuleLoader(source_path.parent)
    program, alias_map = loader.load_entry_file(source_path.name)
    return ModuleLoadResult(
        source_path=source_path,
        program=program,
        alias_map=alias_map,
    )
