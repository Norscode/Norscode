"""Shared project diagnostics helpers."""

from __future__ import annotations

import re
import subprocess
import urllib.parse
from pathlib import Path

from compiler.toml_compat import loads as toml_loads


def _find_config(root: Path) -> Path | None:
    for name in ("norcode.toml", "norscode.toml"):
        for d in [root, *root.parents]:
            p = d / name
            if p.exists():
                return p
    return None


def _stdlib_roots(root: Path, config_data: dict) -> list[Path]:
    paths_data = config_data.get("paths", {}) if isinstance(config_data, dict) else {}
    stdlib_hint = paths_data.get("stdlib") if isinstance(paths_data, dict) else None
    candidates = []
    if stdlib_hint:
        candidates.append(root / stdlib_hint)
    for std_dir in ["std", "stdlib", "../std", "../stdlib"]:
        candidates.append(root / std_dir)
    return [p for p in candidates if p.exists()]


def _module_candidates_web(stdlib_roots: list[Path]) -> list[Path]:
    return [r / "web.no" for r in stdlib_roots] + [r / "web" / "__init__.no" for r in stdlib_roots]


def _dependency_map(config_data: dict) -> dict:
    return config_data.get("dependencies", {}) if isinstance(config_data, dict) else {}


def get_current_git_revision() -> str | None:
    try:
        completed = subprocess.run(
            ["git", "rev-parse", "HEAD"],
            check=True,
            capture_output=True,
            text=True,
        )
    except Exception:
        return None
    return completed.stdout.strip() or None


def get_current_git_branch() -> str | None:
    try:
        completed = subprocess.run(
            ["git", "branch", "--show-current"],
            check=True,
            capture_output=True,
            text=True,
        )
    except Exception:
        return None
    return completed.stdout.strip() or None


def get_current_git_dirty_state() -> bool | None:
    try:
        completed = subprocess.run(
            ["git", "status", "--porcelain"],
            check=True,
            capture_output=True,
            text=True,
        )
    except Exception:
        return None
    return bool(completed.stdout.strip())


def to_short_git_revision(revision: str | None) -> str | None:
    if not revision:
        return None
    text = str(revision).strip()
    return text[:12] if len(text) > 12 else text


def get_current_git_exact_tag() -> str | None:
    try:
        completed = subprocess.run(
            ["git", "describe", "--tags", "--exact-match"],
            check=True,
            capture_output=True,
            text=True,
        )
    except Exception:
        return None
    return completed.stdout.strip() or None


def get_current_git_origin_url() -> str | None:
    try:
        completed = subprocess.run(
            ["git", "config", "--get", "remote.origin.url"],
            check=True,
            capture_output=True,
            text=True,
        )
    except Exception:
        return None
    return completed.stdout.strip() or None


def get_git_remote_host(remote_url: str | None) -> str | None:
    if not remote_url:
        return None
    text = str(remote_url).strip()
    if not text:
        return None
    if text.startswith("git@"):
        text = text.split(":", 1)[0].split("@", 1)[1]
        return text or None
    parsed = urllib.parse.urlparse(text)
    if parsed.hostname:
        return parsed.hostname
    if "/" in text and ":" in text:
        host_part = text.split(":", 1)[0]
        if "@" in host_part:
            host_part = host_part.split("@", 1)[1]
        return host_part or None
    return None


def get_git_remote_repo_slug(remote_url: str | None) -> str | None:
    if not remote_url:
        return None
    text = str(remote_url).strip()
    if not text:
        return None
    if text.endswith(".git"):
        text = text[:-4]
    parsed = urllib.parse.urlparse(text)
    if parsed.scheme and parsed.path:
        path = parsed.path.lstrip("/")
        return path or None
    if ":" in text and "@" in text:
        _, path = text.rsplit(":", 1)
        path = path.lstrip("/")
        return path or None
    if "/" in text:
        return text.split("://", 1)[-1].split("/", 1)[-1].strip("/") or None
    return None


def get_git_remote_protocol(remote_url: str | None) -> str:
    if not remote_url:
        return "unknown"
    text = str(remote_url).strip()
    if text.startswith("git@"):
        return "ssh"
    parsed = urllib.parse.urlparse(text)
    if parsed.scheme:
        return parsed.scheme
    return "unknown"


