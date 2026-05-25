"""Shared selfhost parity helpers."""

from __future__ import annotations

import difflib
import json
import time
from pathlib import Path

from norcode.ir_tools import (
    SELFHOST_PARSER_EXTENDED_FIXTURE,
    SELFHOST_PARSER_M1_FIXTURE,
    SELFHOST_PARSER_M2_FIXTURE,
    _run_selfhost_parser_disasm_cases,
)


def _load_selfhost_parity_fixture(path: Path, mismatch_lines: list[str]) -> dict | None:
    try:
        return json.loads(path.resolve().read_text(encoding="utf-8"))
    except Exception as exc:
        mismatch_lines.append(f"Kunne ikke lese fixture {path.resolve()}: {exc}")
        return None


def _normalize_selfhost_parity_case(item: dict) -> dict:
    normalized = {"source": str(item.get("source", ""))}
    has_lines = "expected_lines" in item
    has_error = "expected_error" in item
    if has_lines == has_error:
        normalized["invalid"] = True
        return normalized
    if has_error:
        normalized["expected_error"] = str(item.get("expected_error", ""))
    else:
        normalized["expected_lines"] = [str(line) for line in item.get("expected_lines", [])]
    return normalized


def _selfhost_parity_fixture_case_maps(
    fixture: dict,
    fixture_tag: str,
    mismatch_lines: list[str],
) -> tuple[dict[str, dict[str, dict]], int]:
    maps: dict[str, dict[str, dict]] = {"expressions": {}, "scripts": {}}
    checked = 0
    for mode in ("expressions", "scripts"):
        cases = fixture.get(mode, [])
        mode_map = maps[mode]
        for idx, item in enumerate(cases):
            if "name" not in item:
                mismatch_lines.append(f"[{fixture_tag}/{mode}#{idx}] mangler navn")
                continue
            name = str(item["name"])
            if name in mode_map:
                mismatch_lines.append(f"[{fixture_tag}/{mode}#{idx}] duplikat navn: {name}")
                continue
            mode_map[name] = _normalize_selfhost_parity_case(item)
            checked += 1
    return maps, checked


def _count_selfhost_parity_cases(fixture: dict) -> dict:
    expression_cases = fixture.get("expressions", [])
    script_cases = fixture.get("scripts", [])
    all_cases = [*expression_cases, *script_cases]
    error_cases = sum(1 for item in all_cases if "expected_error" in item)
    line_cases = len(all_cases) - error_cases
    return {
        "case_count": len(all_cases),
        "expression_cases": len(expression_cases),
        "script_cases": len(script_cases),
        "line_cases": line_cases,
        "error_cases": error_cases,
    }


