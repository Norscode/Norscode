"""Shared bootstrap and entrypoint support helpers."""

from __future__ import annotations

import subprocess
import sys
from pathlib import Path

from compiler.lexer import Lexer

_LEGACY_WARNINGS_EMITTED: set[str] = set()


def _warn_legacy_once(key: str, message: str) -> None:
    if key in _LEGACY_WARNINGS_EMITTED:
        return
    _LEGACY_WARNINGS_EMITTED.add(key)
    print(message, file=sys.stderr)


def _repo_root() -> Path:
    return Path(__file__).resolve().parent.parent


def _format_cli_exception(exc: BaseException) -> str:
    parts: list[str] = []
    seen: set[int] = set()
    current: BaseException | None = exc
    while current is not None and id(current) not in seen:
        seen.add(id(current))
        label = type(current).__name__
        message = str(current)
        stack = getattr(current, "call_stack", None)
        parts.append(f"{label}: {message}")
        if stack:
            stack_text = " -> ".join(reversed([str(item) for item in stack if item]))
            if stack_text:
                parts.append(f"Kallstakk: {stack_text}")
        current = current.__cause__ or current.__context__
        if current is not None and id(current) not in seen:
            parts.append("Årsak:")
    return "\n".join(parts) if parts else str(exc)


def _run_checked_command(args: list[str], cwd: Path | None = None) -> subprocess.CompletedProcess[str]:
    return subprocess.run(
        args,
        cwd=str(cwd or _repo_root()),
        check=True,
        text=True,
        capture_output=True,
    )


def _find_existing_project_config_in_dir(base: Path, project_config_names: tuple[str, ...]) -> Path | None:
    for name in project_config_names:
        candidate = base / name
        if candidate.exists():
            return candidate
    return None


def _project_config_display_names(project_config_names: tuple[str, ...]) -> str:
    return " / ".join(project_config_names)


def _find_project_config(
    start_dir: Path | None,
    project_config_names: tuple[str, ...],
    legacy_project_config_name: str,
) -> Path:
    base = (start_dir or Path.cwd()).resolve()
    for candidate_dir in (base, *base.parents):
        candidate = _find_existing_project_config_in_dir(candidate_dir, project_config_names)
        if candidate is not None:
            if candidate.name == legacy_project_config_name:
                _warn_legacy_once(
                    "legacy-config",
                    f"Merk: bruker legacy konfig '{legacy_project_config_name}'. Bytt til '{project_config_names[0]}'.",
                )
            return candidate
    raise RuntimeError(
        f"Fant ikke {_project_config_display_names(project_config_names)} i denne mappen eller overliggende mapper"
    )


def _resolve_source_path(source_file: str) -> Path:
    path = Path(source_file).expanduser().resolve()
    if not path.exists():
        raise RuntimeError(f"Fant ikke kildefil: {path}")
    return path


def lex_source(source_text: str):
    lexer = Lexer(source_text)
    tokens = []
    while True:
        tok = lexer.next_token()
        tokens.append(tok)
        if tok.typ == "EOF":
            break
    return tokens


def _ast_to_data(value):
    if value is None or isinstance(value, (str, int, float, bool)):
        return value
    if isinstance(value, list):
        return [_ast_to_data(item) for item in value]
    if hasattr(value, "__dict__"):
        data = {"node": value.__class__.__name__}
        for key, item in value.__dict__.items():
            data[key] = _ast_to_data(item)
        return data
    return repr(value)
