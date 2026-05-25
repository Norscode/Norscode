from __future__ import annotations

import datetime as dt
import hashlib
import json
import os
import re
import shutil
import subprocess
import tarfile
import threading
import urllib.parse
import urllib.request
import zipfile
import sys
from http.server import BaseHTTPRequestHandler, ThreadingHTTPServer
from pathlib import Path

from compiler.toml_compat import loads as toml_loads


PROJECT_CONFIG_NAME = "norcode.toml"
LEGACY_PROJECT_CONFIG_NAME = "norsklang.toml"
PROJECT_CONFIG_NAMES = (PROJECT_CONFIG_NAME, LEGACY_PROJECT_CONFIG_NAME)
LOCKFILE_NAME = "norcode.lock"
LEGACY_LOCKFILE_NAME = "norsklang.lock"
LOCKFILE_NAMES = (LOCKFILE_NAME, LEGACY_LOCKFILE_NAME)
REMOTE_REGISTRY_CACHE = ".norcode/registry/remote_index.json"
LEGACY_REMOTE_REGISTRY_CACHE = ".norsklang/registry/remote_index.json"
SEMVER_RE = re.compile(r"^\d+\.\d+\.\d+$")

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


def _load_toml(path: Path) -> dict:
    try:
        return toml_loads(path.read_text(encoding="utf-8"))
    except Exception as exc:
        raise RuntimeError(f"Kunne ikke lese TOML {path}: {exc}") from exc


def _parse_toml_string(raw: str) -> str | None:
    value = raw.strip()
    if len(value) >= 2 and value[0] == '"' and value[-1] == '"':
        return value[1:-1]
    return None


def _parse_project_name_from_toml(path: Path) -> str | None:
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
            if key.strip() == "name":
                return _parse_toml_string(value)
    return None


def _load_security_policy(config_path: Path) -> dict:
    data = _load_toml(config_path)
    sec = data.get("security", {})
    if not isinstance(sec, dict):
        sec = {}

    def to_set(key: str) -> set[str]:
        raw = sec.get(key, [])
        if isinstance(raw, list):
            return {str(x).strip().lower() for x in raw if isinstance(x, str) and str(x).strip()}
        return set()

    return {
        "trusted_git_hosts": to_set("trusted_git_hosts"),
        "trusted_url_hosts": to_set("trusted_url_hosts"),
        "trusted_registry_sha256": (
            str(sec.get("trusted_registry_sha256")).strip().lower()
            if isinstance(sec.get("trusted_registry_sha256"), str) and str(sec.get("trusted_registry_sha256")).strip()
            else None
        ),
    }


def _hash_file(path: Path) -> str:
    h = hashlib.sha256()
    with path.open("rb") as f:
        while True:
            chunk = f.read(1024 * 1024)
            if not chunk:
                break
            h.update(chunk)
    return h.hexdigest()


def _hash_directory(path: Path) -> str:
    h = hashlib.sha256()
    files = [p for p in sorted(path.rglob("*")) if p.is_file() and ".git" not in p.parts]
    for f in files:
        rel = str(f.relative_to(path)).replace("\\", "/")
        h.update(rel.encode("utf-8"))
        h.update(b"\0")
        with f.open("rb") as fd:
            while True:
                chunk = fd.read(1024 * 1024)
                if not chunk:
                    break
                h.update(chunk)
        h.update(b"\0")
    return h.hexdigest()


def _cache_base_dir(project_dir: Path) -> Path:
    primary = (project_dir / ".norcode" / "cache").resolve()
    legacy = (project_dir / ".norsklang" / "cache").resolve()
    if primary.exists() or not legacy.exists():
        return primary
    _warn_legacy_once(
        "legacy-cache",
        "Merk: bruker legacy cache '.norsklang/cache'. Migrer til '.norcode/cache'.",
    )
    return legacy


def _safe_extract_tar(archive_path: Path, dest_dir: Path):
    with tarfile.open(archive_path) as tar:
        for member in tar.getmembers():
            member_path = (dest_dir / member.name).resolve()
            if not str(member_path).startswith(str(dest_dir.resolve())):
                raise RuntimeError(f"Utrygg tar-oppføring blokkert: {member.name}")
        tar.extractall(path=dest_dir)


def _safe_extract_zip(archive_path: Path, dest_dir: Path):
    with zipfile.ZipFile(archive_path) as zf:
        for name in zf.namelist():
            target = (dest_dir / name).resolve()
            if not str(target).startswith(str(dest_dir.resolve())):
                raise RuntimeError(f"Utrygg zip-oppføring blokkert: {name}")
        zf.extractall(path=dest_dir)


def _find_cached_package_root(base_dir: Path) -> Path:
    direct = _find_existing_project_config_in_dir(base_dir)
    if direct is not None:
        return base_dir

    candidates = []
    for cfg_name in PROJECT_CONFIG_NAMES:
        candidates.extend(base_dir.glob(f"**/{cfg_name}"))
    candidates = sorted(candidates)
    if not candidates:
        raise RuntimeError(f"Fant ikke {_project_config_display_names()} i cache: {base_dir}")
    if len(candidates) > 1:
        candidates.sort(key=lambda p: len(p.relative_to(base_dir).parts))
    return candidates[0].parent


def _resolve_package_dir(package_path: str) -> tuple[Path, Path]:
    path = Path(package_path).expanduser()
    if path.is_absolute():
        path = path.resolve()
    else:
        path = (Path.cwd() / path).resolve()

    if path.is_file():
        if path.name not in PROJECT_CONFIG_NAMES:
            raise RuntimeError(f"Sti peker til fil, men ikke {_project_config_display_names()}: {path}")
        package_dir = path.parent
        package_config = path
    else:
        package_dir = path
        package_config = _find_existing_project_config_in_dir(package_dir) or (package_dir / PROJECT_CONFIG_NAME)

    if not package_dir.exists():
        raise RuntimeError(f"Fant ikke pakkesti: {package_dir}")
    if not package_dir.is_dir():
        raise RuntimeError(f"Pakkesti må være mappe: {package_dir}")
    if not package_config.exists():
        raise RuntimeError(f"Fant ikke pakkekonfig: {package_config}")

    return package_dir, package_config