def run_selfhost_parser_core_checks(fixture_path: Path, label: str):
    started = time.perf_counter()
    fixture_path = fixture_path.resolve()
    try:
        fixture = json.loads(fixture_path.read_text(encoding="utf-8"))
    except Exception as exc:
        return {
            "source": label,
            "c_file": "",
            "exe_file": "",
            "returncode": 1,
            "stdout": "",
            "stderr": f"Kunne ikke lese fixture {fixture_path}: {exc}\n",
            "success": False,
            "duration_ms": int((time.perf_counter() - started) * 1000),
            "case_count": 0,
            "expression_cases": 0,
            "script_cases": 0,
            "line_cases": 0,
            "error_cases": 0,
        }

    expression_cases = fixture.get("expressions", [])
    script_cases = fixture.get("scripts", [])
    expression_count = len(expression_cases)
    script_count = len(script_cases)
    line_case_count = 0
    error_case_count = 0
    mismatch_lines: list[str] = []

    def _validate_and_collect(cases: list[dict], mode: str):
        nonlocal line_case_count, error_case_count
        if not cases:
            return
        seen_names: set[str] = set()
        for idx, item in enumerate(cases):
            has_expected_lines = "expected_lines" in item
            has_expected_error = "expected_error" in item
            if "source" not in item:
                mismatch_lines.append(f"[{mode}#{idx}] mangler source i fixture")
                return
            case_name = str(item.get("name") or f"{mode}_{idx}")
            if case_name in seen_names:
                mismatch_lines.append(f"[{mode}#{idx}] duplikat casenavn: {case_name}")
                return
            seen_names.add(case_name)
            if has_expected_lines == has_expected_error:
                mismatch_lines.append(
                    f"[{mode}#{idx}] må ha nøyaktig ett av feltene expected_lines eller expected_error"
                )
                return
            if has_expected_error:
                error_case_count += 1
            else:
                line_case_count += 1
        sources = [str(item["source"]) for item in cases]
        actual_lists = _run_selfhost_parser_disasm_cases(sources, mode=mode)
        if len(actual_lists) != len(cases):
            mismatch_lines.append(f"[{mode}] intern feil: antall resultater avviker")
            return
        for idx, item in enumerate(cases):
            name = str(item.get("name") or f"{mode}_{idx}")
            actual_lines = actual_lists[idx]
            expected_error = item.get("expected_error")
            if expected_error is not None:
                actual_error = actual_lines[0] if actual_lines else ""
                if actual_error != str(expected_error):
                    mismatch_lines.append(f"[{name}] error mismatch")
                    mismatch_lines.append(f"expected: {expected_error}")
                    mismatch_lines.append(f"actual:   {actual_error}")
                continue

            expected_lines = [str(line) for line in item.get("expected_lines", [])]
            if expected_lines != actual_lines:
                mismatch_lines.append(f"[{name}] mismatch")
                mismatch_lines.extend(
                    difflib.unified_diff(
                        expected_lines,
                        actual_lines,
                        fromfile=f"{name}/expected",
                        tofile=f"{name}/actual",
                        lineterm="",
                    )
                )

    try:
        _validate_and_collect(expression_cases, "expression")
        _validate_and_collect(script_cases, "script")
    except Exception as exc:
        mismatch_lines.append(f"Kjøring feilet: {exc}")

    success = len(mismatch_lines) == 0
    return {
        "source": label,
        "c_file": "",
        "exe_file": "",
        "returncode": 0 if success else 1,
        "stdout": "",
        "stderr": "" if success else "\n".join(mismatch_lines) + "\n",
        "success": success,
        "duration_ms": int((time.perf_counter() - started) * 1000),
        "case_count": expression_count + script_count,
        "expression_cases": expression_count,
        "script_cases": script_count,
        "line_cases": line_case_count,
        "error_cases": error_case_count,
    }


def run_selfhost_parser_parity(suite: str = "all") -> dict:
    suites = _selfhost_parity_suite_targets(suite)
    started = time.perf_counter()
    results: list[dict] = []
    for fixture_path, label in suites:
        results.append(run_selfhost_parser_core_checks(fixture_path, label))

    ok = all(item.get("success") for item in results)
    return {
        "suite": suite,
        "ok": ok,
        "case_count": sum(int(item.get("case_count", 0) or 0) for item in results),
        "expression_cases": sum(int(item.get("expression_cases", 0) or 0) for item in results),
        "script_cases": sum(int(item.get("script_cases", 0) or 0) for item in results),
        "line_cases": sum(int(item.get("line_cases", 0) or 0) for item in results),
        "error_cases": sum(int(item.get("error_cases", 0) or 0) for item in results),
        "duration_ms": int((time.perf_counter() - started) * 1000),
        "results": results,
    }


