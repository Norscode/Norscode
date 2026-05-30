# AVVIKLA: compiler/selfhost_whole_compile.py er erstatta av nc-vm + tools/nc_regen_bootstrap.sh
# Behalde for --legacy-python-fallback-overgang.

from __future__ import annotations

import json
import hashlib
from dataclasses import dataclass
from pathlib import Path
from typing import Any

from compiler.ast_bridge import program_from_data
from compiler.selfhost_chain import build_selfhost_ast_bundle
from norcode.bytecode_service import write_bytecode
from norcode.bytecode_service import compile_program_to_bytecode


DEFAULT_ROOTS = ("selfhost", "compiler", "std")


@dataclass(frozen=True)
class WholeCompileOptions:
    roots: tuple[str, ...] = DEFAULT_ROOTS
    output_dir: str = "build/selfhost-whole"
    fail_fast: bool = False


def discover_no_files(project_root: Path, roots: tuple[str, ...]) -> list[Path]:
    files: list[Path] = []
    for root in roots:
        base = (project_root / root).resolve()
        if base.is_file() and base.suffix == ".no":
            files.append(base)
            continue
        if not base.exists():
            continue
        files.extend(path.resolve() for path in base.rglob("*.no") if path.is_file())
    return sorted(set(files), key=lambda path: _display_path(project_root, path))


def _display_path(project_root: Path, path: Path) -> str:
    try:
        return str(path.relative_to(project_root))
    except ValueError:
        return str(path)


def _artifact_path(output_dir: Path, project_root: Path, source: Path, suffix: str) -> Path:
    try:
        rel = source.relative_to(project_root).with_suffix(suffix)
    except ValueError:
        rel = Path(source.name).with_suffix(suffix)
    return output_dir / rel


def compile_one(project_root: Path, output_dir: Path, source: Path) -> dict[str, Any]:
    rel = _display_path(project_root, source)
    ast_path = _artifact_path(output_dir, project_root, source, ".shast.json")
    bytecode_path = _artifact_path(output_dir, project_root, source, ".ncb.json")
    ast_path.parent.mkdir(parents=True, exist_ok=True)
    bytecode_path.parent.mkdir(parents=True, exist_ok=True)

    _source_path, bundle = build_selfhost_ast_bundle(str(source))
    ast_path.write_text(json.dumps(bundle, ensure_ascii=False, indent=2) + "\n", encoding="utf-8")

    program, alias_map = program_from_data(bundle)
    bytecode = compile_program_to_bytecode(program, alias_map=alias_map)
    write_bytecode(bytecode, str(bytecode_path))

    return {
        "file": rel,
        "ok": True,
        "ast": str(ast_path),
        "bytecode": str(bytecode_path),
        "functions": len(bytecode.get("functions", {})),
        "imports": len(bundle.get("imports", [])),
    }


def _artifact_digest(output_dir: Path, results: list[dict[str, Any]]) -> dict[str, Any]:
    entries: list[dict[str, str]] = []
    for row in results:
        if not row.get("ok"):
            continue
        for key in ("ast", "bytecode"):
            path = Path(str(row[key]))
            try:
                rel = str(path.relative_to(output_dir))
            except ValueError:
                rel = str(path)
            file_digest = hashlib.sha256(path.read_bytes()).hexdigest()
            entries.append({"path": rel, "sha256": file_digest})

    entries.sort(key=lambda item: item["path"])
    aggregate = hashlib.sha256()
    for entry in entries:
        aggregate.update(entry["path"].encode("utf-8"))
        aggregate.update(b"\0")
        aggregate.update(entry["sha256"].encode("ascii"))
        aggregate.update(b"\n")

    return {
        "algorithm": "sha256",
        "artifact_count": len(entries),
        "sha256": aggregate.hexdigest(),
        "entries": entries,
    }


def compile_whole_norscode(options: WholeCompileOptions | None = None) -> dict[str, Any]:
    options = options or WholeCompileOptions()
    project_root = Path.cwd().resolve()
    output_dir = (project_root / options.output_dir).resolve()
    output_dir.mkdir(parents=True, exist_ok=True)

    files = discover_no_files(project_root, options.roots)
    results: list[dict[str, Any]] = []
    passed = 0

    for source in files:
        try:
            row = compile_one(project_root, output_dir, source)
            passed += 1
        except Exception as exc:
            row = {
                "file": _display_path(project_root, source),
                "ok": False,
                "error": str(exc),
            }
            if options.fail_fast:
                results.append(row)
                break
        results.append(row)

    manifest = {
        "format": "norscode-selfhost-whole-compile-v1",
        "roots": list(options.roots),
        "output_dir": str(output_dir),
        "total": len(files),
        "passed": passed,
        "failed": len(results) - passed,
        "ok": passed == len(files),
        "digest": _artifact_digest(output_dir, results),
        "results": results,
    }
    manifest_path = output_dir / "manifest.json"
    manifest_path.write_text(json.dumps(manifest, ensure_ascii=False, indent=2, sort_keys=True) + "\n", encoding="utf-8")
    manifest["manifest"] = str(manifest_path)
    return manifest