def _fetch_git_to_cache(project_dir: Path, package_name: str, git_url: str, git_ref: str | None, refresh: bool) -> Path:
    cache_dir = _cache_base_dir(project_dir) / "git"
    cache_dir.mkdir(parents=True, exist_ok=True)
    key = f"{git_url}@{git_ref or 'HEAD'}"
    short = hashlib.sha1(key.encode("utf-8")).hexdigest()[:12]
    target = cache_dir / f"{package_name}-{short}"

    if target.exists() and refresh:
        shutil.rmtree(target)

    if not target.exists():
        subprocess.run(["git", "clone", git_url, str(target)], check=True)
    if git_ref:
        subprocess.run(["git", "-C", str(target), "fetch", "--all", "--tags"], check=True)
        subprocess.run(["git", "-C", str(target), "checkout", git_ref], check=True)

    return _find_cached_package_root(target)


def _fetch_url_to_cache(
    project_dir: Path,
    package_name: str,
    source_url: str,
    refresh: bool,
    expected_sha256: str | None = None,
) -> Path:
    cache_dir = _cache_base_dir(project_dir) / "url"
    cache_dir.mkdir(parents=True, exist_ok=True)
    short = hashlib.sha1(source_url.encode("utf-8")).hexdigest()[:12]
    target = cache_dir / f"{package_name}-{short}"

    if target.exists() and refresh:
        shutil.rmtree(target)

    archives_dir = target / "archives"
    extract_dir = target / "src"
    if not target.exists():
        archives_dir.mkdir(parents=True, exist_ok=True)
        extract_dir.mkdir(parents=True, exist_ok=True)

        filename = Path(source_url.split("?")[0]).name or "package.tar.gz"
        archive_path = archives_dir / filename
        urllib.request.urlretrieve(source_url, archive_path)
        archive_digest = _hash_file(archive_path)
        if expected_sha256 is not None and archive_digest.lower() != expected_sha256.lower():
            raise RuntimeError(
                f"SHA256 mismatch for {source_url}: expected {expected_sha256.lower()} got {archive_digest.lower()}"
            )

        lower_name = filename.lower()
        if lower_name.endswith(".zip"):
            _safe_extract_zip(archive_path, extract_dir)
        elif lower_name.endswith((".tar.gz", ".tgz", ".tar", ".tar.bz2", ".tbz2", ".tar.xz", ".txz")):
            _safe_extract_tar(archive_path, extract_dir)
        else:
            raise RuntimeError(f"Ukjent arkivformat for URL-kilde: {filename}")

    return _find_cached_package_root(extract_dir)


def _is_valid_sha256(value: str) -> bool:
    return bool(re.fullmatch(r"[0-9a-fA-F]{64}", value))


def _verify_registry_integrity(config_path: Path, registry_file: Path) -> None:
    policy = _load_security_policy(config_path)
    expected = policy.get("trusted_registry_sha256")
    if expected is None:
        return

    if not _is_valid_sha256(expected):
        raise RuntimeError("Ugyldig [security].trusted_registry_sha256 i norcode.toml")

    if not registry_file.exists():
        raise RuntimeError(f"Registry-fil mangler, men trusted_registry_sha256 er satt: {registry_file}")

    actual = _hash_file(registry_file).lower()
    if actual != expected:
        raise RuntimeError(
            f"Registry-integritet feilet: expected {expected}, actual {actual} ({registry_file})"
        )


def _extract_git_host(git_url: str) -> str | None:
    raw = git_url.strip()
    if not raw:
        return None
    parsed = urllib.parse.urlparse(raw)
    if parsed.scheme and parsed.hostname:
        return parsed.hostname.lower()
    if "@" in raw and ":" in raw and "://" not in raw:
        after_at = raw.split("@", 1)[1]
        host = after_at.split(":", 1)[0].strip().lower()
        return host or None
    return None


def _extract_url_host(url: str) -> str | None:
    parsed = urllib.parse.urlparse(url)
    if parsed.scheme == "file":
        return "file"
    if parsed.hostname:
        return parsed.hostname.lower()
    return None


def _host_matches(host: str, allowlist: set[str]) -> bool:
    if host in allowlist:
        return True
    for allowed in allowlist:
        if allowed.startswith("*.") and host.endswith(allowed[1:]):
            return True
    return False


def _enforce_trusted_source(kind: str, source: str, security_policy: dict, allow_untrusted: bool = False) -> None:
    if allow_untrusted:
        return

    if kind == "git":
        allowlist = security_policy.get("trusted_git_hosts", set())
        if not allowlist:
            return
        host = _extract_git_host(source)
        if host is None:
            local_path = Path(source).expanduser()
            if local_path.exists():
                return
            raise RuntimeError(f"Git-kilde kan ikke verifiseres mot trusted hosts: {source}")
        if not _host_matches(host, allowlist):
            raise RuntimeError(f"Git-host ikke tillatt av security policy: {host}")
        return

    if kind == "url":
        allowlist = security_policy.get("trusted_url_hosts", set())
        if not allowlist:
            return
        host = _extract_url_host(source)
        if host is None:
            raise RuntimeError(f"URL-kilde kan ikke verifiseres mot trusted hosts: {source}")
        if host == "file":
            return
        if not _host_matches(host, allowlist):
            raise RuntimeError(f"URL-host ikke tillatt av security policy: {host}")
        return


def _normalize_registry_path(raw_path: str, relative_to: Path) -> Path:
    path = Path(raw_path).expanduser()
    if path.is_absolute():
        return path.resolve()
    return (relative_to / path).resolve()


def _semver_key(version: str) -> tuple[int, int, int]:
    if not SEMVER_RE.match(version):
        return (-1, -1, -1)
    a, b, c = version.split(".")
    return int(a), int(b), int(c)


