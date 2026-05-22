from __future__ import annotations

from pathlib import Path

import main


def _patch_fast_ci(monkeypatch) -> None:
    monkeypatch.setattr(main, "get_current_git_revision", lambda: "abc123")
    monkeypatch.setattr(main, "get_current_git_branch", lambda: "main")
    monkeypatch.setattr(main, "get_current_git_exact_tag", lambda: None)
    monkeypatch.setattr(main, "get_current_git_origin_url", lambda: "")
    monkeypatch.setattr(main, "get_git_remote_protocol", lambda _remote: "unknown")
    monkeypatch.setattr(main, "get_git_remote_host", lambda _remote: "")
    monkeypatch.setattr(main, "get_git_remote_provider", lambda _host: "unknown")
    monkeypatch.setattr(main, "get_git_remote_repo_slug", lambda _remote: "")
    monkeypatch.setattr(main, "split_repo_slug", lambda _slug: (None, None))
    monkeypatch.setattr(main, "get_source_repo_url", lambda _host, _slug: None)
    monkeypatch.setattr(main, "get_source_ref_url", lambda _host, _slug, _ref: None)
    monkeypatch.setattr(main, "get_source_revision_url", lambda _host, _slug, _revision: None)
    monkeypatch.setattr(main, "get_current_git_dirty_state", lambda: False)
    monkeypatch.setattr(main, "update_ir_snapshots", lambda check_only: (Path("tests/ir_snapshot_cases.json"), 0, 1))
    monkeypatch.setattr(
        main,
        "update_selfhost_parser_fixtures",
        lambda check_only, suite: {"updated": 0, "cases": 1, "suite": suite},
    )
    monkeypatch.setattr(main, "ir_disasm_source_captured", lambda *_args, **_kwargs: ("tests/ir_sample.nlir", True, ["HALT"], ""))
    monkeypatch.setattr(
        main,
        "run_selfhost_parser_core_checks",
        lambda _fixture, _label: {"success": True, "case_count": 1, "error_cases": 0},
    )
    monkeypatch.setattr(
        main,
        "run_selfhost_parser_suite_all_consistency_check",
        lambda *_args: {"success": True, "checked_cases": 1, "mismatch_count": 0},
    )
    monkeypatch.setattr(main, "sync_selfhost_parser_m2_fixture", lambda check_only: {"updated": 0, "missing_m1_from_core_count": 0})
    monkeypatch.setattr(main, "run_selfhost_parity_progress", lambda: {"ok": True, "ready": True, "coverage": {"total_pct": 100.0}})
    monkeypatch.setattr(main, "run_all_tests", lambda verbose, quiet: [])
    monkeypatch.setattr(
        main,
        "check_workflow_action_versions",
        lambda: {"ok": True, "scanned_files": 0, "issues": []},
    )


def test_ci_require_selfhost_ready_runs_whole_compile_gate(monkeypatch):
    _patch_fast_ci(monkeypatch)
    calls = []

    def fake_compile(options):
        calls.append(options)
        return {
            "ok": True,
            "roots": list(options.roots),
            "output_dir": options.output_dir,
            "total": 296,
            "passed": 296,
            "failed": 0,
            "manifest": "build/selfhost-whole-ci/manifest.json",
            "results": [],
        }

    monkeypatch.setattr(main, "compile_whole_norscode", fake_compile)

    payload = main.run_ci_pipeline(json_output=True, require_selfhost_ready=True)

    assert payload["ok"] is True
    assert len(calls) == 1
    assert calls[0].roots == main.DEFAULT_ROOTS
    assert calls[0].output_dir == "build/selfhost-whole-ci"
    assert "selfhost_whole_compile_check" in payload["steps"]["order"]
    assert payload["selfhost_whole_compile_check"]["ok"] is True
    assert payload["selfhost_whole_compile_check"]["passed"] == 296


def test_selfhost_bootstrap_gate_combines_whole_compile_and_native_verify(monkeypatch):
    def fake_compile(options):
        digest = "a" * 64
        return {
            "ok": True,
            "roots": list(options.roots),
            "output_dir": options.output_dir,
            "total": 296,
            "passed": 296,
            "failed": 0,
            "manifest": "build/selfhost-bootstrap-gate/manifest.json",
            "digest": {"sha256": digest, "artifact_count": 592},
            "results": [],
        }

    monkeypatch.setattr(main, "compile_whole_norscode", fake_compile)
    monkeypatch.setattr(
        main,
        "run_selfhost_execution_smoke",
        lambda: {"ok": True, "expected": 12, "result": 12, "duration_ms": 1},
    )
    monkeypatch.setattr(
        main,
        "run_package_hosting_smoke",
        lambda: {"ok": True, "url": "http://127.0.0.1:1/index.json", "count": 1, "served_bytes": 42},
    )
    monkeypatch.setattr(
        main,
        "verify_bootstrap_compiler",
        lambda: {
            "ok": True,
            "machine_code_hex": "b83c00000031ff0f05",
            "elf_magic": "7f454c46",
            "executable": True,
            "ran": True,
            "returncode": 0,
            "skip_reason": None,
        },
    )

    payload = main.run_selfhost_bootstrap_gate(output_dir="build/selfhost-bootstrap-gate")

    assert payload["ok"] is True
    assert payload["format"] == "norscode-selfhost-bootstrap-gate-v1"
    assert payload["selfhost_whole_compile"]["passed"] == 296
    assert payload["selfhost_whole_compile"]["digest"] == "a" * 64
    assert payload["determinism_check"]["ok"] is True
    assert payload["selfhost_execution_smoke"]["result"] == 12
    assert payload["package_hosting_smoke"]["ok"] is True
    assert payload["native_bootstrap_verify"]["ok"] is True


def test_ci_bootstrap_lane_runs_gate_and_workflow_policy(monkeypatch):
    monkeypatch.setattr(
        main,
        "run_selfhost_bootstrap_gate",
        lambda output_dir: {
            "ok": True,
            "selfhost_whole_compile": {"passed": 296, "total": 296},
            "native_bootstrap_verify": {"ok": True},
            "output_dir": output_dir,
        },
    )
    monkeypatch.setattr(
        main,
        "check_workflow_action_versions",
        lambda: {"ok": True, "scanned_files": 5, "issues": []},
    )

    payload = main.run_ci_bootstrap_lane(
        json_output=True,
        output_dir="build/selfhost-bootstrap-gate",
    )

    assert payload["ok"] is True
    assert payload["mode"] == "bootstrap-lane"
    assert payload["steps"]["order"] == ["selfhost_bootstrap_gate", "workflow_action_check"]
    assert payload["selfhost_bootstrap_gate"]["selfhost_whole_compile"]["passed"] == 296
