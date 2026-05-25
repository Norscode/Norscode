from __future__ import annotations

import hashlib
import shutil
import sys
from pathlib import Path


PROJECT_CONFIG_NAME = "norcode.toml"
LEGACY_PROJECT_CONFIG_NAME = "norsklang.toml"
PROJECT_CONFIG_NAMES = (PROJECT_CONFIG_NAME, LEGACY_PROJECT_CONFIG_NAME)
LOCKFILE_NAME = "norcode.lock"
LEGACY_LOCKFILE_NAME = "norsklang.lock"
LOCKFILE_NAMES = (LOCKFILE_NAME, LEGACY_LOCKFILE_NAME)

_LEGACY_WARNINGS_EMITTED: set[str] = set()


def _warn_legacy_once(key: str, message: str) -> None:
    if key in _LEGACY_WARNINGS_EMITTED:
        return
    _LEGACY_WARNINGS_EMITTED.add(key)
    print(message, file=sys.stderr)


def _project_config_display_names() -> str:
    return " / ".join(PROJECT_CONFIG_NAMES)


def _find_existing_project_config_in_dir(base: Path) -> Path | None:
    for name in PROJECT_CONFIG_NAMES:
        candidate = base / name
        if candidate.exists():
            return candidate
    return None


def _find_project_config(start_dir: Path | None = None) -> Path:
    base = (start_dir or Path.cwd()).resolve()
    for candidate_dir in (base, *base.parents):
        candidate = _find_existing_project_config_in_dir(candidate_dir)
        if candidate is not None:
            if candidate.name == LEGACY_PROJECT_CONFIG_NAME:
                _warn_legacy_once(
                    "legacy-config",
                    "Merk: bruker legacy konfig 'norsklang.toml'. Bytt til 'norcode.toml'.",
                )
            return candidate
    raise RuntimeError(
        f"Fant ikke {_project_config_display_names()} i denne mappen eller overliggende mapper"
    )


def _hash_file(path: Path) -> str:
    digest = hashlib.sha256()
    with path.open("rb") as handle:
        for chunk in iter(lambda: handle.read(65536), b""):
            digest.update(chunk)
    return digest.hexdigest()


def _hash_directory(path: Path) -> str:
    digest = hashlib.sha256()
    for child in sorted(path.rglob("*")):
        if child.is_file():
            digest.update(child.relative_to(path).as_posix().encode("utf-8"))
            digest.update(b"\0")
            digest.update(_hash_file(child).encode("utf-8"))
    return digest.hexdigest()


def _record_action(actions: list[dict], kind: str, legacy: Path, primary: Path, status: str, reason: str | None = None) -> None:
    item: dict[str, object] = {
        "kind": kind,
        "legacy": str(legacy),
        "primary": str(primary),
        "status": status,
    }
    if reason:
        item["reason"] = reason
    actions.append(item)


def _record_file_migration(
    actions: list[dict],
    project_dir: Path,
    legacy_name: str,
    primary_name: str,
    apply_changes: bool,
) -> tuple[Path, Path, str]:
    legacy = (project_dir / legacy_name).resolve()
    primary = (project_dir / primary_name).resolve()
    if not legacy.exists():
        _record_action(actions, "file", legacy, primary, "skipped", "legacy mangler")
        return legacy, primary, "skipped"
    if primary.exists():
        _record_action(actions, "file", legacy, primary, "skipped", "primary finnes allerede")
        return legacy, primary, "skipped"
    status = "copied" if apply_changes else "planned"
    if apply_changes:
        shutil.copy2(legacy, primary)
    _record_action(actions, "file", legacy, primary, status)
    return legacy, primary, status


def _record_dir_migration(
    actions: list[dict],
    project_dir: Path,
    legacy_name: str,
    primary_name: str,
    apply_changes: bool,
) -> tuple[Path, Path, str]:
    legacy = (project_dir / legacy_name).resolve()
    primary = (project_dir / primary_name).resolve()
    if not legacy.exists():
        _record_action(actions, "dir", legacy, primary, "skipped", "legacy mangler")
        return legacy, primary, "skipped"
    if primary.exists():
        _record_action(actions, "dir", legacy, primary, "skipped", "primary finnes allerede")
        return legacy, primary, "skipped"
    status = "copied" if apply_changes else "planned"
    if apply_changes:
        shutil.copytree(legacy, primary)
    _record_action(actions, "dir", legacy, primary, status)
    return legacy, primary, status


def migrate_names(
    apply_changes: bool = False,
    cleanup_legacy: bool = False,
    start_dir: Path | None = None,
) -> dict:
    config_path = _find_project_config(start_dir=start_dir)
    project_dir = config_path.parent.resolve()
    actions: list[dict] = []

    migrated_resources = [
        ("file", *_record_file_migration(actions, project_dir, LEGACY_PROJECT_CONFIG_NAME, PROJECT_CONFIG_NAME, apply_changes)),
        ("file", *_record_file_migration(actions, project_dir, LEGACY_LOCKFILE_NAME, LOCKFILE_NAME, apply_changes)),
        ("dir", *_record_dir_migration(actions, project_dir, ".norsklang", ".norcode", apply_changes)),
    ]

    if cleanup_legacy:
        for kind, legacy, primary, status in migrated_resources:
            if not legacy.exists():
                continue
            primary_ready = primary.exists() or status == "copied"
            if not primary_ready:
                _record_action(actions, f"cleanup-{kind}", legacy, primary, "skipped", "primary mangler")
                continue
            if kind == "dir":
                if not legacy.is_dir() or not primary.is_dir():
                    _record_action(actions, f"cleanup-{kind}", legacy, primary, "skipped", "type mismatch")
                    continue
                if _hash_directory(legacy) != _hash_directory(primary):
                    _record_action(actions, f"cleanup-{kind}", legacy, primary, "skipped", "primary avviker")
                    continue
            else:
                if not legacy.is_file() or not primary.is_file():
                    _record_action(actions, f"cleanup-{kind}", legacy, primary, "skipped", "type mismatch")
                    continue
                if _hash_file(legacy) != _hash_file(primary):
                    _record_action(actions, f"cleanup-{kind}", legacy, primary, "skipped", "primary avviker")
                    continue
            if apply_changes:
                if kind == "dir":
                    shutil.rmtree(legacy)
                else:
                    legacy.unlink()
                cleanup_status = "removed"
            else:
                cleanup_status = "planned-remove"
            _record_action(actions, f"cleanup-{kind}", legacy, primary, cleanup_status)

    copied = sum(1 for a in actions if a["status"] == "copied")
    planned = sum(1 for a in actions if a["status"] == "planned")
    skipped = sum(1 for a in actions if a["status"] == "skipped")
    removed = sum(1 for a in actions if a["status"] == "removed")
    planned_remove = sum(1 for a in actions if a["status"] == "planned-remove")
    return {
        "project_dir": str(project_dir),
        "applied": apply_changes,
        "cleanup": cleanup_legacy,
        "copied": copied,
        "planned": planned,
        "removed": removed,
        "planned_remove": planned_remove,
        "needs_migration": (planned + planned_remove) > 0,
        "skipped": skipped,
        "actions": actions,
    }
