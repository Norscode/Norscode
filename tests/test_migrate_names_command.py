from __future__ import annotations

import json
import os
import subprocess
import sys
from pathlib import Path


REPO_ROOT = Path(__file__).resolve().parents[1]


def _pythonpath_env() -> dict[str, str]:
    env = os.environ.copy()
    existing = env.get("PYTHONPATH")
    env["PYTHONPATH"] = str(REPO_ROOT) if not existing else f"{REPO_ROOT}{os.pathsep}{existing}"
    return env


def _seed_legacy_project(root: Path) -> None:
    (root / "norsklang.toml").write_text('[project]\nname = "demo"\n', encoding="utf-8")
    (root / "norsklang.lock").write_text("legacy-lock\n", encoding="utf-8")
    legacy_dir = root / ".norsklang"
    legacy_dir.mkdir()
    (legacy_dir / "data.txt").write_text("payload\n", encoding="utf-8")


def _run_migrate_names(root: Path, *args: str) -> subprocess.CompletedProcess[str]:
    return subprocess.run(
        [sys.executable, "-m", "norcode", "migrate-names", *args],
        cwd=str(root),
        env=_pythonpath_env(),
        text=True,
        capture_output=True,
        check=False,
    )


def test_migrate_names_check_detects_legacy_tree(tmp_path: Path) -> None:
    _seed_legacy_project(tmp_path)

    result = _run_migrate_names(tmp_path, "--check", "--json")
    assert result.returncode != 0, result.stderr

    payload = json.loads(result.stdout)
    assert payload["needs_migration"] is True
    assert payload["applied"] is False
    assert payload["cleanup"] is False


def test_migrate_names_apply_and_cleanup(tmp_path: Path) -> None:
    _seed_legacy_project(tmp_path)

    result = _run_migrate_names(tmp_path, "--apply", "--cleanup", "--json")
    assert result.returncode == 0, result.stderr

    payload = json.loads(result.stdout)
    assert payload["applied"] is True
    assert payload["cleanup"] is True
    assert payload["needs_migration"] is False
    assert (tmp_path / "norcode.toml").exists()
    assert (tmp_path / "norcode.lock").exists()
    assert (tmp_path / ".norcode" / "data.txt").exists()
    assert not (tmp_path / "norsklang.toml").exists()
    assert not (tmp_path / "norsklang.lock").exists()
    assert not (tmp_path / ".norsklang").exists()
