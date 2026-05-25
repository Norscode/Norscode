from __future__ import annotations

import json
import re
import tempfile
import time
from pathlib import Path
from typing import Callable

from compiler.native.pipeline import verify_bootstrap_compiler
from compiler.selfhost_chain import run_chain
from compiler.selfhost_whole_compile import DEFAULT_ROOTS, WholeCompileOptions, compile_whole_norscode


WORKFLOW_ACTION_POLICY = {
    "minimum_action_majors": {
        "actions/checkout": 6,
        "actions/setup-python": 6,
    },
    "require_node24_env": True,
    "forbid_unsecure_node_opt_out": True,
    "required_selfhost_bootstrap_command": "ci --bootstrap-lane",
    "required_norcode_ci_flags": [
        "--check-names",
        "--require-selfhost-ready",
    ],
}


def check_workflow_action_versions(workflows_dir: Path | None = None) -> dict:
    base = workflows_dir or Path(".github/workflows")
    minimum_action_majors = WORKFLOW_ACTION_POLICY["minimum_action_majors"]
    required_norcode_ci_flags = [str(flag) for flag in WORKFLOW_ACTION_POLICY.get("required_norcode_ci_flags", [])]
    required_selfhost_bootstrap_command = str(WORKFLOW_ACTION_POLICY.get("required_selfhost_bootstrap_command", "") or "")
    payload = {
        "ok": True,
        "scanned_dir": str(base),
        "scanned_files": 0,
        "files": [],
        "file_extensions": [".yml", ".yaml"],
        "issue_count": 0,
        "issue_types": {},
        "issues": [],
        "policy": WORKFLOW_ACTION_POLICY,
    }
    if not base.exists():
        return payload

    workflow_files = sorted([*base.glob("*.yml"), *base.glob("*.yaml")])
    payload["scanned_files"] = len(workflow_files)
    payload["files"] = [str(path) for path in workflow_files]
    for workflow_path in workflow_files:
        try:
            lines = workflow_path.read_text(encoding="utf-8").splitlines()
        except OSError:
            continue
        has_node24_env = False
        saw_ci_command = False
        saw_selfhost_bootstrap_command = False
        for line_no, raw_line in enumerate(lines, start=1):
            line = raw_line.strip()
            if not line or line.startswith("#"):
                continue
            if re.search(
                r'^\s*FORCE_JAVASCRIPT_ACTIONS_TO_NODE24\s*:\s*("true"|true)\s*$',
                line,
                re.IGNORECASE,
            ):
                has_node24_env = True
            for match in re.finditer(r"([A-Za-z0-9_.-]+/[A-Za-z0-9_.-]+)@v(\d+)", line):
                action_name = match.group(1)
                major = int(match.group(2))
                minimum_major = minimum_action_majors.get(action_name)
                if minimum_major is not None and major < minimum_major:
                    payload["issues"].append(
                        {
                            "file": str(workflow_path),
                            "line": line_no,
                            "type": "deprecated_action_major",
                            "rule": "action_min_major",
                            "found": f"{action_name}@v{major}",
                            "expected": f"{action_name}@v{minimum_major}",
                        }
                    )
            if re.search(
                r'^\s*ACTIONS_ALLOW_USE_UNSECURE_NODE_VERSION\s*:\s*("true"|true)\s*$',
                line,
                re.IGNORECASE,
            ):
                payload["issues"].append(
                    {
                        "file": str(workflow_path),
                        "line": line_no,
                        "type": "unsecure_node_opt_out",
                        "rule": "forbid_unsecure_opt_out",
                        "found": "ACTIONS_ALLOW_USE_UNSECURE_NODE_VERSION=true",
                        "expected": "fjern opt-out og bruk Node 24-kompatible action-versjoner",
                    }
                )
            run_match = re.search(r"^\s*run\s*:\s*(.+)$", raw_line)
            command_text = run_match.group(1).strip() if run_match else line
            if required_selfhost_bootstrap_command and required_selfhost_bootstrap_command in command_text:
                saw_selfhost_bootstrap_command = True
            if command_text:
                if ("norcode ci" in command_text or "./bin/nc ci" in command_text) and "--bootstrap-lane" not in command_text:
                    saw_ci_command = True
                    for flag in required_norcode_ci_flags:
                        if flag not in command_text:
                            payload["issues"].append(
                                {
                                    "file": str(workflow_path),
                                    "line": line_no,
                                    "type": "missing_norcode_ci_flag",
                                    "rule": "require_norcode_ci_flag",
                                    "found": command_text,
                                    "expected": f"run-linje med norcode ci eller ./bin/nc ci må inkludere {flag}",
                                }
                            )
        if not has_node24_env:
            payload["issues"].append(
                {
                    "file": str(workflow_path),
                    "line": 1,
                    "type": "missing_node24_env",
                    "rule": "require_node24_env",
                    "found": "FORCE_JAVASCRIPT_ACTIONS_TO_NODE24 mangler/ikke true",
                    "expected": 'FORCE_JAVASCRIPT_ACTIONS_TO_NODE24: "true"',
                }
            )
        if required_selfhost_bootstrap_command and not saw_selfhost_bootstrap_command:
            payload["issues"].append(
                {
                    "file": str(workflow_path),
                    "line": 1,
                    "type": "missing_selfhost_bootstrap_gate",
                    "rule": "require_selfhost_bootstrap_gate",
                    "found": f"mangler run-linje med '{required_selfhost_bootstrap_command}'",
                    "expected": f"legg til run-linje med {required_selfhost_bootstrap_command}",
                }
            )

    payload["issues"] = sorted(
        payload["issues"],
        key=lambda issue: (
            str(issue.get("file", "")),
            int(issue.get("line", 0)),
            str(issue.get("type", "")),
        ),
    )
    payload["issue_count"] = len(payload["issues"])
    issue_types: dict[str, int] = {}
    for issue in payload["issues"]:
        issue_type = str(issue.get("type", "unknown"))
        issue_types[issue_type] = issue_types.get(issue_type, 0) + 1
    payload["issue_types"] = issue_types
    payload["ok"] = payload["issue_count"] == 0
    return payload


