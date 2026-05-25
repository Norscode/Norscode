"""Interactive REPL helpers for Norscode."""

from __future__ import annotations

import subprocess
import sys
import uuid
from pathlib import Path

from compiler.lexer import Lexer
from compiler.parser import Parser

from norcode.compiler_service import build_program


def _try_parse_expression(source_text: str):
    parser = Parser(Lexer(source_text))
    try:
        expr = parser.expr()
    except Exception:
        return None
    if parser.current.typ != "EOF":
        return None
    return expr


def _indent_repl_body(lines: list[str]) -> list[str]:
    indented: list[str] = []
    for line in lines:
        if line.strip():
            indented.append(f"    {line}")
        else:
            indented.append("")
    return indented


def _build_repl_source(imports: list[str], chunk: str) -> tuple[str, bool]:
    expr = _try_parse_expression(chunk)
    if expr is not None:
        body_lines = [f"    skriv({chunk.strip()})", "    returner 0"]
        is_expr = True
    else:
        body_lines = _indent_repl_body(chunk.splitlines())
        body_lines.append("    returner 0")
        is_expr = False

    source_lines = []
    source_lines.extend(imports)
    if imports:
        source_lines.append("")
    source_lines.append("funksjon start() -> heltall {")
    source_lines.extend(body_lines)
    source_lines.append("}")
    source_lines.append("")
    return "\n".join(source_lines), is_expr


def _run_repl_source(source_text: str):
    suffix = uuid.uuid4().hex[:8]
    source_path = Path.cwd() / f".norscode_repl_{suffix}.no"
    c_path = source_path.with_suffix(".c")
    exe_path = source_path.with_suffix("")
    try:
        source_path.write_text(source_text, encoding="utf-8")
        _source_path, c_path, exe_path, _alias_map, _analyzer = build_program(str(source_path))
        result = subprocess.run(
            [str(exe_path.resolve())],
            capture_output=True,
            text=True,
        )
        return {
            "source": str(source_path),
            "c_file": str(c_path),
            "exe_file": str(exe_path),
            "returncode": result.returncode,
            "stdout": result.stdout,
            "stderr": result.stderr,
        }
    finally:
        for path in (source_path, c_path, exe_path):
            try:
                path.unlink()
            except FileNotFoundError:
                pass


def run_repl():
    print("Norscode REPL")
    print("Skriv linjer og avslutt blokken med en tom linje.")
    print("Kommandoer: :quit, :exit, :reset, :imports")
    imports: list[str] = []
    buffer: list[str] = []

    while True:
        prompt = ">>> " if not buffer else "... "
        try:
            line = input(prompt)
        except EOFError:
            print()
            break

        stripped = line.strip()
        if not buffer and stripped in {":quit", ":exit", ":q"}:
            break
        if not buffer and stripped == ":reset":
            imports.clear()
            print("Session nullstilt.")
            continue
        if not buffer and stripped == ":imports":
            if not imports:
                print("Ingen importlinjer enda.")
            else:
                for item in imports:
                    print(item)
            continue

        if stripped == "":
            if not buffer:
                continue
            chunk = "\n".join(buffer).rstrip()
            buffer.clear()

            if not chunk:
                continue
            if chunk.lstrip().startswith("bruk "):
                imports.append(chunk)
                print("Import lagt til.")
                continue

            source_text, is_expr = _build_repl_source(imports, chunk)
            try:
                result = _run_repl_source(source_text)
            except Exception as exc:
                print(f"Feil: {exc}")
                continue

            stdout = result["stdout"]
            stderr = result["stderr"]
            if stdout:
                end = "" if stdout.endswith("\n") else "\n"
                print(stdout, end=end)
            if stderr:
                end = "" if stderr.endswith("\n") else "\n"
                print(stderr, end=end, file=sys.stderr)
            if result["returncode"] != 0:
                print(f"[exit {result['returncode']}]")
            elif is_expr and not stdout:
                print("OK")
            continue

        buffer.append(line)
