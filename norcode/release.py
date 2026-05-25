from __future__ import annotations

import datetime as dt
import re
from pathlib import Path


PROJECT_CONFIG_NAME = "pyproject.toml"
CHANGELOG_NAME = "CHANGELOG.md"
SEMVER_RE = re.compile(r"^\d+\.\d+\.\d+$")


def _find_pyproject(start_dir: Path | None = None) -> Path:
    base = (start_dir or Path.cwd()).resolve()
    for candidate_dir in (base, *base.parents):
        candidate = candidate_dir / PROJECT_CONFIG_NAME
        if candidate.exists():
            return candidate
    raise RuntimeError(f"Fant ikke {PROJECT_CONFIG_NAME} i denne mappen eller overliggende mapper")


def _parse_toml_string(raw: str) -> str | None:
    value = raw.strip()
    if len(value) >= 2 and value[0] == '"' and value[-1] == '"':
        return value[1:-1]
    return None


def _parse_project_version_from_pyproject(path: Path) -> str:
    current_section = ""
    for line in path.read_text(encoding="utf-8").splitlines():
        stripped = line.strip()
        if not stripped or stripped.startswith("#"):
            continue
        if stripped.startswith("[") and stripped.endswith("]"):
            current_section = stripped[1:-1].strip()
            continue
        if current_section == "project" and "=" in stripped:
            key, value = stripped.split("=", 1)
            if key.strip() == "version":
                parsed = _parse_toml_string(value)
                if parsed:
                    return parsed
    raise RuntimeError(f"Fant ikke [project].version i {path}")


def _set_project_version_in_pyproject(path: Path, new_version: str, dry_run: bool = False) -> bool:
    lines = path.read_text(encoding="utf-8").splitlines()
    current_section = ""
    changed = False

    for idx, line in enumerate(lines):
        stripped = line.strip()
        if stripped.startswith("[") and stripped.endswith("]"):
            current_section = stripped[1:-1].strip()
            continue
        if current_section == "project" and "=" in stripped:
            key, _value = stripped.split("=", 1)
            if key.strip() == "version":
                rendered = f'version = "{new_version}"'
                if stripped != rendered:
                    lines[idx] = rendered
                    changed = True
                break

    if changed and not dry_run:
        path.write_text("\n".join(lines) + "\n", encoding="utf-8")
    return changed


def _next_semver(version: str, bump: str) -> str:
    if not SEMVER_RE.match(version):
        raise RuntimeError(f"Ugyldig semver i pyproject: {version}")
    major, minor, patch = (int(part) for part in version.split("."))
    if bump == "major":
        return f"{major + 1}.0.0"
    if bump == "minor":
        return f"{major}.{minor + 1}.0"
    return f"{major}.{minor}.{patch + 1}"


def _upsert_changelog_release(changelog_path: Path, version: str, release_date: str, dry_run: bool = False) -> bool:
    if not changelog_path.exists():
        baseline = (
            "# Changelog\n\n"
            "## [Unreleased]\n\n"
            f"## [{version}] - {release_date}\n\n"
            "### Endret\n"
            f"- Versjonsbump til `{version}`.\n"
        )
        if not dry_run:
            changelog_path.write_text(baseline, encoding="utf-8")
        return True

    lines = changelog_path.read_text(encoding="utf-8").splitlines()
    if any(line.startswith(f"## [{version}]") for line in lines):
        return False

    unreleased_idx = next((i for i, line in enumerate(lines) if line.strip().lower() == "## [unreleased]"), None)
    if unreleased_idx is not None:
        insert_at = len(lines)
        for i in range(unreleased_idx + 1, len(lines)):
            if lines[i].startswith("## "):
                insert_at = i
                break
    else:
        insert_at = next((i for i, line in enumerate(lines) if line.startswith("## ")), len(lines))

    new_block = [
        "",
        f"## [{version}] - {release_date}",
        "",
        "### Endret",
        f"- Versjonsbump til `{version}`.",
    ]
    updated = lines[:insert_at] + new_block + lines[insert_at:]
    if not dry_run:
        changelog_path.write_text("\n".join(updated).rstrip() + "\n", encoding="utf-8")
    return True


def prepare_release(version: str | None = None, bump: str = "patch", dry_run: bool = False, release_date: str | None = None):
    pyproject_path = _find_pyproject()
    old_version = _parse_project_version_from_pyproject(pyproject_path)

    if version is not None:
        if not SEMVER_RE.match(version):
            raise RuntimeError(f"Ugyldig versjon: {version} (forventer MAJOR.MINOR.PATCH)")
        new_version = version
    else:
        new_version = _next_semver(old_version, bump)

    if new_version == old_version:
        raise RuntimeError(f"Versjon er allerede {old_version}")

    today = release_date or dt.date.today().isoformat()
    if not re.match(r"^\d{4}-\d{2}-\d{2}$", today):
        raise RuntimeError(f"Ugyldig datoformat: {today} (forventer YYYY-MM-DD)")

    pyproject_changed = _set_project_version_in_pyproject(pyproject_path, new_version, dry_run=dry_run)
    changelog_path = pyproject_path.parent / CHANGELOG_NAME
    changelog_changed = _upsert_changelog_release(changelog_path, new_version, today, dry_run=dry_run)

    return {
        "pyproject": str(pyproject_path),
        "changelog": str(changelog_path),
        "old_version": old_version,
        "new_version": new_version,
        "release_date": today,
        "dry_run": dry_run,
        "changed_pyproject": pyproject_changed,
        "changed_changelog": changelog_changed,
    }