def run_selfhost_parity_progress() -> dict:
    started = time.perf_counter()
    mismatch_lines: list[str] = []
    m1_fixture = _load_selfhost_parity_fixture(SELFHOST_PARSER_M1_FIXTURE, mismatch_lines)
    m2_fixture = _load_selfhost_parity_fixture(SELFHOST_PARSER_M2_FIXTURE, mismatch_lines)
    ext_fixture = _load_selfhost_parity_fixture(SELFHOST_PARSER_EXTENDED_FIXTURE, mismatch_lines)
    if m1_fixture is None or m2_fixture is None or ext_fixture is None:
        return {
            "ok": False,
            "duration_ms": int((time.perf_counter() - started) * 1000),
            "stderr": "\n".join(mismatch_lines) + "\n",
        }

    m1_maps, _ = _selfhost_parity_fixture_case_maps(m1_fixture, "m1", mismatch_lines)
    m2_maps, _ = _selfhost_parity_fixture_case_maps(m2_fixture, "m2", mismatch_lines)
    ext_maps, _ = _selfhost_parity_fixture_case_maps(ext_fixture, "extended", mismatch_lines)

    overlap_expr = sorted(set(m1_maps["expressions"].keys()) & set(m2_maps["expressions"].keys()))
    overlap_scripts = sorted(set(m1_maps["scripts"].keys()) & set(m2_maps["scripts"].keys()))
    overlap_total = len(overlap_expr) + len(overlap_scripts)

    union_expr = set(m1_maps["expressions"].keys()) | set(m2_maps["expressions"].keys())
    union_scripts = set(m1_maps["scripts"].keys()) | set(m2_maps["scripts"].keys())
    ext_expr = set(ext_maps["expressions"].keys())
    ext_scripts = set(ext_maps["scripts"].keys())
    union_total = len(union_expr) + len(union_scripts)
    ext_total = len(ext_expr) + len(ext_scripts)

    missing_expr = sorted(ext_expr - union_expr)
    missing_scripts = sorted(ext_scripts - union_scripts)
    missing_total = len(missing_expr) + len(missing_scripts)

    extra_expr = sorted(union_expr - ext_expr)
    extra_scripts = sorted(union_scripts - ext_scripts)
    extra_total = len(extra_expr) + len(extra_scripts)

    consistency = run_selfhost_parser_suite_all_consistency_check(
        SELFHOST_PARSER_M1_FIXTURE,
        SELFHOST_PARSER_M2_FIXTURE,
        SELFHOST_PARSER_EXTENDED_FIXTURE,
    )

    def _pct(part: int, total: int) -> float:
        if total <= 0:
            return 100.0
        return round((part / total) * 100.0, 2)

    coverage_expr = _pct(len(union_expr), len(ext_expr))
    coverage_scripts = _pct(len(union_scripts), len(ext_scripts))
    coverage_total = _pct(union_total, ext_total)
    m1_total = len(m1_maps["expressions"]) + len(m1_maps["scripts"])
    m2_total = len(m2_maps["expressions"]) + len(m2_maps["scripts"])
    coverage_m1 = _pct(m1_total, ext_total)
    coverage_m2 = _pct(m2_total, ext_total)

    ready = (
        consistency.get("success")
        and missing_total == 0
        and extra_total == 0
        and overlap_total == 0
    )
    ok = ready and not mismatch_lines
    return {
        "ok": ok,
        "ready": bool(ready),
        "duration_ms": int((time.perf_counter() - started) * 1000),
        "m1": {
            **_count_selfhost_parity_cases(m1_fixture),
            "coverage_total_pct": coverage_m1,
        },
        "m2": {
            **_count_selfhost_parity_cases(m2_fixture),
            "coverage_total_pct": coverage_m2,
        },
        "extended": _count_selfhost_parity_cases(ext_fixture),
        "coverage": {
            "expression_pct": coverage_expr,
            "script_pct": coverage_scripts,
            "total_pct": coverage_total,
            "union_case_count": union_total,
            "extended_case_count": ext_total,
            "missing_in_m1_m2_count": missing_total,
            "extra_in_m1_m2_count": extra_total,
            "overlap_count": overlap_total,
            "missing_in_m1_m2_examples": (missing_expr + missing_scripts)[:10],
            "extra_in_m1_m2_examples": (extra_expr + extra_scripts)[:10],
            "overlap_examples": (overlap_expr + overlap_scripts)[:10],
        },
        "consistency": {
            "ok": bool(consistency.get("success")),
            "checked_cases": int(consistency.get("checked_cases", 0) or 0),
            "mismatch_count": int(consistency.get("mismatch_count", 0) or 0),
        },
        "stderr": "" if ok else (consistency.get("stderr", "") or "\n".join(mismatch_lines) + "\n"),
    }


