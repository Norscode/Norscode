from __future__ import annotations

import json
import subprocess
from pathlib import Path


def test_scaffold_api_command_creates_project(tmp_path: Path) -> None:
    target = tmp_path / "butikk-api"
    result = subprocess.run(
        ["python3", "-m", "norcode", "scaffold-api", str(target), "--name", "butikk_api", "--json"],
        text=True,
        capture_output=True,
        check=False,
    )

    assert result.returncode == 0, result.stderr
    payload = json.loads(result.stdout)
    assert payload["ok"] is True
    assert payload["project_name"] == "butikk_api"
    assert (target / "app.no").exists()
    assert (target / "norcode.toml").exists()
    assert (target / "README.md").exists()


def test_scaffold_api_help_is_modular() -> None:
    result = subprocess.run(
        ["python3", "-m", "norcode", "scaffold-api", "--help"],
        text=True,
        capture_output=True,
        check=False,
    )
    assert result.returncode == 0, result.stderr
    assert "Målmappe for prosjektet" in result.stdout
