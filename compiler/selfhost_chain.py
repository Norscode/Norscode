
from __future__ import annotations

import json
import tomllib
from pathlib import Path
from typing import Any

from .bytecode_backend import compile_program_to_bytecode, BytecodeVM
from .ast_bridge import program_from_data
from .selfhost_ast_bridge import program_payload_to_ast
from .selfhost_parser import parse_selfhost_program


class SelfhostChainError(RuntimeError):
    pass


def _find_project_root(start: Path) -> Path:
    for base in (start, *start.parents):
        if (base / 'norcode.toml').exists() or (base / 'pyproject.toml').exists():
            return base
    return start.parent


def _load_package_entry(package_dir: Path) -> str | None:
    for cfg_name in ('norcode.toml', 'norsklang.toml'):
        cfg = package_dir / cfg_name
        if cfg.exists():
            data = tomllib.loads(cfg.read_text(encoding='utf-8'))
            project = data.get('project', {}) if isinstance(data, dict) else {}
            entry = project.get('entry')
            if isinstance(entry, str) and entry.strip():
                return entry.strip()
    return None


def resolve_module_file(module_name: str, source_path: Path) -> Path:
    current_dir = source_path.parent.resolve()
    project_root = _find_project_root(current_dir)
    dot_path = Path(*module_name.split('.'))
    candidates = [
        current_dir / f'{module_name}.no',
        current_dir / dot_path.with_suffix('.no'),
        project_root / f'{module_name}.no',
        project_root / dot_path.with_suffix('.no'),
        project_root / 'tests' / f'{module_name}.no',
        project_root / 'tests' / dot_path.with_suffix('.no'),
        project_root / 'std' / dot_path.with_suffix('.no'),
    ]
    pkg_dir = project_root / 'packages' / module_name
    entry = _load_package_entry(pkg_dir)
    if entry:
        candidates.append(pkg_dir / entry)
    candidates.append(pkg_dir / 'main.no')
    candidates.append(pkg_dir / f'{module_name}.no')

    for cand in candidates:
        if cand.exists() and cand.is_file():
            return cand.resolve()
    raise SelfhostChainError(f'Fant ikke modulfil for import: {module_name}')


def _ast_from_payload(payload: dict[str, Any], module_name: str) -> dict[str, Any]:
    ast = program_payload_to_ast(payload)
    for fn in ast.get('functions', []):
        fn['module_name'] = module_name
    return ast


def build_selfhost_ast_bundle(source_file: str) -> tuple[Path, dict[str, Any]]:
    source_path = Path(source_file).expanduser().resolve()
    if not source_path.exists():
        raise SelfhostChainError(f'Fant ikke kildefil: {source_path}')

    visited: set[Path] = set()
    alias_map: dict[str, str] = {}
    imports_out: list[dict[str, Any]] = []
    functions_out: list[dict[str, Any]] = []

    def visit(path: Path, module_name: str):
        if path in visited:
            return
        visited.add(path)
        payload = parse_selfhost_program(path.read_text(encoding='utf-8'))
        ast = _ast_from_payload(payload, module_name)
        for item in ast.get('imports', []):
            imported_mod = item.get('module_name')
            alias = item.get('alias') or str(imported_mod).split('.')[-1]
            if imported_mod:
                alias_map[alias] = imported_mod
                if module_name == '__main__':
                    imports_out.append({'module_name': imported_mod, 'alias': item.get('alias')})
                mod_path = resolve_module_file(str(imported_mod), path)
                visit(mod_path, str(imported_mod))
        functions_out.extend(ast.get('functions', []))

    visit(source_path, '__main__')
    bundle = {
        'format': 'norscode-ast-v1',
        'alias_map': alias_map,
        'imports': imports_out,
        'functions': functions_out,
    }
    return source_path, bundle


def export_selfhost_ast_bundle(source_file: str, output: str | None = None) -> Path:
    source_path, bundle = build_selfhost_ast_bundle(source_file)
    out_path = Path(output).expanduser().resolve() if output else source_path.with_suffix('.chain.shast.json')
    out_path.write_text(json.dumps(bundle, ensure_ascii=False, indent=2) + '\n', encoding='utf-8')
    return out_path


def run_chain(
    source_file: str,
    trace: bool = False,
    max_steps: int = 200000,
    trace_focus: str | None = None,
    repeat_limit: int = 0,
    expr_probe: str | None = None,
    expr_probe_log: str | None = None,
) -> Any:
    _source_path, bundle = build_selfhost_ast_bundle(source_file)
    program, alias_map = program_from_data(bundle)
    bytecode = compile_program_to_bytecode(program, alias_map=alias_map)
    vm = BytecodeVM(
        bytecode,
        trace=trace,
        max_steps=max_steps,
        trace_focus=trace_focus,
        repeat_limit=repeat_limit,
        expr_probe=expr_probe,
        expr_probe_log=expr_probe_log,
    )
    try:
        return vm.run()
    except Exception as exc:
        tail = vm.get_trace_tail() if hasattr(vm, "get_trace_tail") else []
        probe = vm.dump_expr_probe() if hasattr(vm, "dump_expr_probe") else ""
        parts = [str(exc)]
        if trace and tail:
            parts.append("TRACE TAIL:\n" + "\n".join(tail))
        if probe:
            parts.append("EXPR PROBE:\n" + probe.rstrip())
        raise SelfhostChainError("\n".join(parts)) from exc


def _default_chain_cases(project_root: Path) -> list[str]:
    return [
        str(project_root / 'tests' / 'test_if.no'),
        str(project_root / 'tests' / 'test_math.no'),
        str(project_root / 'tests' / 'test_text.no'),
        str(project_root / 'tests' / 'test_dependency_import.no'),
        str(project_root / 'tests' / 'test_assert.no'),
        str(project_root / 'tests' / 'test_assert_eq.no'),
        str(project_root / 'tests' / 'test_for.no'),
        str(project_root / 'tests' / 'test_while.no'),
        str(project_root / 'tests' / 'test_elif.no'),
        str(project_root / 'tests' / 'test_selfhost_ifexpr_v21.no'),
        str(project_root / 'tests' / 'test_selfhost_indexset_v22.no'),
        str(project_root / 'tests' / 'test_empty_string_list.no'),
    ]


def check_chain(
    files: list[str] | None = None,
    trace: bool = False,
    max_steps: int = 200000,
    trace_focus: str | None = None,
    repeat_limit: int = 0,
    expr_probe: str | None = None,
    expr_probe_log: str | None = None,
) -> dict[str, Any]:
    root = _find_project_root(Path.cwd())
    targets = files or _default_chain_cases(root)
    results: list[dict[str, Any]] = []
    ok = 0
    for item in targets:
        try:
            result = run_chain(
                item,
                trace=trace,
                max_steps=max_steps,
                trace_focus=trace_focus,
                repeat_limit=repeat_limit,
                expr_probe=expr_probe,
                expr_probe_log=expr_probe_log,
            )
            ok += 1
            results.append({'file': str(Path(item).resolve()), 'ok': True, 'result': result})
        except Exception as exc:
            results.append({'file': str(Path(item).resolve()), 'ok': False, 'error': str(exc)})
    return {'ok': ok == len(targets), 'passed': ok, 'total': len(targets), 'results': results}