def _parse_registry_entry(name: str, value: object) -> dict | None:
    if isinstance(value, str):
        return {"kind": "path", "path": value, "description": None, "version": None, "name": name}

    if not isinstance(value, dict):
        return None

    description = value.get("description")
    version = value.get("version")
    if isinstance(value.get("path"), str):
        return {
            "kind": "path",
            "path": value["path"],
            "description": description,
            "version": version if isinstance(version, str) else None,
            "name": name,
        }
    if isinstance(value.get("git"), str):
        return {
            "kind": "git",
            "git": value["git"],
            "ref": value.get("ref") if isinstance(value.get("ref"), str) else None,
            "description": description,
            "version": version if isinstance(version, str) else None,
            "name": name,
        }
    if isinstance(value.get("url"), str):
        return {
            "kind": "url",
            "url": value["url"],
            "description": description,
            "version": version if isinstance(version, str) else None,
            "name": name,
        }
    return None


def _select_remote_version(package_name: str, package_obj: dict) -> dict | None:
    versions = package_obj.get("versions")
    if not isinstance(versions, dict) or not versions:
        return _parse_registry_entry(package_name, package_obj)

    latest = package_obj.get("latest")
    selected_version = latest if isinstance(latest, str) and latest in versions else None
    if selected_version is None:
        candidates = sorted((v for v in versions.keys() if isinstance(v, str)), key=_semver_key, reverse=True)
        selected_version = candidates[0] if candidates else None
    if selected_version is None:
        return None

    entry = _parse_registry_entry(package_name, versions[selected_version])
    if entry is None:
        return None
    entry["version"] = selected_version
    return entry


def _load_registry_source_text(source: str, project_dir: Path, security_policy: dict, allow_untrusted: bool) -> tuple[str, str]:
    if "://" in source:
        parsed = urllib.parse.urlparse(source)
        scheme = parsed.scheme.lower()
        if scheme in ("http", "https", "file"):
            _enforce_trusted_source("url", source, security_policy, allow_untrusted=allow_untrusted)
            with urllib.request.urlopen(source) as resp:
                text = resp.read().decode("utf-8")
            return source, text
        raise RuntimeError(f"Ukjent registry source scheme: {scheme}")

    source_path = Path(source).expanduser()
    if not source_path.is_absolute():
        source_path = (project_dir / source_path).resolve()
    if not source_path.exists():
        raise RuntimeError(f"Fant ikke registry source-fil: {source_path}")
    return str(source_path), source_path.read_text(encoding="utf-8")


def _parse_remote_registry_text(source_name: str, text: str) -> dict[str, dict]:
    data = None
    try:
        data = json.loads(text)
    except Exception:
        try:
            data = toml_loads(text)
        except Exception as exc:
            raise RuntimeError(f"Ugyldig registry-format i {source_name}: {exc}") from exc

    if not isinstance(data, dict):
        raise RuntimeError(f"Ugyldig registry-rot i {source_name}: forventet objekt/tabell")

    raw_packages = data.get("packages", {})
    if not isinstance(raw_packages, dict):
        raise RuntimeError(f"Ugyldig packages-seksjon i {source_name}")

    out: dict[str, dict] = {}
    for pkg_name, pkg_obj in raw_packages.items():
        if not isinstance(pkg_name, str):
            continue
        entry = _select_remote_version(pkg_name, pkg_obj)
        if entry is None:
            continue
        entry["source"] = source_name
        out[pkg_name] = entry
    return out


def _remote_registry_cache_path(project_dir: Path) -> Path:
    primary = (project_dir / REMOTE_REGISTRY_CACHE).resolve()
    legacy = (project_dir / LEGACY_REMOTE_REGISTRY_CACHE).resolve()
    if primary.exists() or not legacy.exists():
        return primary
    _warn_legacy_once(
        "legacy-registry-cache",
        "Merk: bruker legacy cache '.norsklang/'. Migrer til '.norcode/'.",
    )
    return legacy


def _load_remote_registry_cache(project_dir: Path) -> dict[str, dict]:
    cache_path = _remote_registry_cache_path(project_dir)
    if not cache_path.exists():
        return {}
    try:
        payload = json.loads(cache_path.read_text(encoding="utf-8"))
    except Exception:
        return {}
    entries = payload.get("entries", {})
    if not isinstance(entries, dict):
        return {}
    out = {}
    for name, obj in entries.items():
        if isinstance(name, str) and isinstance(obj, dict):
            out[name] = obj
    return out


def _extract_package_map(raw: object) -> dict[str, dict]:
    if not isinstance(raw, dict):
        return {}

    out: dict[str, dict] = {}
    for name, value in raw.items():
        if isinstance(value, str):
            out[name] = {"path": value, "description": None, "git": None, "url": None, "ref": None}
        elif isinstance(value, dict):
            raw_path = value.get("path")
            raw_git = value.get("git")
            raw_url = value.get("url")
            if isinstance(raw_path, str) or isinstance(raw_git, str) or isinstance(raw_url, str):
                out[name] = {
                    "path": raw_path if isinstance(raw_path, str) else None,
                    "git": raw_git if isinstance(raw_git, str) else None,
                    "url": raw_url if isinstance(raw_url, str) else None,
                    "ref": value.get("ref") if isinstance(value.get("ref"), str) else None,
                    "description": value.get("description"),
                }
    return out