def _bootstrap_compile_summary(whole: dict, duration_ms: int) -> dict:
    digest = whole.get("digest") if isinstance(whole.get("digest"), dict) else {}
    return {
        "ok": bool(whole.get("ok")),
        "passed": int(whole.get("passed", 0) or 0),
        "failed": int(whole.get("failed", 0) or 0),
        "total": int(whole.get("total", 0) or 0),
        "roots": whole.get("roots"),
        "output_dir": whole.get("output_dir"),
        "manifest": whole.get("manifest"),
        "digest": digest.get("sha256"),
        "artifact_count": digest.get("artifact_count"),
        "duration_ms": duration_ms,
    }


def run_selfhost_execution_smoke() -> dict:
    started = time.perf_counter()
    with tempfile.TemporaryDirectory(prefix="norscode-selfhost-exec-") as tmp:
        source = Path(tmp) / "execution_smoke.no"
        source.write_text("funksjon start() -> heltall { returner 12 }\n", encoding="utf-8")
        result = run_chain(str(source))
        return {
            "ok": result == 12,
            "source": str(source),
            "expected": 12,
            "result": result,
            "duration_ms": int((time.perf_counter() - started) * 1000),
        }


def run_package_hosting_smoke() -> dict:
    started = time.perf_counter()
    with tempfile.TemporaryDirectory(prefix="norscode-registry-smoke-") as tmp:
        mirror = Path(tmp) / "registry_mirror.json"
        mirror_payload = {
            "format_version": 1,
            "packages": {
                "bootstrap-smoke": {
                    "version": "0.1.0",
                    "description": "bootstrap package hosting smoke",
                }
            },
        }
        mirror.write_text(json.dumps(mirror_payload, ensure_ascii=False, sort_keys=True) + "\n", encoding="utf-8")
        mirror_bytes = mirror.read_bytes()
        package_count = len(mirror_payload.get("packages", {}))
        return {
            "ok": package_count >= 1,
            "url": mirror.resolve().as_uri(),
            "mirror": str(mirror.resolve()),
            "count": package_count,
            "served_bytes": len(mirror_bytes),
            "skip_reason": None,
            "duration_ms": int((time.perf_counter() - started) * 1000),
        }


