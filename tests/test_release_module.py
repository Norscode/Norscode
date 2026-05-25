from __future__ import annotations

import json
import os
import subprocess
import tempfile
import textwrap
from pathlib import Path


ROOT = Path(__file__).resolve().parents[1]


def _run(args: list[str], cwd: Path) -> str:
    env = os.environ.copy()
    env["PYTHONPATH"] = str(ROOT)
    result = subprocess.run(
        ["python3", "-m", "norcode", *args],
        cwd=str(cwd),
        env=env,
        check=True,
        stdout=subprocess.PIPE,
        stderr=subprocess.PIPE,
        text=True,
    )
    return result.stdout


def test_release_module_bumps_version_and_changelog() -> None:
    with tempfile.TemporaryDirectory(prefix="norscode-release-module-") as tmpdir:
        project = Path(tmpdir)
        (project / "pyproject.toml").write_text(
            textwrap.dedent(
                """
                [project]
                name = "demo"
                version = "1.2.3"
                """
            ).strip() + "\n",
            encoding="utf-8",
        )
        (project / "CHANGELOG.md").write_text("# Changelog\n\n## [Unreleased]\n\n", encoding="utf-8")

        out = _run(["release", "--version", "1.2.4", "--date", "2026-01-02", "--json"], project)
        payload = json.loads(out)
        assert payload["old_version"] == "1.2.3"
        assert payload["new_version"] == "1.2.4"
        assert payload["changed_pyproject"] is True
        assert payload["changed_changelog"] is True

        pyproject = (project / "pyproject.toml").read_text(encoding="utf-8")
        changelog = (project / "CHANGELOG.md").read_text(encoding="utf-8")
        assert 'version = "1.2.4"' in pyproject
        assert "## [1.2.4] - 2026-01-02" in changelog