def _read_registry_entries(project_config_path: Path) -> dict[str, dict]:
    project_dir = project_config_path.parent.resolve()
    entries: dict[str, dict] = {}

    project_toml = _load_toml(project_config_path)
    inline_registry = project_toml.get("registry", {})
    inline_packages = _extract_package_map(inline_registry.get("packages"))
    for name, meta in inline_packages.items():
        if isinstance(meta.get("path"), str):
            entries[name] = {
                "kind": "path",
                "path": _normalize_registry_path(meta["path"], project_dir),
                "description": meta.get("description"),
                "source": str(project_config_path),
            }
        elif isinstance(meta.get("git"), str):
            entries[name] = {
                "kind": "git",
                "git": meta["git"],
                "ref": meta.get("ref"),
                "description": meta.get("description"),
                "source": str(project_config_path),
            }
        elif isinstance(meta.get("url"), str):
            entries[name] = {
                "kind": "url",
                "url": meta["url"],
                "description": meta.get("description"),
                "source": str(project_config_path),
            }

    registry_file = project_dir / "packages" / "registry.toml"
    if registry_file.exists():
        _verify_registry_integrity(project_config_path, registry_file)
        registry_toml = _load_toml(registry_file)
        top_packages = _extract_package_map(registry_toml.get("packages"))
        for name, meta in top_packages.items():
            if isinstance(meta.get("path"), str):
                entries[name] = {
                    "kind": "path",
                    "path": _normalize_registry_path(meta["path"], registry_file.parent),
                    "description": meta.get("description"),
                    "source": str(registry_file),
                }
            elif isinstance(meta.get("git"), str):
                entries[name] = {
                    "kind": "git",
                    "git": meta["git"],
                    "ref": meta.get("ref"),
                    "description": meta.get("description"),
                    "source": str(registry_file),
                }
            elif isinstance(meta.get("url"), str):
                entries[name] = {
                    "kind": "url",
                    "url": meta["url"],
                    "description": meta.get("description"),
                    "source": str(registry_file),
                }

        nested_registry = registry_toml.get("registry", {})
        nested_packages = _extract_package_map(nested_registry.get("packages"))
        for name, meta in nested_packages.items():
            if isinstance(meta.get("path"), str):
                entries[name] = {
                    "kind": "path",
                    "path": _normalize_registry_path(meta["path"], registry_file.parent),
                    "description": meta.get("description"),
                    "source": str(registry_file),
                }
            elif isinstance(meta.get("git"), str):
                entries[name] = {
                    "kind": "git",
                    "git": meta["git"],
                    "ref": meta.get("ref"),
                    "description": meta.get("description"),
                    "source": str(registry_file),
                }
            elif isinstance(meta.get("url"), str):
                entries[name] = {
                    "kind": "url",
                    "url": meta["url"],
                    "description": meta.get("description"),
                    "source": str(registry_file),
                }

    remote_entries = _load_remote_registry_cache(project_dir)
    for name, meta in remote_entries.items():
        if name in entries:
            continue
        if not isinstance(meta, dict):
            continue
        kind = meta.get("kind")
        if kind == "path" and isinstance(meta.get("path"), str):
            entries[name] = {
                "kind": "path",
                "path": _normalize_registry_path(meta["path"], project_dir),
                "description": meta.get("description"),
                "source": meta.get("source", "remote-cache"),
                "version": meta.get("version"),
            }
        elif kind == "git" and isinstance(meta.get("git"), str):
            entries[name] = {
                "kind": "git",
                "git": meta["git"],
                "ref": meta.get("ref"),
                "description": meta.get("description"),
                "source": meta.get("source", "remote-cache"),
                "version": meta.get("version"),
            }
        elif kind == "url" and isinstance(meta.get("url"), str):
            entries[name] = {
                "kind": "url",
                "url": meta["url"],
                "description": meta.get("description"),
                "source": meta.get("source", "remote-cache"),
                "version": meta.get("version"),
            }

    return entries


def _to_project_relative_path(target_dir: Path, project_dir: Path) -> str:
    relative = os.path.relpath(target_dir, project_dir).replace("\\", "/")
    if relative == ".":
        return "."
    if not relative.startswith("."):
        return f"./{relative}"
    return relative


def _render_git_dependency(git_url: str, ref: str | None) -> str:
    if ref:
        return f"git+{git_url}@{ref}"
    return f"git+{git_url}"


def _render_url_dependency(url: str) -> str:
    return f"url+{url}"


def _parse_dependencies_from_toml(path: Path) -> dict[str, str]:
    data = _load_toml(path)
    deps = data.get("dependencies", {})
    if not isinstance(deps, dict):
        return {}
    out = {}
    for name, value in deps.items():
        if isinstance(name, str) and isinstance(value, str):
            out[name] = value
    return out


def _parse_git_dependency(value: str) -> tuple[str, str | None]:
    raw = value[len("git+") :]
    if "@" in raw:
        url, ref = raw.rsplit("@", 1)
        return url, ref
    return raw, None


def _resolve_path_dependency(project_dir: Path, value: str) -> Path:
    dep_path = Path(value).expanduser()
    if dep_path.is_absolute():
        return dep_path.resolve()
    return (project_dir / dep_path).resolve()


def _upsert_dependency(config_path: Path, dep_name: str, dep_value: str) -> bool:
    lines = config_path.read_text(encoding="utf-8").splitlines()
    rendered_line = f'{dep_name} = "{dep_value}"'

    dep_start = None
    dep_end = len(lines)
    for idx, line in enumerate(lines):
        stripped = line.strip()
        if stripped == "[dependencies]":
            dep_start = idx
            continue
        if dep_start is not None and stripped.startswith("[") and stripped.endswith("]"):
            dep_end = idx
            break

    changed = False
    if dep_start is None:
        if lines and lines[-1].strip() != "":
            lines.append("")
        lines.append("[dependencies]")
        lines.append(rendered_line)
        changed = True
    else:
        existing_idx = None
        for idx in range(dep_start + 1, dep_end):
            stripped = lines[idx].strip()
            if not stripped or stripped.startswith("#") or "=" not in stripped:
                continue
            key, _value = stripped.split("=", 1)
            if key.strip() == dep_name:
                existing_idx = idx
                break

        if existing_idx is not None:
            if lines[existing_idx].strip() != rendered_line:
                lines[existing_idx] = rendered_line
                changed = True
        else:
            lines.insert(dep_end, rendered_line)
            changed = True

    if changed:
        config_path.write_text("\n".join(lines) + "\n", encoding="utf-8")
    return changed