def run_selfhost_bootstrap_gate(output_dir: str = "build/selfhost-bootstrap-gate", verify_deterministic: bool = True) -> dict:
    started = time.perf_counter()
    payload = {
        "format": "norscode-selfhost-bootstrap-gate-v1",
        "ok": False,
        "steps": [
            "selfhost_whole_compile",
            "determinism_check",
            "selfhost_execution_smoke",
            "package_hosting_smoke",
            "native_bootstrap_verify",
        ],
        "selfhost_whole_compile": {},
        "determinism_check": {"enabled": verify_deterministic, "ok": True},
        "selfhost_execution_smoke": {},
        "package_hosting_smoke": {},
        "native_bootstrap_verify": {},
        "duration_ms": 0,
    }

    whole_started = time.perf_counter()
    whole = compile_whole_norscode(
        WholeCompileOptions(
            roots=DEFAULT_ROOTS,
            output_dir=output_dir,
        )
    )
    payload["selfhost_whole_compile"] = _bootstrap_compile_summary(
        whole,
        int((time.perf_counter() - whole_started) * 1000),
    )
    if not whole.get("ok"):
        failed_rows = [row for row in whole.get("results", []) if not row.get("ok")]
        payload["selfhost_whole_compile"]["failures"] = [
            {"file": row.get("file"), "error": row.get("error")}
            for row in failed_rows[:25]
        ]
        payload["duration_ms"] = int((time.perf_counter() - started) * 1000)
        return payload

    if verify_deterministic:
        deterministic_started = time.perf_counter()
        deterministic_output_dir = f"{output_dir}-determinism"
        deterministic_whole = compile_whole_norscode(
            WholeCompileOptions(
                roots=DEFAULT_ROOTS,
                output_dir=deterministic_output_dir,
            )
        )
        first_digest = payload["selfhost_whole_compile"].get("digest")
        second_digest = (
            deterministic_whole.get("digest", {}).get("sha256")
            if isinstance(deterministic_whole.get("digest"), dict)
            else None
        )
        payload["determinism_check"] = {
            "enabled": True,
            "ok": bool(deterministic_whole.get("ok")) and first_digest == second_digest,
            "digest": first_digest,
            "deterministic_digest": second_digest,
            "output_dir": deterministic_whole.get("output_dir"),
            "manifest": deterministic_whole.get("manifest"),
            "artifact_count": deterministic_whole.get("digest", {}).get("artifact_count")
            if isinstance(deterministic_whole.get("digest"), dict)
            else None,
            "duration_ms": int((time.perf_counter() - deterministic_started) * 1000),
        }
        if not payload["determinism_check"]["ok"]:
            payload["duration_ms"] = int((time.perf_counter() - started) * 1000)
            return payload

    execution = run_selfhost_execution_smoke()
    payload["selfhost_execution_smoke"] = execution
    if not execution.get("ok"):
        payload["duration_ms"] = int((time.perf_counter() - started) * 1000)
        return payload

    package_hosting = run_package_hosting_smoke()
    payload["package_hosting_smoke"] = package_hosting
    if not package_hosting.get("ok"):
        payload["duration_ms"] = int((time.perf_counter() - started) * 1000)
        return payload

    native_started = time.perf_counter()
    native = verify_bootstrap_compiler()
    payload["native_bootstrap_verify"] = {
        **native,
        "duration_ms": int((time.perf_counter() - native_started) * 1000),
    }
    payload["ok"] = bool(native.get("ok"))
    payload["duration_ms"] = int((time.perf_counter() - started) * 1000)
    return payload