def run_selfhost_parity_gate(min_coverage: float | None = None) -> dict:
    progress = run_selfhost_parity_progress()
    coverage_total = None
    if isinstance(progress.get("coverage"), dict):
        coverage_total = progress["coverage"].get("total_pct")

    ready = bool(progress.get("ready"))
    if min_coverage is not None:
        ready = ready and isinstance(coverage_total, (int, float)) and float(coverage_total) >= float(min_coverage)

    return {
        "ok": bool(progress.get("ok")) and ready,
        "ready": ready,
        "min_coverage": min_coverage,
        "coverage_total_pct": coverage_total,
        "progress": progress,
    }


def run_selfhost_parser_suite_subset_consistency_check(
    subset_fixture: Path,
    extended_fixture: Path,
    subset_tag: str,
) -> dict:
    started = time.perf_counter()
    mismatch_lines: list[str] = []

    subset = _load_selfhost_parity_fixture(subset_fixture, mismatch_lines)
    extended = _load_selfhost_parity_fixture(extended_fixture, mismatch_lines)
    if subset is None or extended is None:
        return {
            "success": False,
            "checked_cases": 0,
            "mismatch_count": len(mismatch_lines),
            "stderr": "\n".join(mismatch_lines) + "\n",
            "duration_ms": int((time.perf_counter() - started) * 1000),
            "scope": subset_tag,
        }

    subset_maps, checked = _selfhost_parity_fixture_case_maps(subset, subset_tag, mismatch_lines)
    ext_maps, _ = _selfhost_parity_fixture_case_maps(extended, "extended", mismatch_lines)

    for mode in ("expressions", "scripts"):
        subset_names = set(subset_maps[mode].keys())
        ext_names = set(ext_maps[mode].keys())
        union_names = subset_names

        for name in sorted(union_names):
            subset_case = subset_maps[mode].get(name)
            ext_case = ext_maps[mode].get(name)
            if ext_case is None:
                mismatch_lines.append(f"[{subset_tag}/{mode}/{name}] finnes i subset men ikke i utvidet fixture")
                continue
            if subset_case.get("invalid") or ext_case.get("invalid"):
                mismatch_lines.append(f"[{subset_tag}/{mode}/{name}] ugyldig expected_* format")
                continue
            if subset_case != ext_case:
                mismatch_lines.append(f"[{subset_tag}/{mode}/{name}] avviker mellom subset og utvidet")

        extra_in_extended = ext_names - subset_names
        for name in sorted(extra_in_extended):
            mismatch_lines.append(f"[{subset_tag}/{mode}/{name}] finnes i utvidet fixture, men mangler i subset")

    success = len(mismatch_lines) == 0
    return {
        "success": success,
        "checked_cases": checked,
        "mismatch_count": len(mismatch_lines),
        "stderr": "" if success else "\n".join(mismatch_lines) + "\n",
        "duration_ms": int((time.perf_counter() - started) * 1000),
        "scope": subset_tag,
    }


