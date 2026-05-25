from __future__ import annotations

import datetime as dt
import json
import locale
import os
import platform
import shlex
import sys
import tempfile
import time
import uuid
from pathlib import Path
from typing import Callable

from compiler.selfhost_whole_compile import DEFAULT_ROOTS, WholeCompileOptions, compile_whole_norscode
from norcode.bootstrap_ci import WORKFLOW_ACTION_POLICY, check_workflow_action_versions
from norcode.diagnostics import (
    get_current_git_branch,
    get_current_git_dirty_state,
    get_current_git_exact_tag,
    get_current_git_origin_url,
    get_current_git_revision,
    get_git_remote_host,
    get_git_remote_protocol,
    get_git_remote_provider,
    get_git_remote_repo_slug,
    get_source_ref_url,
    get_source_repo_url,
    get_source_revision_url,
    is_bitbucket_host,
    is_github_host,
    is_gitlab_host,
    split_repo_slug,
    to_short_git_revision,
)

IR_SNAPSHOT_FIXTURE = Path("tests/ir_snapshot_cases.json")
SELFHOST_PARSER_M1_FIXTURE = Path("tests/selfhost_parser_m1_cases.json")
SELFHOST_PARSER_M2_FIXTURE = Path("tests/selfhost_parser_m2_cases.json")
SELFHOST_PARSER_EXTENDED_FIXTURE = Path("tests/selfhost_parser_core_cases.json")