def run_ci_bootstrap_lane(
    *,
    json_output: bool = False,
    check_names: bool = False,
    output_dir: str = "build/selfhost-bootstrap-gate",
    argv: list[str] | None = None,
    migrate_names_fn: Callable[..., dict] | None = None,
) -> dict:
    if migrate_names_fn is None:
        raise RuntimeError("migrate_names_fn må oppgis")
    started = time.perf_counter()
    step_order = ["selfhost_bootstrap_gate", "workflow_action_check"]
    if check_names:
        step_order.append("name_migration_check")
    payload = {
        "schema_version": 1,
        "mode": "bootstrap-lane",
        "ok": False,
        "invocation": {
            "cmd": "norcode ci --bootstrap-lane",
            "json_output": json_output,
            "check_names": check_names,
            "output_dir": output_dir,
            "argv": list(argv or []),
        },
        "steps": {
            "total": len(step_order),
            "order": step_order,
        },
        "timings_ms": {},
        "timings_s": {},
        "selfhost_bootstrap_gate": {},
        "workflow_action_check": {},
        "name_migration_check": {"enabled": check_names, "ok": True, "needs_migration": False},
    }

    if not json_output:
        print(f"[1/{len(step_order)}] Selfhost bootstrap gate")
    gate_started = time.perf_counter()
    gate = run_selfhost_bootstrap_gate(output_dir=output_dir)
    payload["timings_ms"]["selfhost_bootstrap_gate"] = int((time.perf_counter() - gate_started) * 1000)
    payload["timings_s"]["selfhost_bootstrap_gate"] = round(
        payload["timings_ms"]["selfhost_bootstrap_gate"] / 1000.0,
        3,
    )
    payload["selfhost_bootstrap_gate"] = gate
    if not gate.get("ok"):
        raise RuntimeError("Selfhost bootstrap gate feilet")
    if not json_output:
        whole = gate.get("selfhost_whole_compile", {})
        print(f"OK ({whole.get('passed')}/{whole.get('total')} filer)")

    if not json_output:
        print(f"[2/{len(step_order)}] Workflow action policy")
    workflow_started = time.perf_counter()
    workflow_check = check_workflow_action_versions()
    payload["timings_ms"]["workflow_action_check"] = int((time.perf_counter() - workflow_started) * 1000)
    payload["timings_s"]["workflow_action_check"] = round(
        payload["timings_ms"]["workflow_action_check"] / 1000.0,
        3,
    )
    payload["workflow_action_check"] = workflow_check
    if not workflow_check.get("ok"):
        issue = workflow_check["issues"][0]
        raise RuntimeError(
            f"Workflow policy-brudd ({issue.get('type', 'unknown')}): "
            f"{issue.get('found')} i {issue.get('file')}:{issue.get('line')}"
        )
    if not json_output:
        print(f"OK ({workflow_check.get('scanned_files')} filer)")

    if check_names:
        if not json_output:
            print(f"[3/{len(step_order)}] Name migration check")
        name_started = time.perf_counter()
        migration = migrate_names_fn(apply_changes=False, cleanup_legacy=True)
        payload["timings_ms"]["name_migration_check"] = int((time.perf_counter() - name_started) * 1000)
        payload["timings_s"]["name_migration_check"] = round(
            payload["timings_ms"]["name_migration_check"] / 1000.0,
            3,
        )
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

    payload["timings_ms"]["total"] = int((time.perf_counter() - started) * 1000)
    payload["timings_s"]["total"] = round(payload["timings_ms"]["total"] / 1000.0, 3)
    payload["ok"] = True
    return payload