def run_selfhost_parser_suite_all_consistency_check(
    m1_fixture: Path,
    m2_fixture: Path,
    extended_fixture: Path,
) -> dict:
    started = time.perf_counter()
    mismatch_lines: list[str] = []

    m1 = run_selfhost_parser_suite_subset_consistency_check(m1_fixture, extended_fixture, "m1")
    m2 = run_selfhost_parser_suite_subset_consistency_check(m2_fixture, extended_fixture, "m2")

    m1_fixture_obj = _load_selfhost_parity_fixture(m1_fixture, mismatch_lines)
    m2_fixture_obj = _load_selfhost_parity_fixture(m2_fixture, mismatch_lines)
    ext_fixture_obj = _load_selfhost_parity_fixture(extended_fixture, mismatch_lines)

    coverage_checked = 0
    if m1_fixture_obj is not None and m2_fixture_obj is not None and ext_fixture_obj is not None:
        m1_maps, _ = _selfhost_parity_fixture_case_maps(m1_fixture_obj, "m1", mismatch_lines)
        m2_maps, _ = _selfhost_parity_fixture_case_maps(m2_fixture_obj, "m2", mismatch_lines)
        ext_maps, _ = _selfhost_parity_fixture_case_maps(ext_fixture_obj, "extended", mismatch_lines)

        for mode in ("expressions", "scripts"):
            union_names = set(m1_maps[mode].keys()) | set(m2_maps[mode].keys())
            coverage_checked += len(union_names)
            for name in sorted(union_names):
                union_case = m1_maps[mode].get(name) or m2_maps[mode].get(name)
                ext_case = ext_maps[mode].get(name)
                if ext_case is None:
                    mismatch_lines.append(f"[all/{mode}/{name}] finnes i m1/m2 men ikke i utvidet fixture")
                    continue
                if union_case.get("invalid") or ext_case.get("invalid"):
                    mismatch_lines.append(f"[all/{mode}/{name}] ugyldig expected_* format")
                    continue
                if union_case != ext_case:
                    mismatch_lines.append(f"[all/{mode}/{name}] avviker mellom m1/m2-union og utvidet")

            extra_in_extended = set(ext_maps[mode].keys()) - union_names
            for name in sorted(extra_in_extended):
                mismatch_lines.append(f"[all/{mode}/{name}] finnes i utvidet fixture, men mangler i m1+m2")

    all_success = m1.get("success") and m2.get("success") and not mismatch_lines
    total_mismatch = int(m1.get("mismatch_count", 0) or 0) + int(m2.get("mismatch_count", 0) or 0) + len(mismatch_lines)

    details: list[str] = []
    if not m1.get("success") and m1.get("stderr"):
        details.append("[m1]\n" + str(m1.get("stderr", "")).rstrip())
    if not m2.get("success") and m2.get("stderr"):
        details.append("[m2]\n" + str(m2.get("stderr", "")).rstrip())
    if mismatch_lines:
        details.append("[all]\n" + "\n".join(mismatch_lines))

    return {
        "success": bool(all_success),
        "checked_cases": int(m1.get("checked_cases", 0) or 0)
        + int(m2.get("checked_cases", 0) or 0)
        + coverage_checked,
        "mismatch_count": total_mismatch,
        "stderr": "" if all_success else ("\n\n".join(details).rstrip() + "\n"),
        "duration_ms": int((time.perf_counter() - started) * 1000),
        "scope": "all",
        "checks": {
            "m1": m1,
            "m2": m2,
            "coverage_checked_cases": coverage_checked,
            "coverage_mismatch_count": len(mismatch_lines),
        },
    }


def run_selfhost_parser_suite_consistency_check(m1_fixture: Path, extended_fixture: Path) -> dict:
    return run_selfhost_parser_suite_subset_consistency_check(m1_fixture, extended_fixture, "m1")


def _selfhost_parity_suite_targets(suite: str) -> list[tuple[Path, str]]:
    suites = {
        "m1": [(SELFHOST_PARSER_M1_FIXTURE, "Selfhost parser parity (M1)")],
        "m2": [(SELFHOST_PARSER_M2_FIXTURE, "Selfhost parser parity (M2)")],
        "extended": [(SELFHOST_PARSER_EXTENDED_FIXTURE, "Selfhost parser parity (utvidet)")],
        "all": [
            (SELFHOST_PARSER_M1_FIXTURE, "Selfhost parser parity (M1)"),
            (SELFHOST_PARSER_M2_FIXTURE, "Selfhost parser parity (M2)"),
            (SELFHOST_PARSER_EXTENDED_FIXTURE, "Selfhost parser parity (utvidet)"),
        ],
    }
    if suite not in suites:
        raise RuntimeError(f"Ugyldig suite: {suite}")
    return suites[suite]