def _upsert_section_string_value(config_path: Path, section: str, key: str, value: str) -> bool:
    lines = config_path.read_text(encoding="utf-8").splitlines()
    rendered = f'{key} = "{value}"'
    section_header = f"[{section}]"

    sec_start = None
    sec_end = len(lines)
    for idx, line in enumerate(lines):
        stripped = line.strip()
        if stripped == section_header:
            sec_start = idx
            continue
        if sec_start is not None and stripped.startswith("[") and stripped.endswith("]"):
            sec_end = idx
            break

    changed = False
    if sec_start is None:
        if lines and lines[-1].strip() != "":
            lines.append("")
        lines.append(section_header)
        lines.append(rendered)
        changed = True
    else:
        existing_idx = None
        for idx in range(sec_start + 1, sec_end):
            stripped = lines[idx].strip()
            if not stripped or stripped.startswith("#") or "=" not in stripped:
                continue
            cur_key, _cur_val = stripped.split("=", 1)
            if cur_key.strip() == key:
                existing_idx = idx
                break
        if existing_idx is not None:
            if lines[existing_idx].strip() != rendered:
                lines[existing_idx] = rendered
                changed = True
        else:
            lines.insert(sec_end, rendered)
            changed = True

    if changed:
        config_path.write_text("\n".join(lines) + "\n", encoding="utf-8")
    return changed


def _resolve_dependency_from_registry(
    dep_name: str,
    registry_hit: dict,
    project_dir: Path,
    fetch: bool,
    refresh: bool,
    pin: bool,
    security_policy: dict,
    allow_untrusted: bool,
) -> tuple[str, str]:
    kind = registry_hit.get("kind")
    if kind == "path":
        resolved_dir, _pkg_config = _resolve_package_dir(str(registry_hit["path"]))
        dep_value = _to_project_relative_path(resolved_dir, project_dir)
        return "path", dep_value

    if kind == "git":
        if pin and not registry_hit.get("ref"):
            raise RuntimeError(f"Registry-pakke '{dep_name}' mangler låst git-ref (bruk pakke med ref eller uten --pin)")
        _enforce_trusted_source("git", registry_hit["git"], security_policy, allow_untrusted=allow_untrusted)
        if fetch:
            cached_root = _fetch_git_to_cache(
                project_dir,
                dep_name,
                registry_hit["git"],
                registry_hit.get("ref"),
                refresh=refresh,
            )
            return "path", _to_project_relative_path(cached_root, project_dir)
        return "git", _render_git_dependency(registry_hit["git"], registry_hit.get("ref"))

    if kind == "url":
        _enforce_trusted_source("url", registry_hit["url"], security_policy, allow_untrusted=allow_untrusted)
        if fetch:
            cached_root = _fetch_url_to_cache(project_dir, dep_name, registry_hit["url"], refresh=refresh)
            return "path", _to_project_relative_path(cached_root, project_dir)
        return "url", _render_url_dependency(registry_hit["url"])

    raise RuntimeError(f"Ukjent registry-kind for {dep_name}: {kind}")


def registry_sync(
    source_override: str | None = None,
    allow_untrusted: bool = False,
    require_all: bool = False,
    fallback_to_cache: bool = True,
):
    config_path = _find_project_config()
    project_dir = config_path.parent.resolve()
    security_policy = _load_security_policy(config_path)
    project_toml = _load_toml(config_path)
    registry_section = project_toml.get("registry", {}) if isinstance(project_toml.get("registry", {}), dict) else {}
    configured_sources = registry_section.get("sources", [])

    sources: list[str]
    if source_override:
        sources = [source_override]
    elif isinstance(configured_sources, list):
        sources = [s for s in configured_sources if isinstance(s, str) and s.strip()]
    else:
        sources = []

    if not sources:
        raise RuntimeError("Ingen registry-kilder satt. Bruk [registry].sources eller --source")

    merged: dict[str, dict] = {}
    used_sources: list[str] = []
    failed_sources: list[dict] = []
    for src in sources:
        try:
            source_name, text = _load_registry_source_text(src, project_dir, security_policy, allow_untrusted=allow_untrusted)
            entries = _parse_remote_registry_text(source_name, text)
        except Exception as exc:
            failed_sources.append({"source": src, "error": str(exc)})
            if require_all:
                raise RuntimeError(f"Registry-sync feilet for {src}: {exc}") from exc
            continue

        for name, entry in entries.items():
            prev = merged.get(name)
            if prev is None:
                merged[name] = entry
                continue
            prev_v = prev.get("version")
            cur_v = entry.get("version")
            if isinstance(cur_v, str) and isinstance(prev_v, str):
                if _semver_key(cur_v) > _semver_key(prev_v):
                    merged[name] = entry
            else:
                merged[name] = entry
        used_sources.append(source_name)

    cache_path = _remote_registry_cache_path(project_dir)
    cache_path.parent.mkdir(parents=True, exist_ok=True)

    stale_fallback_used = False
    if not merged and failed_sources and fallback_to_cache and cache_path.exists():
        merged = _load_remote_registry_cache(project_dir)
        stale_fallback_used = True

    if not merged and failed_sources:
        raise RuntimeError("Registry-sync feilet: ingen vellykkede kilder og ingen brukbar cache")

    payload = {
        "version": 1,
        "synced_at": dt.datetime.now(dt.timezone.utc).isoformat(),
        "sources": used_sources,
        "entries": merged,
        "failed_sources": failed_sources,
        "stale_fallback_used": stale_fallback_used,
    }
    cache_path.write_text(json.dumps(payload, ensure_ascii=False, indent=2, sort_keys=True) + "\n", encoding="utf-8")
    return {
        "cache": str(cache_path),
        "sources": used_sources,
        "count": len(merged),
        "failed_sources": failed_sources,
        "stale_fallback_used": stale_fallback_used,
    }


def registry_mirror(output_file: str | None = None):
    config_path = _find_project_config()
    project_dir = config_path.parent.resolve()
    entries = _read_registry_entries(config_path)
    mirror_path = (Path(output_file).expanduser() if output_file else project_dir / "build" / "registry_mirror.json")
    if not mirror_path.is_absolute():
        mirror_path = (project_dir / mirror_path).resolve()
    mirror_path.parent.mkdir(parents=True, exist_ok=True)

    packages = {}
    for name, meta in sorted(entries.items(), key=lambda item: item[0]):
        row = {"description": meta.get("description")}
        if isinstance(meta.get("version"), str):
            row["version"] = meta["version"]
        if isinstance(meta.get("source"), str):
            row["source"] = meta["source"]

        kind = meta.get("kind")
        if kind == "path":
            p = meta.get("path")
            if isinstance(p, Path):
                row["path"] = str(p)
            elif isinstance(p, str):
                row["path"] = p
        elif kind == "git":
            row["git"] = meta.get("git")
            if isinstance(meta.get("ref"), str):
                row["ref"] = meta["ref"]
        elif kind == "url":
            row["url"] = meta.get("url")
        packages[name] = row

    payload = {
        "format_version": 1,
        "generated_at": dt.datetime.now(dt.timezone.utc).isoformat(),
        "packages": packages,
    }
    mirror_path.write_text(json.dumps(payload, ensure_ascii=False, indent=2, sort_keys=True) + "\n", encoding="utf-8")
    return {
        "output": str(mirror_path),
        "count": len(packages),
    }


