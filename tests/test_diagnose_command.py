from __future__ import annotations

import json
import subprocess
import sys
from pathlib import Path


def test_norcode_diagnose_json_contract() -> None:
    completed = subprocess.run(
        [sys.executable, "-m", "norcode", "diagnose", "--json"],
        cwd=Path(__file__).resolve().parents[1],
        text=True,
        capture_output=True,
        check=True,
    )

    payload = json.loads(completed.stdout)
    assert payload["ok"] is True
    assert "root" in payload
    assert "project_root" in payload
    assert "paths" in payload
    assert "stdlib_roots" in payload
    assert "dependency_count" in payload
    assert "test_count" in payload
    assert "git" in payload


def test_norcode_diagnose_human_output() -> None:
    completed = subprocess.run(
        [sys.executable, "-m", "norcode", "diagnose"],
        cwd=Path(__file__).resolve().parents[1],
        text=True,
        capture_output=True,
        check=True,
    )

    assert "Diagnose OK: ja" in completed.stdout
    assert "Stdlib-roots:" in completed.stdout