def update_selfhost_parser_fixtures(check_only: bool = False, suite: str = "all", sync_m2: bool = True) -> dict:
    targets = _selfhost_parity_suite_targets(suite)
    summaries: list[dict] = []
    total_updated = 0
    total_cases = 0
    m2_sync_payload = None

    if sync_m2 and suite in {"m2", "all"}:
        m2_sync_payload = sync_selfhost_parser_m2_fixture(check_only=check_only)
        total_updated += int(m2_sync_payload.get("updated", 0) or 0)
        total_updated += int(m2_sync_payload.get("missing_m1_from_core_count", 0) or 0)

    for fixture_path, label in targets:
        fixture_abs = fixture_path.resolve()
        fixture = json.loads(fixture_abs.read_text(encoding="utf-8"))
        updated = 0
        case_count = 0

        for mode, key in (("expression", "expressions"), ("script", "scripts")):
            cases = fixture.get(key, [])
            if not cases:
                continue
            sources = [str(item.get("source", "")) for item in cases]
            actual_lists = _run_selfhost_parser_disasm_cases(sources, mode=mode)
            if len(actual_lists) != len(cases):
                raise RuntimeError(f"Intern feil ved oppdatering av {fixture_abs} ({mode}): antall resultater avviker")

            for idx, item in enumerate(cases):
                case_count += 1
                actual_lines = actual_lists[idx]
                old_lines = item.get("expected_lines")
                old_error = item.get("expected_error")

                if actual_lines and actual_lines[0].startswith("/* feil:"):
                    new_error = actual_lines[0]
                    if old_error != new_error or old_lines is not None:
                        updated += 1
                    item["expected_error"] = new_error
                    item.pop("expected_lines", None)
                else:
                    if old_lines != actual_lines or old_error is not None:
                        updated += 1
                    item["expected_lines"] = actual_lines
                    item.pop("expected_error", None)

        total_updated += updated
        total_cases += case_count
        summaries.append(
            {
                "fixture": str(fixture_abs),
                "label": label,
                "cases": case_count,
                "updated": updated,
            }
        )
        if not check_only:
            fixture_abs.write_text(json.dumps(fixture, ensure_ascii=False, indent=2) + "\n", encoding="utf-8")

    return {
        "suite": suite,
        "check_only": check_only,
        "sync_m2": sync_m2,
        "m2_sync": m2_sync_payload,
        "updated": total_updated,
        "cases": total_cases,
        "fixtures": summaries,
    }


def sync_selfhost_parser_m2_fixture(check_only: bool = False) -> dict:
    m1_path = SELFHOST_PARSER_M1_FIXTURE.resolve()
    core_path = SELFHOST_PARSER_EXTENDED_FIXTURE.resolve()
    m2_path = SELFHOST_PARSER_M2_FIXTURE.resolve()

    m1 = json.loads(m1_path.read_text(encoding="utf-8"))
    core = json.loads(core_path.read_text(encoding="utf-8"))
    current_m2 = json.loads(m2_path.read_text(encoding="utf-8")) if m2_path.exists() else {"expressions": [], "scripts": []}

    target_m2: dict[str, list[dict]] = {"expressions": [], "scripts": []}
    missing_from_core: dict[str, list[str]] = {"expressions": [], "scripts": []}

    for mode in ("expressions", "scripts"):
        m1_cases = m1.get(mode, [])
        core_cases = core.get(mode, [])
        m1_names = {str(item.get("name", "")) for item in m1_cases if "name" in item}
        core_names = {str(item.get("name", "")) for item in core_cases if "name" in item}
        target_m2[mode] = [item for item in core_cases if str(item.get("name", "")) not in m1_names]
        missing_from_core[mode] = sorted(name for name in m1_names if name and name not in core_names)

    updated = 1 if current_m2 != target_m2 else 0
    if not check_only and updated:
        m2_path.write_text(json.dumps(target_m2, ensure_ascii=False, indent=2) + "\n", encoding="utf-8")

    missing_count = len(missing_from_core["expressions"]) + len(missing_from_core["scripts"])
    ok = updated == 0 and missing_count == 0

    return {
        "check_only": check_only,
        "ok": ok,
        "updated": updated,
        "fixture": str(m2_path),
        "m1_fixture": str(m1_path),
        "core_fixture": str(core_path),
        "m1_cases": len(m1.get("expressions", [])) + len(m1.get("scripts", [])),
        "core_cases": len(core.get("expressions", [])) + len(core.get("scripts", [])),
        "m2_cases": len(target_m2.get("expressions", [])) + len(target_m2.get("scripts", [])),
        "missing_m1_from_core_count": missing_count,
        "missing_m1_from_core": missing_from_core,
    }