def registry_host(
    host: str = "127.0.0.1",
    port: int = 8765,
    mirror_file: str | None = None,
    once: bool = False,
):
    if mirror_file and Path(mirror_file).expanduser().exists():
        mirror_path = Path(mirror_file).expanduser().resolve()
        try:
            mirror_payload = json.loads(mirror_path.read_text(encoding="utf-8"))
            package_count = len(mirror_payload.get("packages", {})) if isinstance(mirror_payload, dict) else 0
        except Exception:
            package_count = 0
        mirror = {"output": str(mirror_path), "count": package_count}
    else:
        mirror = registry_mirror(output_file=mirror_file)
        mirror_path = Path(mirror["output"]).resolve()
    mirror_bytes = mirror_path.read_bytes()

    class RegistryHandler(BaseHTTPRequestHandler):
        def log_message(self, _format: str, *args):  # noqa: N802
            return

        def do_GET(self):  # noqa: N802
            parsed = urllib.parse.urlparse(self.path)
            if parsed.path in {"/", "/index.json", "/packages"}:
                body = mirror_bytes
                self.send_response(200)
                self.send_header("Content-Type", "application/json; charset=utf-8")
                self.send_header("Content-Length", str(len(body)))
                self.end_headers()
                self.wfile.write(body)
                return
            self.send_response(404)
            self.send_header("Content-Type", "text/plain; charset=utf-8")
            self.end_headers()
            self.wfile.write(b"not found\n")

    try:
        server = ThreadingHTTPServer((host, port), RegistryHandler)
    except PermissionError as exc:
        if not once:
            raise
        fallback_index = mirror_path.parent / "index.json"
        fallback_index.write_bytes(mirror_bytes)
        return {
            "ok": True,
            "url": fallback_index.resolve().as_uri(),
            "mirror": str(mirror_path),
            "count": mirror["count"],
            "served_bytes": len(mirror_bytes),
            "once": True,
            "skip_reason": f"registry host unavailable: {exc}",
        }
    actual_host, actual_port = server.server_address
    url = f"http://{actual_host}:{actual_port}/index.json"

    if once:
        thread = threading.Thread(target=server.serve_forever, daemon=True)
        thread.start()
        try:
            with urllib.request.urlopen(url, timeout=5) as resp:
                hosted = resp.read()
            ok = hosted == mirror_bytes
        finally:
            server.shutdown()
            server.server_close()
            thread.join(timeout=5)
        return {
            "ok": ok,
            "url": url,
            "mirror": str(mirror_path),
            "count": mirror["count"],
            "served_bytes": len(mirror_bytes),
            "once": True,
        }

    return {
        "ok": True,
        "url": url,
        "mirror": str(mirror_path),
        "count": mirror["count"],
        "served_bytes": len(mirror_bytes),
        "once": False,
        "server": server,
    }


def registry_sign(write_config: bool = False):
    config_path = _find_project_config()
    project_dir = config_path.parent.resolve()
    registry_path = (project_dir / "packages" / "registry.toml").resolve()
    if not registry_path.exists():
        raise RuntimeError(f"Fant ikke registry-fil: {registry_path}")

    digest = _hash_file(registry_path).lower()
    changed = False
    if write_config:
        changed = _upsert_section_string_value(
            config_path,
            section="security",
            key="trusted_registry_sha256",
            value=digest,
        )

    return {
        "registry": str(registry_path),
        "sha256": digest,
        "config": str(config_path),
        "written_to_config": write_config,
        "config_changed": changed,
    }


def list_registry_packages():
    config_path = _find_project_config()
    entries = _read_registry_entries(config_path)
    return config_path, entries


def generate_lockfile(check_only: bool = False):
    config_path = _find_project_config()
    project_dir = config_path.parent.resolve()
    deps = _parse_dependencies_from_toml(config_path)
    project_toml = _load_toml(config_path)
    project_meta = project_toml.get("project", {}) if isinstance(project_toml.get("project", {}), dict) else {}

    lock = {
        "lock_version": 1,
        "generated_at": dt.datetime.now(dt.timezone.utc).isoformat(),
        "project": {
            "name": project_meta.get("name", project_dir.name),
            "version": project_meta.get("version"),
            "entry": project_meta.get("entry"),
        },
        "dependencies": {},
    }

    for dep_name, dep_value in sorted(deps.items(), key=lambda item: item[0]):
        entry = {"specifier": dep_value}
        if dep_value.startswith("git+"):
            git_url, git_ref = _parse_git_dependency(dep_value)
            entry["kind"] = "git"
            entry["resolved"] = {
                "url": git_url,
                "ref": git_ref,
                "pinned": bool(git_ref),
            }
        elif dep_value.startswith("url+"):
            url = dep_value[len("url+") :]
            entry["kind"] = "url"
            entry["resolved"] = {
                "url": url,
                "pinned": True,
            }
        else:
            dep_path = _resolve_path_dependency(project_dir, dep_value)
            entry["kind"] = "path"
            try:
                if dep_path.is_absolute() and dep_path.is_relative_to(project_dir):
                    resolved_path = dep_path.relative_to(project_dir)
                else:
                    resolved_path = dep_path
            except ValueError:
                resolved_path = dep_path
            resolved = {
                "path": str(resolved_path),
                "exists": dep_path.exists(),
            }
            if dep_path.exists() and dep_path.is_dir():
                resolved["digest_sha256"] = _hash_directory(dep_path)
                dep_cfg = _find_existing_project_config_in_dir(dep_path)
                if dep_cfg is not None and dep_cfg.exists():
                    dep_toml = _load_toml(dep_cfg)
                    proj = dep_toml.get("project", {})
                    if isinstance(proj, dict):
                        resolved["project_name"] = proj.get("name")
                        resolved["project_version"] = proj.get("version")
                        resolved["entry"] = proj.get("entry")
            elif dep_path.exists() and dep_path.is_file():
                resolved["digest_sha256"] = _hash_file(dep_path)
            entry["resolved"] = resolved

        lock["dependencies"][dep_name] = entry

    lock_path = project_dir / LOCKFILE_NAME
    payload = json.dumps(lock, ensure_ascii=False, indent=2, sort_keys=True) + "\n"

    if check_only:
        if not lock_path.exists():
            return lock_path, False, "mangler"
        try:
            current_obj = json.loads(lock_path.read_text(encoding="utf-8"))
        except Exception:
            return lock_path, False, "utdatert"

        expected_obj = dict(lock)
        current_obj = dict(current_obj) if isinstance(current_obj, dict) else {}
        expected_obj.pop("generated_at", None)
        current_obj.pop("generated_at", None)
        ok = current_obj == expected_obj
        return lock_path, ok, "ok" if ok else "utdatert"

    lock_path.write_text(payload, encoding="utf-8")
    return lock_path, True, "skrevet"


