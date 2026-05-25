"""Semantic analysis service facade for Norscode.

This layer isolates semantic analysis from parser/frontend orchestration so the
semantic engine can later be replaced independently during self-hosting.
"""

from __future__ import annotations

from dataclasses import dataclass
from typing import Any

from compiler.semantic import SemanticAnalyzer


@dataclass(frozen=True)
class SemanticResult:
    analyzer: SemanticAnalyzer
    function_names: list[str]



def analyze_program(program: Any, alias_map: dict[str, str] | None = None) -> SemanticResult:
    analyzer = SemanticAnalyzer(alias_map=alias_map)
    analyzer.analyze(program)
    return SemanticResult(
        analyzer=analyzer,
        function_names=sorted(list(analyzer.functions.keys())),
    )