def split_repo_slug(slug: str | None) -> tuple[str | None, str | None]:
    if not slug:
        return None, None
    text = str(slug).strip().strip("/")
    if not text or "/" not in text:
        return None, None
    owner, name = text.split("/", 1)
    owner = owner.strip() or None
    name = name.strip() or None
    return owner, name


def is_github_host(host: str | None) -> bool:
    return bool(host and "github" in str(host).lower())


def is_gitlab_host(host: str | None) -> bool:
    return bool(host and "gitlab" in str(host).lower())


def is_bitbucket_host(host: str | None) -> bool:
    return bool(host and "bitbucket" in str(host).lower())


def get_git_remote_provider(host: str | None) -> str:
    if is_github_host(host):
        return "github"
    if is_gitlab_host(host):
        return "gitlab"
    if is_bitbucket_host(host):
        return "bitbucket"
    return "unknown"


def get_source_revision_url(remote_host: str | None, repo_slug: str | None, revision: str | None) -> str | None:
    if not remote_host or not repo_slug or not revision:
        return None
    if is_github_host(remote_host):
        return f"https://{remote_host}/{repo_slug}/commit/{revision}"
    if is_gitlab_host(remote_host):
        return f"https://{remote_host}/{repo_slug}/-/commit/{revision}"
    if is_bitbucket_host(remote_host):
        return f"https://{remote_host}/{repo_slug}/commits/{revision}"
    return None


def get_source_ref_url(remote_host: str | None, repo_slug: str | None, source_ref: str | None) -> str | None:
    if not remote_host or not repo_slug or not source_ref:
        return None
    if is_github_host(remote_host):
        return f"https://{remote_host}/{repo_slug}/tree/{source_ref}"
    if is_gitlab_host(remote_host):
        return f"https://{remote_host}/{repo_slug}/-/tree/{source_ref}"
    if is_bitbucket_host(remote_host):
        return f"https://{remote_host}/{repo_slug}/src/{source_ref}"
    return None


def get_source_repo_url(remote_host: str | None, repo_slug: str | None) -> str | None:
    if not remote_host or not repo_slug:
        return None
    return f"https://{remote_host}/{repo_slug}"


def run_diagnostics(path: str | None = None) -> dict:
    target = Path(path or ".").expanduser().resolve()
    root = target.parent if target.is_file() else target

    config_path = _find_config(root)
    config_data: dict = {}
    if config_path is not None:
        try:
            config_data = toml_loads(config_path.read_text(encoding="utf-8"))
        except Exception:
            config_data = {}

    project_data = config_data.get("project", {}) if isinstance(config_data, dict) else {}
    paths_data = config_data.get("paths", {}) if isinstance(config_data, dict) else {}
    project_entry = project_data.get("entry") if isinstance(project_data, dict) else None
    project_name = project_data.get("name") if isinstance(project_data, dict) else None

    stdlib_roots = _stdlib_roots(root, config_data)
    web_candidates = _module_candidates_web(stdlib_roots)
    std_resolves = any(p.exists() for p in web_candidates)
    dep_map = _dependency_map(config_data)

    test_dir = root / "tests"
    tests = sorted(test_dir.rglob("test_*.no")) if test_dir.exists() else []

    return {
        "ok": True,
        "cwd": str(Path.cwd()),
        "target": str(target),
        "root": str(root),
        "project_root": str(config_path.parent) if config_path is not None else None,
        "config_path": str(config_path) if config_path is not None else None,
        "project_name": project_name,
        "project_entry": project_entry,
        "paths": {
            "source": paths_data.get("source") if isinstance(paths_data, dict) else None,
            "stdlib": paths_data.get("stdlib") if isinstance(paths_data, dict) else None,
            "build": paths_data.get("build") if isinstance(paths_data, dict) else None,
        },
        "stdlib_roots": [str(p) for p in stdlib_roots],
        "stdlib_resolves_web": std_resolves,
        "dependency_count": len(dep_map),
        "dependencies": sorted(dep_map.keys()),
        "test_count": len(tests),
        "tests": [str(p) for p in tests[:10]],
        "git": {
            "revision": get_current_git_revision(),
            "branch": get_current_git_branch(),
            "dirty": get_current_git_dirty_state(),
        },
    }