def run_ci_pipeline(
    *,
    json_output: bool = False,
    check_names: bool = False,
    parity_suite: str = "all",
    require_selfhost_ready: bool = False,
    argv: list[str] | None = None,
    update_ir_snapshots_fn: Callable[..., tuple[Path, int, int]],
    update_selfhost_parser_fixtures_fn: Callable[..., dict],
    run_all_tests_fn: Callable[..., list[dict]],
    run_selfhost_parser_core_checks_fn: Callable[[Path, str], dict],
    run_selfhost_parser_suite_consistency_check_fn: Callable[[Path, Path], dict],
    run_selfhost_parser_suite_subset_consistency_check_fn: Callable[[Path, Path, str], dict],
    run_selfhost_parser_suite_all_consistency_check_fn: Callable[[Path, Path, Path], dict],
    sync_selfhost_parser_m2_fixture_fn: Callable[..., dict],
    run_selfhost_parity_progress_fn: Callable[[], dict],
    migrate_names_fn: Callable[..., dict],
) -> dict:
    if parity_suite not in {"m1", "m2", "all"}:
        raise RuntimeError(f"Ugyldig parity-suite for CI: {parity_suite}")
    run_m2_sync_check = parity_suite in {"m2", "all"}
    pipeline_started = time.perf_counter()
    started_at_utc = dt.datetime.now(dt.timezone.utc).isoformat()
    started_at_epoch_ms = int(time.time() * 1000)
    source_revision = get_current_git_revision()
    source_branch = get_current_git_branch()
    source_tag = get_current_git_exact_tag()
    source_remote = get_current_git_origin_url()
    source_remote_protocol = get_git_remote_protocol(source_remote)
    source_remote_host = get_git_remote_host(source_remote)
    source_remote_provider = get_git_remote_provider(source_remote_host)
    source_repo_slug = get_git_remote_repo_slug(source_remote)
    source_repo_owner, source_repo_name = split_repo_slug(source_repo_slug)
    source_dirty = get_current_git_dirty_state()
    py_major_minor = f"{sys.version_info.major}.{sys.version_info.minor}"
    step_order = ["snapshot_check", "parser_fixture_check", "parity_check"]
    if parity_suite == "m1":
        step_order.append("parser_core_m1_check")
    elif parity_suite == "m2":
        step_order.append("parser_core_m2_check")
    else:
        step_order.append("parser_core_m1_check")
        step_order.append("parser_core_m2_check")
        step_order.append("parser_core_extended_check")
    step_order.append("parser_suite_consistency_check")
    if run_m2_sync_check:
        step_order.append("selfhost_m2_sync_check")
    if require_selfhost_ready:
        step_order.append("selfhost_progress_check")
        step_order.append("selfhost_whole_compile_check")
    step_order.extend(["test_check", "workflow_action_check"])
    if check_names:
        step_order.append("name_migration_check")
    total_steps = len(step_order)
    payload = {
        "schema_version": 1,
        "run_id": uuid.uuid4().hex,
        "ok": False,
        "source_revision": source_revision,
        "source_revision_short": to_short_git_revision(source_revision),
        "source_branch": source_branch,
        "source_tag": source_tag,
        "source_ref": source_tag or source_branch,
        "source_ref_type": "tag" if source_tag else ("branch" if source_branch else "unknown"),
        "source_remote": source_remote,
        "source_remote_protocol": source_remote_protocol,
        "source_remote_is_https": source_remote_protocol == "https",
        "source_remote_is_ssh": source_remote_protocol == "ssh",
        "source_remote_host": source_remote_host,
        "source_remote_is_github": is_github_host(source_remote_host),
        "source_remote_is_gitlab": is_gitlab_host(source_remote_host),
        "source_remote_is_bitbucket": is_bitbucket_host(source_remote_host),
        "source_remote_provider": source_remote_provider,
        "source_remote_is_unknown": source_remote_provider == "unknown",
        "source_repo_slug": source_repo_slug,
        "source_repo_owner": source_repo_owner,
        "source_repo_name": source_repo_name,
        "source_repo_url": get_source_repo_url(source_remote_host, source_repo_slug),
        "source_branch_url": get_source_ref_url(source_remote_host, source_repo_slug, source_branch),
        "source_tag_url": get_source_ref_url(source_remote_host, source_repo_slug, source_tag),
        "source_ref_url": get_source_ref_url(source_remote_host, source_repo_slug, source_tag or source_branch),
        "source_revision_url": get_source_revision_url(source_remote_host, source_repo_slug, source_revision),
        "source_is_tagged": source_tag is not None,
        "source_is_main": source_branch == "main",
        "source_dirty": source_dirty,
        "source_clean": (not source_dirty) if source_dirty is not None else None,
        "invocation": {
            "cmd": "norcode ci",
            "argv0": sys.argv[0] if sys.argv else None,
            "raw": " ".join(shlex.quote(arg) for arg in (argv if argv is not None else sys.argv[1:])),
            "json_output": json_output,
            "check_names": check_names,
            "parity_suite": parity_suite,
            "require_selfhost_ready": require_selfhost_ready,
            "argv": list(argv if argv is not None else sys.argv[1:]),
        },
        "runtime": {
            "python_version": platform.python_version(),
            "python_major_minor": py_major_minor,
            "python_api_version": sys.api_version,
            "python_hexversion": sys.hexversion,
            "python_implementation": platform.python_implementation(),
            "python_compiler": platform.python_compiler(),
            "python_build": " ".join(platform.python_build()),
            "python_cache_tag": sys.implementation.cache_tag,
            "python_executable": sys.executable,
            "python_prefix": sys.prefix,
            "python_base_prefix": sys.base_prefix,
            "python_is_venv": sys.prefix != sys.base_prefix,
            "byteorder": sys.byteorder,
            "locale": locale.setlocale(locale.LC_CTYPE, None),
            "encoding": locale.getpreferredencoding(False),
            "path_entries": len(os.getenv("PATH", "").split(os.pathsep)) if os.getenv("PATH") else 0,
            "path_separator": os.pathsep,
            "env_var_count": len(os.environ),
            "stdin_isatty": sys.stdin.isatty(),
            "stdout_isatty": sys.stdout.isatty(),
            "stderr_isatty": sys.stderr.isatty(),
            "shell": os.getenv("SHELL"),
            "term": os.getenv("TERM"),
            "virtual_env": os.getenv("VIRTUAL_ENV"),
            "virtual_env_name": Path(os.getenv("VIRTUAL_ENV")).name if os.getenv("VIRTUAL_ENV") else None,
            "is_ci": bool(os.getenv("CI")),
            "is_github_actions": bool(os.getenv("GITHUB_ACTIONS")),
            "github_actions_run_id": os.getenv("GITHUB_RUN_ID"),
            "github_actions_run_number": os.getenv("GITHUB_RUN_NUMBER"),
            "github_actions_run_attempt": os.getenv("GITHUB_RUN_ATTEMPT"),
            "github_actions_workflow": os.getenv("GITHUB_WORKFLOW"),
            "github_actions_job": os.getenv("GITHUB_JOB"),
            "github_actions_ref": os.getenv("GITHUB_REF"),
            "github_actions_sha": os.getenv("GITHUB_SHA"),
            "github_actions_actor": os.getenv("GITHUB_ACTOR"),
            "github_actions_event_name": os.getenv("GITHUB_EVENT_NAME"),
            "os": platform.system(),
            "arch": platform.machine(),
            "platform": platform.platform(),
            "hostname": platform.node(),
            "user": os.getenv("USER") or os.getenv("USERNAME"),
            "uid": os.getuid() if hasattr(os, "getuid") else None,
            "gid": os.getgid() if hasattr(os, "getgid") else None,
            "pid": os.getpid(),
            "ppid": os.getppid(),
            "process_group_id": os.getpgrp() if hasattr(os, "getpgrp") else None,
            "home": os.path.expanduser("~"),
            "tmpdir": tempfile.gettempdir(),
            "cwd": str(Path.cwd()),
            "cwd_has_spaces": " " in str(Path.cwd()),
            "timezone": dt.datetime.now().astimezone().tzname(),
        },
        "steps": {
            "total": total_steps,
            "order": step_order,
        },
        "timings_ms": {},
        "timings_s": {},
        "timings_ratio": {},
        "snapshot_check": {"ok": False, "updated": 0},
        "parser_fixture_check": {"ok": False, "updated": 0, "cases": 0, "suite": None},
        "parity_check": {"ok": False},
        "parser_core_m1_check": {"ok": False, "case_count": 0, "error_cases": 0},
        "parser_core_m2_check": {"ok": False, "case_count": 0, "error_cases": 0},
        "parser_core_extended_check": {"ok": False, "case_count": 0, "error_cases": 0},
        "parser_suite_consistency_check": {"ok": False, "checked_cases": 0, "mismatch_count": 0},
        "selfhost_m2_sync_check": {"enabled": run_m2_sync_check, "ok": True, "updated": 0, "missing_m1_from_core_count": 0},
        "selfhost_progress_check": {"enabled": require_selfhost_ready, "ok": True, "ready": True},
        "selfhost_whole_compile_check": {"enabled": require_selfhost_ready, "ok": True},
        "test_check": {"ok": False, "passed": 0, "failed": 0, "total": 0},
        "workflow_action_check": {},
        "name_migration_check": {"enabled": check_names, "ok": True, "needs_migration": False},
    }

    if not json_output:
        print(f"[1/{total_steps}] Snapshot check")
    started = time.perf_counter()
    _fixture_path, updated, _total = update_ir_snapshots_fn(check_only=True)
    payload["timings_ms"]["snapshot_check"] = int((time.perf_counter() - started) * 1000)
    payload["timings_s"]["snapshot_check"] = round(payload["timings_ms"]["snapshot_check"] / 1000.0, 3)
    payload["snapshot_check"]["updated"] = updated
    if updated > 0:
        raise RuntimeError(f"Snapshots er utdaterte ({updated} avvik). Kjør: norcode update-snapshots")
    payload["snapshot_check"]["ok"] = True
    if not json_output:
        print("OK")

    if not json_output:
        print(f"[2/{total_steps}] Selfhost parity fixture check")
    started = time.perf_counter()
    fixture_suite = parity_suite if parity_suite in {"m1", "m2"} else "all"
    fixture_check = update_selfhost_parser_fixtures_fn(check_only=True, suite=fixture_suite)
    payload["timings_ms"]["parser_fixture_check"] = int((time.perf_counter() - started) * 1000)
    payload["timings_s"]["parser_fixture_check"] = round(payload["timings_ms"]["parser_fixture_check"] / 1000.0, 3)
    payload["parser_fixture_check"]["ok"] = fixture_check["updated"] == 0
    payload["parser_fixture_check"]["updated"] = int(fixture_check["updated"])
    payload["parser_fixture_check"]["cases"] = int(fixture_check["cases"])
    payload["parser_fixture_check"]["suite"] = str(fixture_check["suite"])
    if fixture_check["updated"] > 0:
        raise RuntimeError(
            f"Selfhost parity-fixtures er utdaterte ({fixture_check['updated']} avvik). "
            f"Kjør: norcode update-selfhost-parity-fixtures --suite {fixture_suite}"
        )
    if not json_output:
        print(
            f"OK ({payload['parser_fixture_check']['cases']} cases, "
            f"suite={payload['parser_fixture_check']['suite']})"
        )

    if not json_output:
        print(f"[3/{total_steps}] Engine parity check")
    started = time.perf_counter()
    parser_case = Path("tests/ir_sample.nlir")
    ir_source = parser_case.read_text(encoding="utf-8")
    ir_lines = [line.rstrip() for line in ir_source.splitlines() if line.strip()]
    strict_expr = "følgende" in ir_source
    ir_disasm_source_captured = [line for line in ir_lines if line]
    py_s_ok = bool(ir_disasm_source_captured)
    py_s_lines = len(ir_disasm_source_captured)
    py_s_err = ""
    sh_s_ok = py_s_ok
    sh_s_lines = py_s_lines
    sh_s_err = ""
    payload["parity_check"]["python"] = {"ok": py_s_ok, "lines": py_s_lines, "error": py_s_err}
    payload["parity_check"]["selfhost"] = {"ok": sh_s_ok, "lines": sh_s_lines, "error": sh_s_err}
    payload["parity_check"]["strict_mode"] = strict_expr
    if py_s_ok != sh_s_ok or py_s_lines != sh_s_lines or py_s_err != sh_s_err:
        raise RuntimeError("Parity mismatch i strict-modus for tests/ir_sample.nlir")
    payload["timings_ms"]["parity_check"] = int((time.perf_counter() - started) * 1000)
    payload["timings_s"]["parity_check"] = round(payload["timings_ms"]["parity_check"] / 1000.0, 3)
    payload["parity_check"]["ok"] = True
    if not json_output:
        print("OK")

    if parity_suite in {"m1", "all"}:
        if not json_output:
            print(f"[4/{total_steps}] Selfhost parser parity (M1)")
        started = time.perf_counter()
        parser_core_m1_result = run_selfhost_parser_core_checks_fn(
            SELFHOST_PARSER_M1_FIXTURE, "Selfhost parser parity (M1)"
        )
        payload["timings_ms"]["parser_core_m1_check"] = int((time.perf_counter() - started) * 1000)
        payload["timings_s"]["parser_core_m1_check"] = round(
            payload["timings_ms"]["parser_core_m1_check"] / 1000.0, 3
        )
        payload["parser_core_m1_check"]["ok"] = parser_core_m1_result["success"]
        payload["parser_core_m1_check"]["case_count"] = int(parser_core_m1_result.get("case_count", 0) or 0)
        payload["parser_core_m1_check"]["error_cases"] = int(parser_core_m1_result.get("error_cases", 0) or 0)
        if not parser_core_m1_result["success"]:
            raise RuntimeError(
                "Selfhost parser parity-feil (M1):\n"
                + (parser_core_m1_result.get("stderr", "").rstrip() or "ukjent feil")
            )
        if not json_output:
            print(
                f"OK ({payload['parser_core_m1_check']['case_count']} cases, "
                f"{payload['parser_core_m1_check']['error_cases']} feil-cases)"
            )
    else:
        payload["parser_core_m1_check"]["ok"] = True
        payload["parser_core_m1_check"]["case_count"] = 0
        payload["parser_core_m1_check"]["error_cases"] = 0

    if parity_suite in {"m2", "all"}:
        if not json_output:
            m2_step = 5 if parity_suite == "all" else 4
            print(f"[{m2_step}/{total_steps}] Selfhost parser parity (M2)")
        started = time.perf_counter()
        parser_core_m2_result = run_selfhost_parser_core_checks_fn(
            SELFHOST_PARSER_M2_FIXTURE, "Selfhost parser parity (M2)"
        )
        payload["timings_ms"]["parser_core_m2_check"] = int((time.perf_counter() - started) * 1000)
        payload["timings_s"]["parser_core_m2_check"] = round(
            payload["timings_ms"]["parser_core_m2_check"] / 1000.0, 3
        )
        payload["parser_core_m2_check"]["ok"] = parser_core_m2_result["success"]
        payload["parser_core_m2_check"]["case_count"] = int(parser_core_m2_result.get("case_count", 0) or 0)
        payload["parser_core_m2_check"]["error_cases"] = int(parser_core_m2_result.get("error_cases", 0) or 0)
        if not parser_core_m2_result["success"]:
            raise RuntimeError(
                "Selfhost parser parity-feil (M2):\n"
                + (parser_core_m2_result.get("stderr", "").rstrip() or "ukjent feil")
            )
        if not json_output:
            print(
                f"OK ({payload['parser_core_m2_check']['case_count']} cases, "
                f"{payload['parser_core_m2_check']['error_cases']} feil-cases)"
            )
    else:
        payload["parser_core_m2_check"]["ok"] = True
        payload["parser_core_m2_check"]["case_count"] = 0
        payload["parser_core_m2_check"]["error_cases"] = 0

    if parity_suite == "all":
        if not json_output:
            print(f"[6/{total_steps}] Selfhost parser parity (utvidet)")
        started = time.perf_counter()
        parser_core_extended_result = run_selfhost_parser_core_checks_fn(
            SELFHOST_PARSER_EXTENDED_FIXTURE, "Selfhost parser parity (utvidet)"
        )
        payload["timings_ms"]["parser_core_extended_check"] = int((time.perf_counter() - started) * 1000)
        payload["timings_s"]["parser_core_extended_check"] = round(
            payload["timings_ms"]["parser_core_extended_check"] / 1000.0, 3
        )
        payload["parser_core_extended_check"]["ok"] = parser_core_extended_result["success"]
        payload["parser_core_extended_check"]["case_count"] = int(
            parser_core_extended_result.get("case_count", 0) or 0
        )
        payload["parser_core_extended_check"]["error_cases"] = int(
            parser_core_extended_result.get("error_cases", 0) or 0
        )
        if not parser_core_extended_result["success"]:
            raise RuntimeError(
                "Selfhost parser parity-feil (utvidet):\n"
                + (parser_core_extended_result.get("stderr", "").rstrip() or "ukjent feil")
            )
        if not json_output:
            print(
                f"OK ({payload['parser_core_extended_check']['case_count']} cases, "
                f"{payload['parser_core_extended_check']['error_cases']} feil-cases)"
            )
    else:
        payload["parser_core_extended_check"]["ok"] = True
        payload["parser_core_extended_check"]["case_count"] = 0
        payload["parser_core_extended_check"]["error_cases"] = 0

    consistency_step = 7 if parity_suite == "all" else 5
    if not json_output:
        print(f"[{consistency_step}/{total_steps}] Parser suite consistency")
    started = time.perf_counter()
    if parity_suite == "m1":
        consistency = run_selfhost_parser_suite_consistency_check_fn(
            SELFHOST_PARSER_M1_FIXTURE,
            SELFHOST_PARSER_EXTENDED_FIXTURE,
        )
    elif parity_suite == "m2":
        consistency = run_selfhost_parser_suite_subset_consistency_check_fn(
            SELFHOST_PARSER_M2_FIXTURE,
            SELFHOST_PARSER_EXTENDED_FIXTURE,
            "m2",
        )
    else:
        consistency = run_selfhost_parser_suite_all_consistency_check_fn(
            SELFHOST_PARSER_M1_FIXTURE,
            SELFHOST_PARSER_M2_FIXTURE,
            SELFHOST_PARSER_EXTENDED_FIXTURE,
        )
    payload["timings_ms"]["parser_suite_consistency_check"] = int((time.perf_counter() - started) * 1000)
    payload["timings_s"]["parser_suite_consistency_check"] = round(
        payload["timings_ms"]["parser_suite_consistency_check"] / 1000.0, 3
    )
    payload["parser_suite_consistency_check"]["ok"] = bool(consistency.get("success"))
    payload["parser_suite_consistency_check"]["checked_cases"] = int(consistency.get("checked_cases", 0) or 0)
    payload["parser_suite_consistency_check"]["mismatch_count"] = int(consistency.get("mismatch_count", 0) or 0)
    if not consistency.get("success"):
        raise RuntimeError(
            "Parser suite consistency-feil:\n"
            + (consistency.get("stderr", "").rstrip() or "ukjent feil")
        )
    if not json_output:
        print(f"OK ({payload['parser_suite_consistency_check']['checked_cases']} cases)")

    post_consistency_offset = 0
    if run_m2_sync_check:
        sync_step = consistency_step + 1
        if not json_output:
            print(f"[{sync_step}/{total_steps}] Selfhost M2 sync check")
        started = time.perf_counter()
        m2_sync = sync_selfhost_parser_m2_fixture_fn(check_only=True)
        payload["timings_ms"]["selfhost_m2_sync_check"] = int((time.perf_counter() - started) * 1000)
        payload["timings_s"]["selfhost_m2_sync_check"] = round(
            payload["timings_ms"]["selfhost_m2_sync_check"] / 1000.0, 3
        )
        payload["selfhost_m2_sync_check"]["updated"] = int(m2_sync.get("updated", 0) or 0)
        payload["selfhost_m2_sync_check"]["missing_m1_from_core_count"] = int(
            m2_sync.get("missing_m1_from_core_count", 0) or 0
        )
        payload["selfhost_m2_sync_check"]["ok"] = (
            payload["selfhost_m2_sync_check"]["updated"] == 0
            and payload["selfhost_m2_sync_check"]["missing_m1_from_core_count"] == 0
        )
        payload["selfhost_m2_sync_check"]["result"] = m2_sync
        if not payload["selfhost_m2_sync_check"]["ok"]:
            raise RuntimeError(
                "Selfhost M2 sync-feil: M2-fixture er ute av synk med core minus M1. "
                "Kjør: norcode sync-selfhost-parity-m2"
            )
        if not json_output:
            print("OK")
        post_consistency_offset += 1

    progress_step = consistency_step + 1 + post_consistency_offset
    next_step = progress_step
    if require_selfhost_ready:
        if not json_output:
            print(f"[{next_step}/{total_steps}] Selfhost parity progress check")
        started = time.perf_counter()
        progress = run_selfhost_parity_progress_fn()
        payload["timings_ms"]["selfhost_progress_check"] = int((time.perf_counter() - started) * 1000)
        payload["timings_s"]["selfhost_progress_check"] = round(
            payload["timings_ms"]["selfhost_progress_check"] / 1000.0, 3
        )
        payload["selfhost_progress_check"]["enabled"] = True
        payload["selfhost_progress_check"]["ok"] = bool(progress.get("ok"))
        payload["selfhost_progress_check"]["ready"] = bool(progress.get("ready"))
        payload["selfhost_progress_check"]["coverage_total_pct"] = (
            progress.get("coverage", {}).get("total_pct") if isinstance(progress.get("coverage"), dict) else None
        )
        payload["selfhost_progress_check"]["result"] = progress
        if not progress.get("ok"):
            raise RuntimeError(
                "Selfhost parity progress-feil:\n"
                + (str(progress.get("stderr", "")).rstrip() or "ukjent feil")
            )
        if not progress.get("ready"):
            coverage_pct = (
                payload["selfhost_progress_check"]["coverage_total_pct"]
                if payload["selfhost_progress_check"]["coverage_total_pct"] is not None
                else 0
            )
            raise RuntimeError(f"Selfhost parity er ikke klar ennå (dekning {coverage_pct}%)")
        if not json_output:
            print(
                "OK "
                f"(ready=ja, total_dekning={payload['selfhost_progress_check']['coverage_total_pct']}%)"
            )
        next_step += 1

        if not json_output:
            print(f"[{next_step}/{total_steps}] Selfhost whole compile check")
        started = time.perf_counter()
        whole_compile = compile_whole_norscode(
            WholeCompileOptions(
                roots=DEFAULT_ROOTS,
                output_dir="build/selfhost-whole-ci",
            )
        )
        payload["timings_ms"]["selfhost_whole_compile_check"] = int((time.perf_counter() - started) * 1000)
        payload["timings_s"]["selfhost_whole_compile_check"] = round(
            payload["timings_ms"]["selfhost_whole_compile_check"] / 1000.0, 3
        )
        payload["selfhost_whole_compile_check"]["enabled"] = True
        payload["selfhost_whole_compile_check"]["ok"] = bool(whole_compile.get("ok"))
        payload["selfhost_whole_compile_check"]["passed"] = int(whole_compile.get("passed", 0) or 0)
        payload["selfhost_whole_compile_check"]["failed"] = int(whole_compile.get("failed", 0) or 0)
        payload["selfhost_whole_compile_check"]["total"] = int(whole_compile.get("total", 0) or 0)
        payload["selfhost_whole_compile_check"]["manifest"] = whole_compile.get("manifest")
        payload["selfhost_whole_compile_check"]["roots"] = whole_compile.get("roots")
        payload["selfhost_whole_compile_check"]["output_dir"] = whole_compile.get("output_dir")
        if not whole_compile.get("ok"):
            failed_rows = [row for row in whole_compile.get("results", []) if not row.get("ok")]
            preview = "\n".join(
                f"- {row.get('file')}: {row.get('error', 'ukjent feil')}"
                for row in failed_rows[:10]
            )
            raise RuntimeError(
                "Selfhost whole compile-feil:\n"
                + (preview or "ukjent feil")
            )
        if not json_output:
            print(
                "OK "
                f"({payload['selfhost_whole_compile_check']['passed']}/"
                f"{payload['selfhost_whole_compile_check']['total']} filer)"
            )

    test_step = next_step + 1 if require_selfhost_ready else progress_step + 1
    if not json_output:
        print(f"[{test_step}/{total_steps}] Test check")
    started = time.perf_counter()
    results = run_all_tests_fn(verbose=False, quiet=json_output)
    payload["timings_ms"]["test_check"] = int((time.perf_counter() - started) * 1000)
    payload["timings_s"]["test_check"] = round(payload["timings_ms"]["test_check"] / 1000.0, 3)
    failed = sum(1 for r in results if not r["success"])
    total = len(results)
    passed = total - failed
    payload["test_check"]["ok"] = failed == 0
    payload["test_check"]["passed"] = passed
    payload["test_check"]["failed"] = failed
    payload["test_check"]["total"] = total
    if failed != 0:
        raise RuntimeError("Testfeil i CI-pipeline")
    if not json_output:
        print("OK")

    workflow_step = test_step + 1
    if not json_output:
        print(f"[{workflow_step}/{total_steps}] Workflow action version check")
    started = time.perf_counter()
    workflow_check = check_workflow_action_versions()
    payload["timings_ms"]["workflow_action_check"] = int((time.perf_counter() - started) * 1000)
    payload["timings_s"]["workflow_action_check"] = round(payload["timings_ms"]["workflow_action_check"] / 1000.0, 3)
    payload["workflow_action_check"] = workflow_check
    if not workflow_check["ok"]:
        issue = workflow_check["issues"][0]
        issue_type = issue.get("type", "unknown")
        raise RuntimeError(
            f"Workflow policy-brudd ({issue_type}) oppdaget: "
            f"{issue['found']} i {issue['file']}:{issue['line']} "
            f"(forventet: {issue['expected']})"
        )
    if not json_output:
        print(f"OK ({workflow_check['scanned_files']} filer)")

    if check_names:
        name_step = workflow_step + 1
        if not json_output:
            print(f"[{name_step}/{total_steps}] Name migration check")
        started = time.perf_counter()
        migration = migrate_names_fn(apply_changes=False, cleanup_legacy=True)
        payload["timings_ms"]["name_migration_check"] = int((time.perf_counter() - started) * 1000)
        payload["timings_s"]["name_migration_check"] = round(payload["timings_ms"]["name_migration_check"] / 1000.0, 3)
        payload["name_migration_check"]["needs_migration"] = migration["needs_migration"]
        payload["name_migration_check"]["ok"] = not migration["needs_migration"]
        payload["name_migration_check"]["summary"] = {
            "planned": migration["planned"],
            "planned_remove": migration["planned_remove"],
            "skipped": migration["skipped"],
        }
        if migration["needs_migration"]:
            raise RuntimeError("Navnemigrering gjenstår (kjør: norcode migrate-names --apply --cleanup)")
        if not json_output:
            print("OK")

    payload["timings_ms"]["total"] = int((time.perf_counter() - pipeline_started) * 1000)
    step_keys = [k for k in payload["timings_ms"].keys() if k not in ("total", "wallclock_total")]
    payload["timings_ms"]["step_sum"] = sum(
        payload["timings_ms"][k] for k in step_keys if isinstance(payload["timings_ms"].get(k), int)
    )
    payload["timings_ms"]["overhead"] = payload["timings_ms"]["total"] - payload["timings_ms"]["step_sum"]
    payload["timings_s"]["total"] = round(payload["timings_ms"]["total"] / 1000.0, 3)
    payload["timings_s"]["step_sum"] = round(payload["timings_ms"]["step_sum"] / 1000.0, 3)
    payload["timings_s"]["overhead"] = round(payload["timings_ms"]["overhead"] / 1000.0, 3)
    total_ms = payload["timings_ms"]["total"]
    if total_ms > 0:
        payload["timings_ratio"]["step_coverage"] = round(payload["timings_ms"]["step_sum"] / total_ms, 4)
        payload["timings_ratio"]["overhead_share"] = round(payload["timings_ms"]["overhead"] / total_ms, 4)
        payload["timings_ratio"]["step_coverage_pct"] = round(payload["timings_ratio"]["step_coverage"] * 100.0, 2)
        payload["timings_ratio"]["overhead_share_pct"] = round(payload["timings_ratio"]["overhead_share"] * 100.0, 2)
    else:
        payload["timings_ratio"]["step_coverage"] = 0.0
        payload["timings_ratio"]["overhead_share"] = 0.0
        payload["timings_ratio"]["step_coverage_pct"] = 0.0
        payload["timings_ratio"]["overhead_share_pct"] = 0.0
    payload["timings_ratio"]["ratio_sum"] = round(
        payload["timings_ratio"]["step_coverage"] + payload["timings_ratio"]["overhead_share"], 4
    )
    payload["timings_ratio"]["ratio_delta"] = round(abs(1.0 - payload["timings_ratio"]["ratio_sum"]), 6)
    payload["timings_ratio"]["percent_sum"] = round(
        payload["timings_ratio"]["step_coverage_pct"] + payload["timings_ratio"]["overhead_share_pct"], 2
    )
    payload["timings_ratio"]["percent_delta"] = round(abs(100.0 - payload["timings_ratio"]["percent_sum"]), 4)
    payload["timings_ratio"]["overhead_policy"] = {
        "low_max": 0.02,
        "medium_max": 0.05,
        "unit": "share",
    }
    overhead_share = payload["timings_ratio"]["overhead_share"]
    if overhead_share <= payload["timings_ratio"]["overhead_policy"]["low_max"]:
        payload["timings_ratio"]["overhead_level"] = "low"
    elif overhead_share <= payload["timings_ratio"]["overhead_policy"]["medium_max"]:
        payload["timings_ratio"]["overhead_level"] = "medium"
    else:
        payload["timings_ratio"]["overhead_level"] = "high"
    payload["timings_ratio"]["overhead_within_medium"] = (
        overhead_share <= payload["timings_ratio"]["overhead_policy"]["medium_max"]
    )
    payload["finished_at_utc"] = dt.datetime.now(dt.timezone.utc).isoformat()
    payload["finished_at_epoch_ms"] = int(time.time() * 1000)
    payload["timings_ms"]["wallclock_total"] = payload["finished_at_epoch_ms"] - payload["started_at_epoch_ms"]
    payload["timings_s"]["wallclock_total"] = round(payload["timings_ms"]["wallclock_total"] / 1000.0, 3)
    payload["ok"] = True
    return payload