def verify_lockfile():
    cwd = Path.cwd().resolve()
    lock_path = (cwd / LOCKFILE_NAME).resolve()
    for candidate_name in LOCKFILE_NAMES:
        candidate = cwd / candidate_name
        if candidate.exists():
            lock_path = candidate
            break
    if lock_path.name == LEGACY_LOCKFILE_NAME:
        _warn_legacy_once(
            "legacy-lockfile",
            "Merk: bruker legacy lockfile 'norsklang.lock'. Bytt til 'norcode.lock'.",
        )
    if not lock_path.exists():
        return lock_path, False, [{"name": "*", "status": "mangler lockfile"}]
    try:
        lock = json.loads(lock_path.read_text(encoding="utf-8"))
    except Exception as exc:
        return lock_path, False, [{"name": "*", "status": f"ugyldig lockfile: {exc}"}]
    deps = lock.get("dependencies", {})
    if not isinstance(deps, dict):
        return lock_path, False, [{"name": "*", "status": "ugyldig dependencies i lockfile"}]

    results = []
    ok = True
    for name, meta in sorted(deps.items(), key=lambda item: item[0]):
        status = "ok"
        resolved = meta.get("resolved", {}) if isinstance(meta, dict) else {}
        kind = meta.get("kind") if isinstance(meta, dict) else None
        if not isinstance(resolved, dict):
            status = "ugyldig resolved"
            ok = False
        elif kind == "path":
            path = resolved.get("path")
            if not isinstance(path, str) or not path:
                status = "mangler path i lock"
                ok = False
            else:
                p = Path(path)
                if not p.is_absolute():
                    p = lock_path.parent / path
                if not p.exists():
                    status = f"mangler path: {path}"
                    ok = False
                elif "digest_sha256" in resolved:
                    digest = _hash_directory(p) if p.is_dir() else _hash_file(p)
                    if digest != resolved.get("digest_sha256"):
                        status = "digest avviker"
                        ok = False
        results.append({"name": name, "status": status})
    return lock_path, ok, results


def update_dependencies(
    package: str | None = None,
    check_only: bool = False,
    pin: bool = False,
    fetch: bool = False,
    refresh: bool = False,
    with_lock: bool = False,
    allow_untrusted: bool = False,
):
    config_path = _find_project_config()
    project_dir = config_path.parent.resolve()
    project_toml = _load_toml(config_path)
    registry_section = project_toml.get("registry", {}) if isinstance(project_toml.get("registry", {}), dict) else {}
    if package:
        entries = _read_registry_entries(config_path)
        if package not in entries:
            raise RuntimeError(f"Ukjent registry-pakke: {package}")
        targets = {package: entries[package]}
    else:
        targets = _read_registry_entries(config_path)

    security_policy = _load_security_policy(config_path)
    updated = 0
    unchanged = 0
    skipped = 0
    items: list[dict[str, object]] = []

    for dep_name, registry_hit in sorted(targets.items(), key=lambda item: item[0]):
        try:
            dep_kind, dep_value = _resolve_dependency_from_registry(
                dep_name,
                registry_hit,
                project_dir,
                fetch=fetch,
                refresh=refresh,
                pin=pin,
                security_policy=security_policy,
                allow_untrusted=allow_untrusted,
            )
        except Exception as exc:
            skipped += 1
            items.append({"name": dep_name, "status": "skipped", "reason": str(exc)})
            continue

        current_deps = _parse_dependencies_from_toml(config_path)
        current_value = current_deps.get(dep_name)
        if current_value == dep_value:
            unchanged += 1
            items.append({"name": dep_name, "status": "unchanged", "to": dep_value})
            continue
        if not check_only:
            _upsert_dependency(config_path, dep_name, dep_value)
        updated += 1
        items.append({"name": dep_name, "status": "updated", "to": dep_value})

    lock_info = None
    if with_lock and not check_only:
        lock_path, _ok, status = generate_lockfile(check_only=False)
        lock_info = {"path": str(lock_path), "status": status}

    return {
        "config": str(config_path),
        "target": package or "all",
        "updated": updated,
        "unchanged": unchanged,
        "skipped": skipped,
        "items": items,
        "lock": lock_info,
    }


def add_dependency(
    package: str,
    package_path: str | None = None,
    dep_name_override: str | None = None,
    git_url: str | None = None,
    git_ref: str | None = None,
    tarball_url: str | None = None,
    fetch: bool = False,
    refresh: bool = False,
    pin: bool = False,
    expected_sha256: str | None = None,
    allow_untrusted: bool = False,
):
    config_path = _find_project_config()
    project_dir = config_path.parent.resolve()
    registry_entries = _read_registry_entries(config_path)
    security_policy = _load_security_policy(config_path)

    if git_url and tarball_url:
        raise RuntimeError("Bruk enten --git eller --url, ikke begge")
    if (git_url or tarball_url) and package_path:
        raise RuntimeError("Ikke kombiner path-argument med --git/--url")
    if git_ref and not git_url:
        raise RuntimeError("--ref krever --git")
    if pin and git_url and not git_ref:
        raise RuntimeError("--pin krever --ref når --git brukes")
    if expected_sha256 is not None and not _is_valid_sha256(expected_sha256):
        raise RuntimeError("--sha256 må være 64 hex-tegn")
    if expected_sha256 is not None and not fetch:
        raise RuntimeError("--sha256 krever --fetch")

    path_like = any(sep in package for sep in ("/", "\\")) or package.startswith(".")
    dep_kind = "path"
    dep_value = ""
    package_name = package

    if git_url:
        dep_name = dep_name_override or package
        _enforce_trusted_source("git", git_url, security_policy, allow_untrusted=allow_untrusted)
        dep_kind = "git"
        dep_value = _render_git_dependency(git_url, git_ref)
        package_name = dep_name
        if fetch:
            cached_root = _fetch_git_to_cache(project_dir, dep_name, git_url, git_ref, refresh=refresh)
            dep_kind = "path"
            dep_value = _to_project_relative_path(cached_root, project_dir)
            cached_cfg = _find_existing_project_config_in_dir(cached_root) or (cached_root / PROJECT_CONFIG_NAME)
            package_name = _parse_project_name_from_toml(cached_cfg) or cached_root.name
    elif tarball_url:
        dep_name = dep_name_override or package
        _enforce_trusted_source("url", tarball_url, security_policy, allow_untrusted=allow_untrusted)
        dep_kind = "url"
        dep_value = _render_url_dependency(tarball_url)
        package_name = dep_name
        if fetch:
            cached_root = _fetch_url_to_cache(
                project_dir,
                dep_name,
                tarball_url,
                refresh=refresh,
                expected_sha256=expected_sha256,
            )
            dep_kind = "path"
            dep_value = _to_project_relative_path(cached_root, project_dir)
            cached_cfg = _find_existing_project_config_in_dir(cached_root) or (cached_root / PROJECT_CONFIG_NAME)
            package_name = _parse_project_name_from_toml(cached_cfg) or cached_root.name
    if package_path:
        dep_name = dep_name_override or package
        resolved_dir, pkg_config = _resolve_package_dir(package_path)
        dep_kind = "path"
        dep_value = _to_project_relative_path(resolved_dir, project_dir)
        package_name = _parse_project_name_from_toml(pkg_config) or resolved_dir.name
    elif not git_url and not tarball_url and path_like:
        resolved_dir, pkg_config = _resolve_package_dir(package)
        dep_name = dep_name_override or _parse_project_name_from_toml(pkg_config) or resolved_dir.name
        dep_kind = "path"
        dep_value = _to_project_relative_path(resolved_dir, project_dir)
        package_name = _parse_project_name_from_toml(pkg_config) or resolved_dir.name
    elif not git_url and not tarball_url:
        dep_name = dep_name_override or package
        registry_hit = registry_entries.get(package)
        if registry_hit is not None:
            if registry_hit.get("kind") == "path":
                resolved_dir, pkg_config = _resolve_package_dir(str(registry_hit["path"]))
                dep_kind = "path"
                dep_value = _to_project_relative_path(resolved_dir, project_dir)
                package_name = _parse_project_name_from_toml(pkg_config) or resolved_dir.name
            elif registry_hit.get("kind") == "git":
                if pin and not registry_hit.get("ref"):
                    raise RuntimeError(f"Registry-pakke '{package}' mangler låst git-ref (bruk pakke med ref eller uten --pin)")
                _enforce_trusted_source("git", registry_hit["git"], security_policy, allow_untrusted=allow_untrusted)
                dep_kind = "git"
                dep_value = _render_git_dependency(registry_hit["git"], registry_hit.get("ref"))
                package_name = dep_name
                if fetch:
                    cached_root = _fetch_git_to_cache(
                        project_dir,
                        dep_name,
                        registry_hit["git"],
                        registry_hit.get("ref"),
                        refresh=refresh,
                    )
                    dep_kind = "path"
                    dep_value = _to_project_relative_path(cached_root, project_dir)
                    cached_cfg = _find_existing_project_config_in_dir(cached_root) or (cached_root / PROJECT_CONFIG_NAME)
                    package_name = _parse_project_name_from_toml(cached_cfg) or cached_root.name
            elif registry_hit.get("kind") == "url":
                _enforce_trusted_source("url", registry_hit["url"], security_policy, allow_untrusted=allow_untrusted)
                dep_kind = "url"
                dep_value = _render_url_dependency(registry_hit["url"])
                package_name = dep_name
                if fetch:
                    cached_root = _fetch_url_to_cache(
                        project_dir,
                        dep_name,
                        registry_hit["url"],
                        refresh=refresh,
                        expected_sha256=expected_sha256,
                    )
                    dep_kind = "path"
                    dep_value = _to_project_relative_path(cached_root, project_dir)
                    cached_cfg = _find_existing_project_config_in_dir(cached_root) or (cached_root / PROJECT_CONFIG_NAME)
                    package_name = _parse_project_name_from_toml(cached_cfg) or cached_root.name
            else:
                raise RuntimeError(f"Ukjent registry-kind for {package}: {registry_hit.get('kind')}")
        else:
            default_dir = project_dir / "packages" / package
            resolved_dir, pkg_config = _resolve_package_dir(str(default_dir))
            dep_kind = "path"
            dep_value = _to_project_relative_path(resolved_dir, project_dir)
            package_name = _parse_project_name_from_toml(pkg_config) or resolved_dir.name

    if not dep_name:
        raise RuntimeError("Kunne ikke finne avhengighetsnavn (bruk --name)")

    if not dep_value:
        raise RuntimeError("Kunne ikke finne dependency-verdi")

    changed = _upsert_dependency(config_path, dep_name, dep_value)
    return config_path, dep_name, dep_value, package_name, dep_kind, changed
