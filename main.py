from __future__ import annotations

import argparse
import datetime as dt
import difflib
import hashlib
import json
import locale
import os
import re
import shutil
import shlex
import subprocess
import sys
import tarfile
import tempfile
import time
import platform
from http.server import BaseHTTPRequestHandler, ThreadingHTTPServer
import threading
import urllib.parse
import urllib.request
import uuid
import zipfile
from pathlib import Path
from types import SimpleNamespace

from compiler.cgen import CGenerator
from compiler.ast_nodes import (
    AwaitNode,
    BinOpNode,
    BreakNode,
    CallNode,
    ContinueNode,
    ExprStmtNode,
    FieldAccessNode,
    ForEachNode,
    ForNode,
    IfNode,
    IfExprNode,
    IndexNode,
    IndexSetNode,
    ListLiteralNode,
    MapLiteralNode,
    ModuleCallNode,
    PrintNode,
    ReturnNode,
    SliceNode,
    StructLiteralNode,
    ThrowNode,
    TryCatchNode,
    UnaryOpNode,
    VarDeclareNode,
    VarSetNode,
    WhileNode,
)
from compiler.formatter import format_source
from compiler.lexer import Lexer
from compiler.loader import ModuleLoader
from compiler.parser import Parser
from compiler.semantic import SemanticAnalyzer
from compiler.selfhost_chain import export_selfhost_ast_bundle, run_chain, check_chain
from compiler.toml_compat import loads as toml_loads


IR_OPS_WITH_ARG = {"PUSH", "LABEL", "JMP", "JZ", "CALL", "STORE", "LOAD"}
IR_OPS_NO_ARG = {
    "ADD",
    "SUB",
    "MUL",
    "DIV",
    "MOD",
    "EQ",
    "GT",
    "LT",
    "AND",
    "OR",
    "NOT",
    "DUP",
    "POP",
    "SWAP",
    "OVER",
    "PRINT",
    "HALT",
    "RET",
}
IR_ALL_OPS = IR_OPS_WITH_ARG | IR_OPS_NO_ARG
IR_SNAPSHOT_FIXTURE = Path("tests/ir_snapshot_cases.json")
SELFHOST_PARSER_M1_FIXTURE = Path("tests/selfhost_parser_m1_cases.json")
SELFHOST_PARSER_M2_FIXTURE = Path("tests/selfhost_parser_m2_cases.json")
SELFHOST_PARSER_EXTENDED_FIXTURE = Path("tests/selfhost_parser_core_cases.json")
WORKFLOW_ACTION_POLICY = {
    "minimum_action_majors": {
        "actions/checkout": 6,
        "actions/setup-python": 6,
    },
    "require_node24_env": True,
    "forbid_unsecure_node_opt_out": True,
    "required_norcode_ci_flags": [
        "--check-names",
        "--require-selfhost-ready",
    ],
}
PROJECT_CONFIG_NAME = "norcode.toml"
LEGACY_PROJECT_CONFIG_NAME = "norsklang.toml"
PROJECT_CONFIG_NAMES = (PROJECT_CONFIG_NAME, LEGACY_PROJECT_CONFIG_NAME)
PYPROJECT_NAME = "pyproject.toml"
CHANGELOG_NAME = "CHANGELOG.md"
LOCKFILE_NAME = "norcode.lock"
LEGACY_LOCKFILE_NAME = "norsklang.lock"
LOCKFILE_NAMES = (LOCKFILE_NAME, LEGACY_LOCKFILE_NAME)
REMOTE_REGISTRY_CACHE = ".norcode/registry/remote_index.json"
LEGACY_REMOTE_REGISTRY_CACHE = ".norsklang/registry/remote_index.json"
SEMVER_RE = re.compile(r"^\d+\.\d+\.\d+$")
_LEGACY_WARNINGS_EMITTED: set[str] = set()


def _warn_legacy_once(key: str, message: str):
    if key in _LEGACY_WARNINGS_EMITTED:
        return
    _LEGACY_WARNINGS_EMITTED.add(key)
    print(message, file=sys.stderr)


def _repo_root() -> Path:
    return Path(__file__).resolve().parent


def _run_checked_command(args: list[str], cwd: Path | None = None) -> subprocess.CompletedProcess[str]:
    return subprocess.run(
        args,
        cwd=str(cwd or _repo_root()),
        check=True,
        text=True,
        capture_output=True,
    )


def run_benchmark_suite() -> dict:
    root = _repo_root()
    benchmarks = [
        {
            "name": "check-map",
            "kind": "check",
            "file": "tests/test_map.no",
            "budget_ms": 4000,
        },
        {
            "name": "test-json",
            "kind": "test",
            "file": "tests/test_json.no",
            "budget_ms": 4000,
        },
        {
            "name": "test-selfhost",
            "kind": "test",
            "file": "tests/test_selfhost.no",
            "budget_ms": 12000,
        },
        {
            "name": "commands-json",
            "kind": "cli",
            "args": ["./bin/nc", "commands", "--json"],
            "budget_ms": 2000,
        },
    ]
    payload = {
        "ok": False,
        "benchmarks": [],
        "total_duration_ms": 0,
        "max_duration_ms": 0,
        "budget_exceeded_count": 0,
        "thresholds": {item["name"]: item["budget_ms"] for item in benchmarks},
    }
    started = time.perf_counter()
    for item in benchmarks:
        case_started = time.perf_counter()
        if item["kind"] == "check":
            result = check_program(item["file"])
            case_ok = True
            details = {"source": str(result[0]), "functions": len(result[3].functions)}
        elif item["kind"] == "test":
            result = run_test_file(item["file"])
            case_ok = bool(result["success"])
            details = {
                "source": result["source"],
                "success": result["success"],
                "stdout_lines": len(result.get("stdout", "").splitlines()),
            }
            if not case_ok:
                raise RuntimeError(f"Benchmark-feil i test {item['file']}")
        else:
            completed = _run_checked_command(item["args"], cwd=root)
            case_ok = completed.returncode == 0
            details = {
                "cmd": " ".join(item["args"]),
                "stdout_lines": len(completed.stdout.splitlines()),
            }
        duration_ms = int((time.perf_counter() - case_started) * 1000)
        within_budget = duration_ms <= int(item["budget_ms"])
        payload["benchmarks"].append(
            {
                "name": item["name"],
                "kind": item["kind"],
                "duration_ms": duration_ms,
                "budget_ms": item["budget_ms"],
                "within_budget": within_budget,
                "ok": case_ok,
                "details": details,
            }
        )
        if not within_budget:
            payload["budget_exceeded_count"] += 1
        payload["max_duration_ms"] = max(payload["max_duration_ms"], duration_ms)
    payload["total_duration_ms"] = int((time.perf_counter() - started) * 1000)
    payload["ok"] = payload["budget_exceeded_count"] == 0
    return payload


def run_smoke_suite() -> dict:
    root = _repo_root()
    release_version = f"smoke-{dt.datetime.now(dt.timezone.utc).strftime('%Y%m%d-%H%M%S')}"
    temp_prefix = Path(tempfile.mkdtemp(prefix="norscode-smoke-"))
    payload = {
        "ok": False,
        "release_version": release_version,
        "temp_prefix": str(temp_prefix),
        "steps": [],
    }
    try:
        steps = [
            ("build-bootstrap", ["bash", "tools/build-bootstrap-binary.sh"], root),
            ("package-release", ["bash", "package-release.sh", release_version], root),
            (
                "install-release",
                ["bash", "tools/install-release.sh", f"release-artifacts/norscode-language-{release_version}.tar.gz", "--prefix", str(temp_prefix)],
                root,
            ),
            (
                "installed-help",
                [str(temp_prefix / "current" / "bin" / "nc"), "--help"],
                temp_prefix / "current",
            ),
            (
                "installed-test",
                [str(temp_prefix / "current" / "bin" / "nc"), "test"],
                temp_prefix / "current",
            ),
        ]
        for name, args, cwd in steps:
            started = time.perf_counter()
            completed = subprocess.run(
                args,
                cwd=str(cwd),
                check=True,
                text=True,
                capture_output=True,
            )
            payload["steps"].append(
                {
                    "name": name,
                    "ok": True,
                    "duration_ms": int((time.perf_counter() - started) * 1000),
                    "stdout_lines": len(completed.stdout.splitlines()),
                }
            )
        payload["ok"] = True
        return payload
    except subprocess.CalledProcessError as exc:
        payload["steps"].append(
            {
                "name": "failed",
                "ok": False,
                "returncode": exc.returncode,
                "stdout": exc.stdout,
                "stderr": exc.stderr,
            }
        )
        raise RuntimeError("Smoke-test feilet") from exc


def run_negative_suite() -> dict:
    parser_cases = [
        {"name": "empty-op", "source": "funksjon start() -> heltall { + }"},
        {"name": "broken-fun", "source": "funksjon start() -> heltall { fun }"},
        {"name": "dangling-assign", "source": "funksjon start() -> heltall { la x = }"},
        {"name": "unknown-token", "source": "funksjon start() -> heltall { ??? }"},
        {"name": "missing-brace", "source": "funksjon start() -> heltall { la x = 1"},
    ]
    payload = {
        "ok": False,
        "parser_cases": [],
        "runtime_cases": [],
        "parser_failures": 0,
        "runtime_failures": 0,
    }
    for case in parser_cases:
        started = time.perf_counter()
        failed = False
        error_text = ""
        try:
            parse_source(case["source"])
        except Exception as exc:
            failed = True
            error_text = str(exc)
        duration_ms = int((time.perf_counter() - started) * 1000)
        payload["parser_cases"].append(
            {
                "name": case["name"],
                "ok": failed,
                "duration_ms": duration_ms,
                "error": error_text,
            }
        )
        if not failed:
            payload["parser_failures"] += 1

    runtime_source = "funksjon start() -> heltall { kast(\"boom\") }"
    started = time.perf_counter()
    runtime_ok = False
    runtime_error = ""
    with tempfile.TemporaryDirectory(prefix="norscode-negative-") as tmpdir:
        tmp_path = Path(tmpdir) / "negative_runtime.no"
        tmp_path.write_text(runtime_source, encoding="utf-8")
        try:
            run_program(str(tmp_path))
        except subprocess.CalledProcessError as exc:
            runtime_ok = True
            runtime_error = (exc.stderr or "").strip() or (exc.stdout or "").strip()
        except Exception as exc:
            runtime_ok = True
            runtime_error = str(exc)
    duration_ms = int((time.perf_counter() - started) * 1000)
    payload["runtime_cases"].append(
        {
            "name": "unhandled-throw",
            "ok": runtime_ok,
            "duration_ms": duration_ms,
            "error": runtime_error,
        }
    )
    if not runtime_ok:
        payload["runtime_failures"] += 1

    payload["ok"] = payload["parser_failures"] == 0 and payload["runtime_failures"] == 0
    return payload


def _find_existing_project_config_in_dir(base: Path) -> Path | None:
    for name in PROJECT_CONFIG_NAMES:
        candidate = base / name
        if candidate.exists():
            return candidate
    return None


def _project_config_display_names() -> str:
    return " / ".join(PROJECT_CONFIG_NAMES)


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


def _resolve_package_dir(package_path: str) -> tuple[Path, Path]:
    path = Path(package_path).expanduser()
    if not path.is_absolute():
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


def _load_toml(path: Path) -> dict:
    try:
        return toml_loads(path.read_text(encoding="utf-8"))
    except Exception as exc:
        raise RuntimeError(f"Kunne ikke lese TOML {path}: {exc}") from exc


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


def _verify_registry_integrity(config_path: Path, registry_file: Path):
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


def _enforce_trusted_source(kind: str, source: str, security_policy: dict, allow_untrusted: bool = False):
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

    # 1) Inline registry in project config: [registry.packages]
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

    # 2) External registry file: packages/registry.toml
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

    # 3) Synkronisert remote cache (overskriver ikke lokale entries)
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


def _hash_file(path: Path) -> str:
    h = hashlib.sha256()
    with path.open("rb") as f:
        while True:
            chunk = f.read(1024 * 1024)
            if not chunk:
                break
            h.update(chunk)
    return h.hexdigest()


def _is_valid_sha256(value: str) -> bool:
    return bool(re.fullmatch(r"[0-9a-fA-F]{64}", value))


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
        # Velg nærmeste kandidat (kortest sti) for forutsigbarhet.
        candidates.sort(key=lambda p: len(p.relative_to(base_dir).parts))
    return candidates[0].parent


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


def verify_lockfile():
    cwd = Path.cwd().resolve()
    lock_path = (cwd / LOCKFILE_NAME).resolve()
    for candidate_name in LOCKFILE_NAMES:
        candidate = (cwd / candidate_name).resolve()
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
    for name, entry in sorted(deps.items(), key=lambda item: item[0]):
        if not isinstance(entry, dict):
            results.append({"name": name, "status": "ugyldig entry"})
            ok = False
            continue

        kind = entry.get("kind")
        resolved = entry.get("resolved", {})
        if kind == "path":
            path_str = resolved.get("path") if isinstance(resolved, dict) else None
            expected_digest = resolved.get("digest_sha256") if isinstance(resolved, dict) else None
            if not isinstance(path_str, str):
                results.append({"name": name, "status": "mangler path i lock"})
                ok = False
                continue

            path = Path(path_str)
            if not path.is_absolute():
                path = lock_path.parent / path
            if not path.exists():
                results.append({"name": name, "status": f"mangler path: {path}"})
                ok = False
                continue

            if isinstance(expected_digest, str):
                actual = _hash_directory(path) if path.is_dir() else _hash_file(path)
                if actual != expected_digest:
                    results.append({"name": name, "status": "digest mismatch"})
                    ok = False
                    continue
            results.append({"name": name, "status": "ok"})
        elif kind == "git":
            ref = resolved.get("ref") if isinstance(resolved, dict) else None
            pinned = bool(ref)
            results.append({"name": name, "status": "ok" if pinned else "advarsel: upinnet git-ref"})
        elif kind == "url":
            results.append({"name": name, "status": "ok"})
        else:
            results.append({"name": name, "status": f"ukjent kind: {kind}"})
            ok = False

    return lock_path, ok, results


def migrate_names(apply_changes: bool = False, cleanup_legacy: bool = False) -> dict:
    config_path = _find_project_config()
    project_dir = config_path.parent.resolve()
    actions: list[dict] = []

    def record_file_migration(legacy_name: str, primary_name: str) -> tuple[Path, Path, str]:
        legacy = (project_dir / legacy_name).resolve()
        primary = (project_dir / primary_name).resolve()
        if not legacy.exists():
            actions.append(
                {
                    "kind": "file",
                    "legacy": str(legacy),
                    "primary": str(primary),
                    "status": "skipped",
                    "reason": "legacy mangler",
                }
            )
            return legacy, primary, "skipped"
        if primary.exists():
            actions.append(
                {
                    "kind": "file",
                    "legacy": str(legacy),
                    "primary": str(primary),
                    "status": "skipped",
                    "reason": "primary finnes allerede",
                }
            )
            return legacy, primary, "skipped"
        if apply_changes:
            shutil.copy2(legacy, primary)
            status = "copied"
        else:
            status = "planned"
        actions.append(
            {
                "kind": "file",
                "legacy": str(legacy),
                "primary": str(primary),
                "status": status,
            }
        )
        return legacy, primary, status

    def record_dir_migration(legacy_name: str, primary_name: str) -> tuple[Path, Path, str]:
        legacy = (project_dir / legacy_name).resolve()
        primary = (project_dir / primary_name).resolve()
        if not legacy.exists():
            actions.append(
                {
                    "kind": "dir",
                    "legacy": str(legacy),
                    "primary": str(primary),
                    "status": "skipped",
                    "reason": "legacy mangler",
                }
            )
            return legacy, primary, "skipped"
        if primary.exists():
            actions.append(
                {
                    "kind": "dir",
                    "legacy": str(legacy),
                    "primary": str(primary),
                    "status": "skipped",
                    "reason": "primary finnes allerede",
                }
            )
            return legacy, primary, "skipped"
        if apply_changes:
            shutil.copytree(legacy, primary)
            status = "copied"
        else:
            status = "planned"
        actions.append(
            {
                "kind": "dir",
                "legacy": str(legacy),
                "primary": str(primary),
                "status": status,
            }
        )
        return legacy, primary, status

    migrated_resources = [
        ("file", *record_file_migration(LEGACY_PROJECT_CONFIG_NAME, PROJECT_CONFIG_NAME)),
        ("file", *record_file_migration(LEGACY_LOCKFILE_NAME, LOCKFILE_NAME)),
        ("dir", *record_dir_migration(".norsklang", ".norcode")),
    ]

    if cleanup_legacy:
        for kind, legacy, primary, status in migrated_resources:
            if not legacy.exists():
                continue
            primary_ready = primary.exists() or status == "copied"
            if not primary_ready:
                actions.append(
                    {
                        "kind": f"cleanup-{kind}",
                        "legacy": str(legacy),
                        "primary": str(primary),
                        "status": "skipped",
                        "reason": "primary mangler",
                    }
                )
                continue
            if kind == "dir":
                if not legacy.is_dir() or not primary.is_dir():
                    actions.append(
                        {
                            "kind": f"cleanup-{kind}",
                            "legacy": str(legacy),
                            "primary": str(primary),
                            "status": "skipped",
                            "reason": "type mismatch",
                        }
                    )
                    continue
                if _hash_directory(legacy) != _hash_directory(primary):
                    actions.append(
                        {
                            "kind": f"cleanup-{kind}",
                            "legacy": str(legacy),
                            "primary": str(primary),
                            "status": "skipped",
                            "reason": "primary avviker",
                        }
                    )
                    continue
            else:
                if not legacy.is_file() or not primary.is_file():
                    actions.append(
                        {
                            "kind": f"cleanup-{kind}",
                            "legacy": str(legacy),
                            "primary": str(primary),
                            "status": "skipped",
                            "reason": "type mismatch",
                        }
                    )
                    continue
                if _hash_file(legacy) != _hash_file(primary):
                    actions.append(
                        {
                            "kind": f"cleanup-{kind}",
                            "legacy": str(legacy),
                            "primary": str(primary),
                            "status": "skipped",
                            "reason": "primary avviker",
                        }
                    )
                    continue
            if apply_changes:
                if kind == "dir":
                    shutil.rmtree(legacy)
                else:
                    legacy.unlink()
                cleanup_status = "removed"
            else:
                cleanup_status = "planned-remove"
            actions.append(
                {
                    "kind": f"cleanup-{kind}",
                    "legacy": str(legacy),
                    "primary": str(primary),
                    "status": cleanup_status,
                }
            )

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
    deps = _parse_dependencies_from_toml(config_path)
    registry_entries = _read_registry_entries(config_path)
    security_policy = _load_security_policy(config_path)

    if package is not None:
        if package not in deps:
            raise RuntimeError(f"Dependency finnes ikke i {_project_config_display_names()}: {package}")
        targets = [package]
    else:
        targets = sorted(deps.keys())

    updated = 0
    unchanged = 0
    skipped = 0
    items = []

    for dep_name in targets:
        current = deps[dep_name]
        hit = registry_entries.get(dep_name)
        if hit is None:
            skipped += 1
            items.append(
                {
                    "name": dep_name,
                    "status": "skipped",
                    "reason": "ikke i registry",
                    "current": current,
                }
            )
            continue

        new_kind, desired = _resolve_dependency_from_registry(
            dep_name=dep_name,
            registry_hit=hit,
            project_dir=project_dir,
            fetch=fetch,
            refresh=refresh,
            pin=pin,
            security_policy=security_policy,
            allow_untrusted=allow_untrusted,
        )

        if current == desired:
            unchanged += 1
            items.append(
                {
                    "name": dep_name,
                    "status": "unchanged",
                    "kind": new_kind,
                    "value": desired,
                }
            )
            continue

        if not check_only:
            _upsert_dependency(config_path, dep_name, desired)
        updated += 1
        items.append(
            {
                "name": dep_name,
                "status": "updated",
                "kind": new_kind,
                "from": current,
                "to": desired,
            }
        )

    lock_info = None
    if with_lock and not check_only:
        lock_path, _ok, status = generate_lockfile(check_only=False)
        lock_info = {"path": str(lock_path), "status": status}

    return {
        "config": str(config_path),
        "check_only": check_only,
        "target": package or "*",
        "updated": updated,
        "unchanged": unchanged,
        "skipped": skipped,
        "items": items,
        "lock": lock_info,
    }


def _find_pyproject(start_dir: Path | None = None) -> Path:
    base = (start_dir or Path.cwd()).resolve()
    for candidate_dir in (base, *base.parents):
        candidate = candidate_dir / PYPROJECT_NAME
        if candidate.exists():
            return candidate
    raise RuntimeError(f"Fant ikke {PYPROJECT_NAME} i denne mappen eller overliggende mapper")


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


def _resolve_source_path(source_file: str) -> Path:
    path = Path(source_file).expanduser().resolve()
    if not path.exists():
        raise RuntimeError(f"Fant ikke kildefil: {path}")
    return path


def load_program(source_file: str):
    source_path = _resolve_source_path(source_file)
    loader = ModuleLoader(source_path.parent)
    loaded = loader.load_entry_file(source_path.name)

    if isinstance(loaded, tuple):
        program, alias_map = loaded
    else:
        program, alias_map = loaded, {}

    return source_path, program, alias_map


def lex_source(source_text: str):
    lexer = Lexer(source_text)
    tokens = []
    while True:
        tok = lexer.next_token()
        tokens.append(tok)
        if tok.typ == "EOF":
            break
    return tokens


def parse_source(source_text: str):
    parser = Parser(Lexer(source_text))
    return parser.parse()


def _ast_to_data(value):
    if value is None or isinstance(value, (str, int, float, bool)):
        return value
    if isinstance(value, list):
        return [_ast_to_data(item) for item in value]
    if hasattr(value, "__dict__"):
        data = {"node": value.__class__.__name__}
        for key, item in value.__dict__.items():
            data[key] = _ast_to_data(item)
        return data
    return repr(value)


def debug_source(source_file: str, show_tokens: bool, show_ast: bool, show_symbols: bool):
    source_path = _resolve_source_path(source_file)
    source_text = source_path.read_text(encoding="utf-8")
    parsed_program = parse_source(source_text)

    payload = {
        "source": str(source_path),
        "imports": [
            {"module": imp.module_name, "alias": imp.alias}
            for imp in getattr(parsed_program, "imports", [])
        ],
        "functions": [
            {"name": fn.name, "params": len(getattr(fn, "params", []))}
            for fn in getattr(parsed_program, "functions", [])
        ],
    }

    if show_tokens:
        payload["tokens"] = [
            {"type": tok.typ, "value": tok.value, "line": tok.line, "column": tok.column}
            for tok in lex_source(source_text)
        ]

    if show_ast:
        payload["ast"] = _ast_to_data(parsed_program)

    if show_symbols:
        _src, _program, alias_map, analyzer = check_program(source_file)
        payload["aliases"] = alias_map
        symbols = []
        for name, meta in sorted(analyzer.functions.items(), key=lambda item: item[0]):
            module_name = getattr(meta, "module_name", None) or "__main__"
            symbols.append(
                {
                    "name": name,
                    "module": module_name,
                    "params": list(getattr(meta, "params", [])),
                    "return_type": getattr(meta, "return_type", None),
                    "builtin": bool(getattr(meta, "builtin", False)),
                }
            )
        payload["symbols"] = symbols

    return payload


def check_program(source_file: str):
    source_path, program, alias_map = load_program(source_file)
    analyzer = SemanticAnalyzer(alias_map=alias_map)
    analyzer.analyze(program)
    return source_path, program, alias_map, analyzer


def build_program(source_file: str):
    source_path, program, alias_map, analyzer = check_program(source_file)

    cgen = CGenerator(analyzer.functions, alias_map=alias_map)
    code = cgen.generate(program)

    c_path = source_path.with_suffix(".c")
    exe_path = source_path.with_suffix("")

    c_path.write_text(code, encoding="utf-8")
    subprocess.run(["clang", str(c_path), "-o", str(exe_path)], check=True)

    return source_path, c_path, exe_path, alias_map, analyzer


def disasm_program(source_file: str):
    source_path, program, alias_map, analyzer = check_program(source_file)
    cgen = CGenerator(analyzer.functions, alias_map=alias_map)
    code = cgen.generate(program)
    return source_path, code


def tokenize_simple(text: str) -> list[str]:
    tokens: list[str] = []
    current: list[str] = []
    in_comment = False

    for ch in text:
        if in_comment:
            if ch == "\n":
                in_comment = False
            continue

        if ch == "#":
            if current:
                tokens.append("".join(current))
                current.clear()
            in_comment = True
            continue

        if ch.isalnum() or ch in "_-":
            current.append(ch)
            continue

        if current:
            tokens.append("".join(current))
            current.clear()

    if current:
        tokens.append("".join(current))

    return tokens


def parse_ir_tokens(tokens: list[str], strict: bool = False) -> list[str]:
    def is_selfhost_int_token(value: str) -> bool:
        try:
            return str(int(value)) == value
        except ValueError:
            return False

    def parse_selfhost_int(value: str) -> int:
        try:
            return int(value)
        except ValueError:
            return 0

    lines: list[str] = []
    i = 0
    pc = 0
    while i < len(tokens):
        op = tokens[i]
        if strict and op not in IR_ALL_OPS:
            raise RuntimeError(f"/* feil: ukjent opcode {op} ved token {i} */")
        if op in IR_OPS_WITH_ARG:
            if i + 1 >= len(tokens):
                raise RuntimeError(f"/* feil: op mangler verdi ved token {i} */")
            arg_token = tokens[i + 1]
            if strict:
                if not is_selfhost_int_token(arg_token):
                    raise RuntimeError(f"/* feil: ugyldig heltallsargument {arg_token} ved token {i + 1} */")
                arg_value = int(arg_token)
            else:
                arg_value = parse_selfhost_int(arg_token)
            lines.append(f"{pc}: {op} {arg_value}")
            i += 2
        else:
            lines.append(f"{pc}: {op}")
            i += 1
        pc += 1
    return lines


def _escape_no_string(value: str) -> str:
    return value.replace("\\", "\\\\").replace('"', '\\"')


def _run_selfhost_disasm(tokens: list[str], strict: bool = False) -> list[str]:
    build_dir = Path("build").resolve()
    build_dir.mkdir(parents=True, exist_ok=True)

    suffix = uuid.uuid4().hex[:8]
    source_path = build_dir / f"ir_disasm_{suffix}.no"
    c_path = source_path.with_suffix(".c")
    exe_path = source_path.with_suffix("")

    token_list = ", ".join(f'"{_escape_no_string(tok)}"' for tok in tokens)
    fn_name = "disasm_fra_tokens_strict" if strict else "disasm_fra_tokens"
    source = (
        "bruk selfhost.compiler som sh\n\n"
        "funksjon start() -> heltall {\n"
        f"    la tokens: liste_tekst = [{token_list}]\n"
        f"    skriv(sh.{fn_name}(tokens))\n"
        "    returner 0\n"
        "}\n"
    )

    try:
        source_path.write_text(source, encoding="utf-8")
        _src, _c, built_exe, _alias_map, _analyzer = build_program(str(source_path))
        result = subprocess.run(
            [str(built_exe.resolve())],
            capture_output=True,
            text=True,
            check=True,
        )
        text = result.stdout.rstrip("\n")
        return [] if not text else text.splitlines()
    finally:
        for path in (source_path, c_path, exe_path):
            try:
                path.unlink()
            except FileNotFoundError:
                pass


def _run_selfhost_parser_disasm_cases(cases: list[str], mode: str) -> list[list[str]]:
    if mode not in {"expression", "script"}:
        raise RuntimeError(f"Ugyldig parsermodus: {mode}")

    build_dir = Path("build").resolve()
    build_dir.mkdir(parents=True, exist_ok=True)

    suffix = uuid.uuid4().hex[:8]
    source_path = build_dir / f"parser_core_{suffix}.no"
    c_path = source_path.with_suffix(".c")
    exe_path = source_path.with_suffix("")

    fn_name = "disasm_uttrykk" if mode == "expression" else "disasm_skript"
    lines = [
        "bruk selfhost.compiler som sh",
        "",
        "funksjon start() -> heltall {",
    ]
    for i, source in enumerate(cases):
        marker = f"__NORCODE_CASE_{i}__"
        lines.append(f'    skriv("{marker}")')
        lines.append(f'    skriv(sh.{fn_name}("{_escape_no_string(source)}"))')
    lines.extend(
        [
            "    returner 0",
            "}",
            "",
        ]
    )
    source = "\n".join(lines)

    try:
        source_path.write_text(source, encoding="utf-8")
        _src, _c, built_exe, _alias_map, _analyzer = build_program(str(source_path))
        result = subprocess.run(
            [str(built_exe.resolve())],
            capture_output=True,
            text=True,
            check=True,
        )
        out_lines = result.stdout.splitlines()
        parsed: list[list[str]] = [[] for _ in cases]
        current = -1
        for line in out_lines:
            if line.startswith("__NORCODE_CASE_") and line.endswith("__"):
                mid = line[len("__NORCODE_CASE_") : -2]
                try:
                    idx = int(mid)
                except ValueError:
                    continue
                if idx < 0 or idx >= len(cases):
                    continue
                current = idx
                continue
            if current >= 0:
                parsed[current].append(line)
        for idx in range(len(parsed)):
            while parsed[idx] and parsed[idx][-1] == "":
                parsed[idx].pop()
        return parsed
    finally:
        for path in (source_path, c_path, exe_path):
            try:
                path.unlink()
            except FileNotFoundError:
                pass


def ir_disasm_source(source_file: str, strict: bool = False, engine: str = "python"):
    source_path = _resolve_source_path(source_file)
    source = source_path.read_text(encoding="utf-8")
    tokens = tokenize_simple(source)
    if engine == "selfhost":
        disasm_lines = _run_selfhost_disasm(tokens, strict=strict)
        if strict and disasm_lines and disasm_lines[0].startswith("/* feil:"):
            raise RuntimeError(disasm_lines[0])
    else:
        disasm_lines = parse_ir_tokens(tokens, strict=strict)
    return source_path, disasm_lines


def ir_disasm_source_captured(source_file: str, strict: bool, engine: str):
    try:
        source_path, lines = ir_disasm_source(source_file, strict=strict, engine=engine)
        return source_path, True, lines, ""
    except Exception as exc:
        source_path = _resolve_source_path(source_file)
        return source_path, False, [], str(exc)


def get_current_git_revision() -> str | None:
    try:
        result = subprocess.run(
            ["git", "rev-parse", "HEAD"],
            capture_output=True,
            text=True,
            check=True,
        )
        revision = result.stdout.strip()
        return revision or None
    except Exception:
        return None


def get_current_git_branch() -> str | None:
    try:
        result = subprocess.run(
            ["git", "rev-parse", "--abbrev-ref", "HEAD"],
            capture_output=True,
            text=True,
            check=True,
        )
        branch = result.stdout.strip()
        return branch or None
    except Exception:
        return None


def get_current_git_dirty_state() -> bool | None:
    try:
        result = subprocess.run(
            ["git", "status", "--porcelain"],
            capture_output=True,
            text=True,
            check=True,
        )
        return bool(result.stdout.strip())
    except Exception:
        return None


def to_short_git_revision(revision: str | None) -> str | None:
    if not revision:
        return None
    return revision[:7]


def get_current_git_exact_tag() -> str | None:
    try:
        result = subprocess.run(
            ["git", "describe", "--tags", "--exact-match"],
            capture_output=True,
            text=True,
            check=True,
        )
        tag = result.stdout.strip()
        return tag or None
    except Exception:
        return None


def get_current_git_origin_url() -> str | None:
    try:
        result = subprocess.run(
            ["git", "config", "--get", "remote.origin.url"],
            capture_output=True,
            text=True,
            check=True,
        )
        url = result.stdout.strip()
        return url or None
    except Exception:
        return None


def get_git_remote_host(remote_url: str | None) -> str | None:
    if not remote_url:
        return None
    parsed = urllib.parse.urlparse(remote_url)
    if parsed.hostname:
        return parsed.hostname
    if "@" in remote_url and ":" in remote_url:
        rhs = remote_url.split("@", 1)[1]
        host = rhs.split(":", 1)[0].strip()
        return host or None
    return None


def get_git_remote_repo_slug(remote_url: str | None) -> str | None:
    if not remote_url:
        return None
    parsed = urllib.parse.urlparse(remote_url)
    path = ""
    if parsed.scheme and parsed.netloc:
        path = parsed.path
    elif "@" in remote_url and ":" in remote_url:
        path = remote_url.split(":", 1)[1]
    cleaned = path.strip().lstrip("/")
    if cleaned.endswith(".git"):
        cleaned = cleaned[:-4]
    parts = [p for p in cleaned.split("/") if p]
    if len(parts) >= 2:
        return "/".join(parts[-2:])
    return None


def get_git_remote_protocol(remote_url: str | None) -> str:
    if not remote_url:
        return "unknown"
    parsed = urllib.parse.urlparse(remote_url)
    if parsed.scheme:
        return parsed.scheme.lower()
    if "@" in remote_url and ":" in remote_url:
        return "ssh"
    return "unknown"


def split_repo_slug(slug: str | None) -> tuple[str | None, str | None]:
    if not slug or "/" not in slug:
        return None, None
    owner, name = slug.split("/", 1)
    owner = owner.strip() or None
    name = name.strip() or None
    return owner, name


def is_github_host(host: str | None) -> bool:
    return bool(host and host.lower() == "github.com")


def is_gitlab_host(host: str | None) -> bool:
    return bool(host and host.lower() == "gitlab.com")


def is_bitbucket_host(host: str | None) -> bool:
    return bool(host and host.lower() == "bitbucket.org")


def get_git_remote_provider(host: str | None) -> str:
    if not host:
        return "unknown"
    normalized = host.lower()
    if normalized == "github.com":
        return "github"
    if normalized == "gitlab.com":
        return "gitlab"
    if normalized == "bitbucket.org":
        return "bitbucket"
    return "unknown"


def get_source_revision_url(remote_host: str | None, repo_slug: str | None, revision: str | None) -> str | None:
    if not remote_host or not repo_slug or not revision:
        return None
    return f"https://{remote_host}/{repo_slug}/commit/{revision}"


def get_source_ref_url(remote_host: str | None, repo_slug: str | None, source_ref: str | None) -> str | None:
    if not remote_host or not repo_slug or not source_ref:
        return None
    return f"https://{remote_host}/{repo_slug}/tree/{source_ref}"


def get_source_repo_url(remote_host: str | None, repo_slug: str | None) -> str | None:
    if not remote_host or not repo_slug:
        return None
    return f"https://{remote_host}/{repo_slug}"


def run_program(source_file: str):
    source_path, c_path, exe_path, _alias_map, _analyzer = build_program(source_file)
    print(f"Generert C-fil: {c_path}")
    print("Kompilert med: clang")
    print(f"Kjører: {exe_path}")
    subprocess.run([str(exe_path.resolve())], check=True)
    return source_path


def _decode_text_map(value: object) -> dict[str, str]:
    if isinstance(value, dict):
        return {str(key): str(item) for key, item in value.items()}
    if isinstance(value, str) and value.strip():
        try:
            parsed = json.loads(value)
        except Exception:
            return {}
        if isinstance(parsed, dict):
            return {str(key): str(item) for key, item in parsed.items()}
    return {}


def _normalize_serve_response(response: object) -> tuple[int, dict[str, str], bytes]:
    status = 200
    headers: dict[str, str] = {}
    body: object = response
    if isinstance(response, dict):
        try:
            status = int(str(response.get("status", 200)).strip() or "200")
        except Exception:
            status = 200
        headers = _decode_text_map(response.get("headers", {}))
        body = response.get("body", "")
    if isinstance(body, bytes):
        body_bytes = body
    elif body is None:
        body_bytes = b""
    else:
        body_bytes = str(body).encode("utf-8")
    if not any(key.lower() == "content-type" for key in headers):
        headers["content-type"] = "text/plain; charset=utf-8"
    headers["content-length"] = str(len(body_bytes))
    return status, headers, body_bytes


class _ServeRuntime:
    def __init__(self, source_file: str, reload_enabled: bool = False):
        self.source_file = str(source_file)
        self.reload_enabled = reload_enabled
        self.lock = threading.RLock()
        self.source_path: Path | None = None
        self.vm = None
        self.source_mtime_ns = 0
        self._load_initial()

    def _compile(self):
        from compiler.bytecode_backend import BytecodeVM, compile_source_to_bytecode

        source_path, payload = compile_source_to_bytecode(self.source_file)
        vm = BytecodeVM(payload)
        vm.call_function("web.startup", [])
        return source_path, vm

    def _load_initial(self):
        source_path, vm = self._compile()
        self.source_path = source_path
        self.vm = vm
        try:
            self.source_mtime_ns = source_path.stat().st_mtime_ns
        except FileNotFoundError:
            self.source_mtime_ns = 0

    def _reload_if_needed(self):
        if not self.reload_enabled or self.source_path is None or self.vm is None:
            return
        try:
            current_mtime = self.source_path.stat().st_mtime_ns
        except FileNotFoundError:
            return
        if current_mtime <= self.source_mtime_ns:
            return
        from compiler.bytecode_backend import BytecodeVM, compile_source_to_bytecode

        source_path, payload = compile_source_to_bytecode(self.source_file)
        new_vm = BytecodeVM(payload)
        new_vm.call_function("web.startup", [])
        old_vm = self.vm
        self.vm = new_vm
        self.source_path = source_path
        self.source_mtime_ns = current_mtime
        try:
            old_vm.call_function("web.shutdown", [])
        except Exception as exc:
            print(f"Advarsel: shutdown ved reload feilet: {exc}", file=sys.stderr)

    def handle(self, method: str, path: str, query: dict[str, str], headers: dict[str, str], body: str):
        from compiler.bytecode_backend import BytecodeVM

        with self.lock:
            self._reload_if_needed()
            if not isinstance(self.vm, BytecodeVM):
                raise RuntimeError("Serverruntime er ikke klar")
            ctx = self.vm.call_function("web.request_context", [method, path, query, headers, body])
            return self.vm.call_function("web.handle_request", [ctx])

    def shutdown(self):
        with self.lock:
            if self.vm is None:
                return 0
            try:
                return self.vm.call_function("web.shutdown", [])
            finally:
                self.vm = None


class _NorscodeThreadingHTTPServer(ThreadingHTTPServer):
    daemon_threads = True
    allow_reuse_address = True


def serve_program(source_file: str, host: str = "127.0.0.1", port: int = 8000, reload_enabled: bool = False, once: bool = False):
    runtime = _ServeRuntime(source_file, reload_enabled=reload_enabled)

    class Handler(BaseHTTPRequestHandler):
        def _dispatch(self):
            parsed = urllib.parse.urlsplit(self.path)
            query = {str(key): str(value) for key, value in urllib.parse.parse_qsl(parsed.query, keep_blank_values=True)}
            headers = {str(key): str(value) for key, value in self.headers.items()}
            length_text = self.headers.get("Content-Length", "0")
            try:
                length = max(0, int(length_text))
            except Exception:
                length = 0
            body_bytes = self.rfile.read(length) if length else b""
            body = body_bytes.decode("utf-8", errors="replace")
            try:
                response = runtime.handle(self.command, parsed.path or "/", query, headers, body)
                status, response_headers, response_body = _normalize_serve_response(response)
            except Exception as exc:
                status = 500
                response_headers = {"content-type": "application/json; charset=utf-8"}
                response_body = json.dumps({"error": str(exc)}, ensure_ascii=False).encode("utf-8")
            self.send_response(status)
            for key, value in response_headers.items():
                if key.lower() == "content-length":
                    continue
                self.send_header(key, value)
            self.send_header("Content-Length", str(len(response_body)))
            self.end_headers()
            if self.command != "HEAD":
                self.wfile.write(response_body)
            if once:
                threading.Thread(target=self.server.shutdown, daemon=True).start()

        def do_GET(self):
            self._dispatch()

        def do_HEAD(self):
            self._dispatch()

        def do_POST(self):
            self._dispatch()

        def do_PUT(self):
            self._dispatch()

        def do_PATCH(self):
            self._dispatch()

        def do_DELETE(self):
            self._dispatch()

        def do_OPTIONS(self):
            self._dispatch()

        def log_message(self, format, *args):
            print(f"[serve] {self.address_string()} - {format % args}", file=sys.stderr)

    server = _NorscodeThreadingHTTPServer((host, port), Handler)
    bind_host, bind_port = server.server_address
    print(f"Starter Norscode server fra {Path(source_file).expanduser().resolve()}")
    print(f"Lytter på http://{bind_host}:{bind_port}")
    if reload_enabled:
        print("Reload: på")
    try:
        server.serve_forever()
    except KeyboardInterrupt:
        print()
    finally:
        runtime.shutdown()
        server.server_close()


def _try_parse_expression(source_text: str):
    parser = Parser(Lexer(source_text))
    try:
        expr = parser.expr()
    except Exception:
        return None
    if parser.current.typ != "EOF":
        return None
    return expr


def _indent_repl_body(lines: list[str]) -> list[str]:
    indented: list[str] = []
    for line in lines:
        if line.strip():
            indented.append(f"    {line}")
        else:
            indented.append("")
    return indented


def _build_repl_source(imports: list[str], chunk: str) -> tuple[str, bool]:
    expr = _try_parse_expression(chunk)
    if expr is not None:
        body_lines = [f"    skriv({chunk.strip()})", "    returner 0"]
        is_expr = True
    else:
        body_lines = _indent_repl_body(chunk.splitlines())
        body_lines.append("    returner 0")
        is_expr = False

    source_lines = []
    source_lines.extend(imports)
    if imports:
        source_lines.append("")
    source_lines.append("funksjon start() -> heltall {")
    source_lines.extend(body_lines)
    source_lines.append("}")
    source_lines.append("")
    return "\n".join(source_lines), is_expr


def _run_repl_source(source_text: str):
    suffix = uuid.uuid4().hex[:8]
    source_path = Path.cwd() / f".norscode_repl_{suffix}.no"
    c_path = source_path.with_suffix(".c")
    exe_path = source_path.with_suffix("")
    try:
        source_path.write_text(source_text, encoding="utf-8")
        _source_path, c_path, exe_path, _alias_map, _analyzer = build_program(str(source_path))
        result = subprocess.run(
            [str(exe_path.resolve())],
            capture_output=True,
            text=True,
        )
        return {
            "source": str(source_path),
            "c_file": str(c_path),
            "exe_file": str(exe_path),
            "returncode": result.returncode,
            "stdout": result.stdout,
            "stderr": result.stderr,
        }
    finally:
        for path in (source_path, c_path, exe_path):
            try:
                path.unlink()
            except FileNotFoundError:
                pass


def run_repl():
    print("Norscode REPL")
    print("Skriv linjer og avslutt blokken med en tom linje.")
    print("Kommandoer: :quit, :exit, :reset, :imports")
    imports: list[str] = []
    buffer: list[str] = []

    while True:
        prompt = ">>> " if not buffer else "... "
        try:
            line = input(prompt)
        except EOFError:
            print()
            break

        stripped = line.strip()
        if not buffer and stripped in {":quit", ":exit", ":q"}:
            break
        if not buffer and stripped == ":reset":
            imports.clear()
            print("Session nullstilt.")
            continue
        if not buffer and stripped == ":imports":
            if not imports:
                print("Ingen importlinjer enda.")
            else:
                for item in imports:
                    print(item)
            continue

        if stripped == "":
            if not buffer:
                continue
            chunk = "\n".join(buffer).rstrip()
            buffer.clear()

            if not chunk:
                continue
            if chunk.lstrip().startswith("bruk "):
                imports.append(chunk)
                print("Import lagt til.")
                continue

            source_text, is_expr = _build_repl_source(imports, chunk)
            try:
                result = _run_repl_source(source_text)
            except Exception as exc:
                print(f"Feil: {exc}")
                continue

            stdout = result["stdout"]
            stderr = result["stderr"]
            if stdout:
                end = "" if stdout.endswith("\n") else "\n"
                print(stdout, end=end)
            if stderr:
                end = "" if stderr.endswith("\n") else "\n"
                print(stderr, end=end, file=sys.stderr)
            if result["returncode"] != 0:
                print(f"[exit {result['returncode']}]")
            elif is_expr and not stdout:
                print("OK")
            continue

        buffer.append(line)


def discover_tests() -> list[Path]:
    tests_dir = Path("tests").resolve()
    if not tests_dir.exists():
        return []
    return sorted(p.resolve() for p in tests_dir.rglob("test_*.no"))


def run_test_file(source_file: str):
    started = time.perf_counter()
    source_path, c_path, exe_path, _alias_map, _analyzer = build_program(source_file)

    result = subprocess.run(
        [str(exe_path.resolve())],
        capture_output=True,
        text=True,
    )
    duration_ms = int((time.perf_counter() - started) * 1000)

    return {
        "source": str(source_path),
        "c_file": str(c_path),
        "exe_file": str(exe_path),
        "returncode": result.returncode,
        "stdout": result.stdout,
        "stderr": result.stderr,
        "success": result.returncode == 0,
        "duration_ms": duration_ms,
    }


def print_test_result(result, verbose: bool = False):
    status = "OK" if result["success"] else "FEIL"
    duration_ms = result.get("duration_ms")
    if isinstance(duration_ms, int):
        print(f"{status}: {result['source']} ({duration_ms} ms)")
    else:
        print(f"{status}: {result['source']}")

    if verbose or not result["success"]:
        if result["stdout"]:
            print("STDOUT:")
            print(result["stdout"], end="" if result["stdout"].endswith("\n") else "\n")
        if result["stderr"]:
            print("STDERR:")
            print(result["stderr"], end="" if result["stderr"].endswith("\n") else "\n")


def _collect_lint_issues(program, alias_map: dict[str, str] | None = None):
    issues: list[dict] = []

    imports = list(getattr(program, "imports", []))
    if not imports and alias_map:
        imports = [SimpleNamespace(module_name=module_name, alias=alias) for alias, module_name in alias_map.items()]
    used_modules: set[str] = set()
    seen_import_keys: set[str] = set()

    for imp in imports:
        key = imp.alias or imp.module_name
        if key in seen_import_keys:
            issues.append(
                {
                    "severity": "warning",
                    "code": "duplicate-import",
                    "location": key,
                    "message": f"Dobbel import: {key}",
                }
            )
        else:
            seen_import_keys.add(key)

    def visit_expr(expr):
        if expr is None:
            return
        if isinstance(expr, ModuleCallNode):
            used_modules.add(expr.module_name)
            for arg in expr.args:
                visit_expr(arg)
            return
        if isinstance(expr, CallNode):
            for arg in expr.args:
                visit_expr(arg)
            return
        if isinstance(expr, IfExprNode):
            visit_expr(expr.condition)
            visit_expr(expr.then_expr)
            visit_expr(expr.else_expr)
            return
        if isinstance(expr, AwaitNode):
            visit_expr(expr.expr)
            return
        if isinstance(expr, UnaryOpNode):
            visit_expr(expr.node)
            return
        if isinstance(expr, BinOpNode):
            visit_expr(expr.left)
            visit_expr(expr.right)
            return
        if isinstance(expr, IndexNode):
            visit_expr(expr.list_expr)
            visit_expr(expr.index_expr)
            return
        if isinstance(expr, SliceNode):
            visit_expr(expr.target)
            visit_expr(expr.start_expr)
            visit_expr(expr.end_expr)
            return
        if isinstance(expr, FieldAccessNode):
            visit_expr(expr.target)
            return
        if isinstance(expr, ListLiteralNode):
            for item in expr.items:
                visit_expr(item)
            return
        if isinstance(expr, MapLiteralNode):
            for key_expr, value_expr in expr.items:
                visit_expr(key_expr)
                visit_expr(value_expr)
            return
        if isinstance(expr, StructLiteralNode):
            for _field_name, value_expr in expr.fields:
                visit_expr(value_expr)
            return

    def visit_stmt(stmt, function_name: str, in_loop: bool):
        if isinstance(stmt, VarDeclareNode):
            visit_expr(stmt.expr)
            return False
        if isinstance(stmt, VarSetNode):
            visit_expr(stmt.expr)
            return False
        if isinstance(stmt, IndexSetNode):
            visit_expr(stmt.index_expr)
            visit_expr(stmt.value_expr)
            return False
        if isinstance(stmt, PrintNode):
            visit_expr(stmt.expr)
            return False
        if isinstance(stmt, IfNode):
            visit_expr(stmt.condition)
            visit_block(stmt.then_block, function_name, in_loop)
            for elif_cond, elif_block in getattr(stmt, "elif_blocks", []):
                visit_expr(elif_cond)
                visit_block(elif_block, function_name, in_loop)
            if stmt.else_block:
                visit_block(stmt.else_block, function_name, in_loop)
            return False
        if isinstance(stmt, IfExprNode):
            visit_expr(stmt.condition)
            visit_expr(stmt.then_expr)
            visit_expr(stmt.else_expr)
            return False
        if isinstance(stmt, WhileNode):
            visit_expr(stmt.condition)
            visit_block(stmt.body, function_name, True)
            return False
        if isinstance(stmt, ForNode):
            visit_expr(stmt.start_expr)
            visit_expr(stmt.end_expr)
            visit_expr(stmt.step_expr)
            visit_block(stmt.body, function_name, True)
            return False
        if isinstance(stmt, ForEachNode):
            visit_expr(stmt.list_expr)
            visit_block(stmt.body, function_name, True)
            return False
        if isinstance(stmt, ReturnNode):
            visit_expr(stmt.expr)
            return True
        if isinstance(stmt, ThrowNode):
            visit_expr(stmt.expr)
            return True
        if isinstance(stmt, TryCatchNode):
            visit_block(stmt.try_block, function_name, in_loop)
            visit_block(stmt.catch_block, function_name, in_loop)
            return False
        if isinstance(stmt, ExprStmtNode):
            visit_expr(stmt.expr)
            return False
        if isinstance(stmt, (BreakNode, ContinueNode)):
            return True
        return False

    def visit_block(block, function_name: str, in_loop: bool):
        dead = False
        for stmt in getattr(block, "statements", []):
            if dead:
                issues.append(
                    {
                        "severity": "warning",
                        "code": "unreachable-code",
                        "location": function_name,
                        "message": f"Uoppnåelig kode etter terminal statement i funksjon '{function_name}'",
                    }
                )
                break
            dead = visit_stmt(stmt, function_name, in_loop)

    for fn in getattr(program, "functions", []):
        visit_block(fn.body, fn.name, False)

    for test in getattr(program, "tests", []):
        visit_block(test.body, test.name, False)

    for imp in imports:
        key = imp.alias or imp.module_name
        if key not in used_modules:
            issues.append(
                {
                    "severity": "warning",
                    "code": "unused-import",
                    "location": key,
                    "message": f"Ubrukt import: {imp.module_name}" + (f" som {imp.alias}" if imp.alias else ""),
                }
            )

    return issues


def lint_program(source_file: str):
    source_path, program, alias_map = load_program(source_file)
    issues = _collect_lint_issues(program, alias_map=alias_map)
    return {
        "source": str(source_path),
        "alias_map": alias_map,
        "issues": issues,
        "success": len(issues) == 0,
    }


def print_lint_result(result, verbose: bool = False):
    issues = result.get("issues", [])
    status = "OK" if not issues else "FEIL"
    print(f"{status}: {result['source']} ({len(issues)} funn)")
    if verbose or issues:
        for issue in issues:
            location = issue.get("location")
            prefix = f"{issue.get('severity', 'warning').upper()} {issue.get('code', 'lint')}"
            if location:
                print(f"- {prefix}: {location} -> {issue.get('message', '')}")
            else:
                print(f"- {prefix}: {issue.get('message', '')}")


def format_program_file(source_file: str, check: bool = False, diff: bool = False):
    source_path = _resolve_source_path(source_file)
    original = source_path.read_text(encoding="utf-8")
    formatted = format_source(original)
    changed = formatted != original
    diff_lines = None

    if diff:
        diff_lines = list(
            difflib.unified_diff(
                original.splitlines(),
                formatted.splitlines(),
                fromfile=str(source_path),
                tofile=f"{source_path} (formatted)",
                lineterm="",
            )
        )
        if diff_lines:
            print("\n".join(diff_lines))
        else:
            print(f"Ingen endringer for {source_path}")

    if check:
        return {
            "source": str(source_path),
            "changed": changed,
            "written": False,
            "diff": diff_lines,
        }

    if changed and not diff:
        source_path.write_text(formatted, encoding="utf-8")

    return {
        "source": str(source_path),
        "changed": changed,
        "written": changed and not diff,
        "diff": diff_lines,
    }


def run_ir_snapshot_checks():
    started = time.perf_counter()
    fixture_path = IR_SNAPSHOT_FIXTURE.resolve()
    try:
        fixture = json.loads(fixture_path.read_text(encoding="utf-8"))
        cases = [(item["name"], item["source"]) for item in fixture.get("non_strict", [])]
        strict_cases = fixture.get("strict", [])
    except Exception as exc:
        return {
            "source": "IR snapshot parity (python vs selfhost)",
            "c_file": "",
            "exe_file": "",
            "returncode": 1,
            "stdout": "",
            "stderr": f"Kunne ikke lese fixture {fixture_path}: {exc}\n",
            "success": False,
            "duration_ms": int((time.perf_counter() - started) * 1000),
        }

    mismatch_lines: list[str] = []

    for name, src in cases:
        tokens = tokenize_simple(src)
        py_lines = parse_ir_tokens(tokens, strict=False)
        sh_lines = _run_selfhost_disasm(tokens, strict=False)
        if py_lines != sh_lines:
            mismatch_lines.append(f"[{name}] non-strict mismatch")
            mismatch_lines.append("--- python")
            mismatch_lines.append("+++ selfhost")
            mismatch_lines.extend(
                difflib.unified_diff(py_lines, sh_lines, fromfile="python", tofile="selfhost", lineterm="")
            )

    for item in strict_cases:
        name = item.get("name", "strict_case")
        src = item.get("source", "")
        expected_error = item.get("expected_error")
        expected_lines = item.get("expected_lines")

        tokens = tokenize_simple(src)
        py_ok = True
        py_lines: list[str] = []
        py_err = ""
        try:
            py_lines = parse_ir_tokens(tokens, strict=True)
        except Exception as exc:
            py_ok = False
            py_err = str(exc)

        sh_ok = True
        sh_lines: list[str] = []
        sh_err = ""
        try:
            sh_lines = _run_selfhost_disasm(tokens, strict=True)
            if sh_lines and sh_lines[0].startswith("/* feil:"):
                sh_ok = False
                sh_err = sh_lines[0]
                sh_lines = []
        except Exception as exc:
            sh_ok = False
            sh_err = str(exc)

        if py_ok != sh_ok or py_lines != sh_lines or py_err != sh_err:
            mismatch_lines.append(f"[{name}] strict mismatch")
            mismatch_lines.append(f"python: {'OK ' + repr(py_lines) if py_ok else py_err}")
            mismatch_lines.append(f"selfhost: {'OK ' + repr(sh_lines) if sh_ok else sh_err}")

        if expected_error is not None:
            if py_ok:
                mismatch_lines.append(f"[{name}] strict expected error but got success")
                mismatch_lines.append(f"python: OK {repr(py_lines)}")
            elif py_err != expected_error:
                mismatch_lines.append(f"[{name}] strict expected error mismatch")
                mismatch_lines.append(f"expected: {expected_error}")
                mismatch_lines.append(f"actual:   {py_err}")

        if expected_lines is not None:
            if not py_ok:
                mismatch_lines.append(f"[{name}] strict expected lines but got error")
                mismatch_lines.append(f"python error: {py_err}")
            elif py_lines != expected_lines:
                mismatch_lines.append(f"[{name}] strict expected lines mismatch")
                mismatch_lines.extend(
                    difflib.unified_diff(expected_lines, py_lines, fromfile="expected", tofile="actual", lineterm="")
                )

    success = len(mismatch_lines) == 0
    return {
        "source": "IR snapshot parity (python vs selfhost)",
        "c_file": "",
        "exe_file": "",
        "returncode": 0 if success else 1,
        "stdout": "",
        "stderr": "" if success else "\n".join(mismatch_lines) + "\n",
        "success": success,
        "duration_ms": int((time.perf_counter() - started) * 1000),
    }


def run_selfhost_parser_core_checks(fixture_path: Path, label: str):
    started = time.perf_counter()
    fixture_path = fixture_path.resolve()
    try:
        fixture = json.loads(fixture_path.read_text(encoding="utf-8"))
    except Exception as exc:
        return {
            "source": label,
            "c_file": "",
            "exe_file": "",
            "returncode": 1,
            "stdout": "",
            "stderr": f"Kunne ikke lese fixture {fixture_path}: {exc}\n",
            "success": False,
            "duration_ms": int((time.perf_counter() - started) * 1000),
            "case_count": 0,
            "expression_cases": 0,
            "script_cases": 0,
            "line_cases": 0,
            "error_cases": 0,
        }

    expression_cases = fixture.get("expressions", [])
    script_cases = fixture.get("scripts", [])
    expression_count = len(expression_cases)
    script_count = len(script_cases)
    line_case_count = 0
    error_case_count = 0
    mismatch_lines: list[str] = []

    def _validate_and_collect(cases: list[dict], mode: str):
        nonlocal line_case_count, error_case_count
        if not cases:
            return
        seen_names: set[str] = set()
        for idx, item in enumerate(cases):
            has_expected_lines = "expected_lines" in item
            has_expected_error = "expected_error" in item
            if "source" not in item:
                mismatch_lines.append(f"[{mode}#{idx}] mangler source i fixture")
                return
            case_name = str(item.get("name") or f"{mode}_{idx}")
            if case_name in seen_names:
                mismatch_lines.append(f"[{mode}#{idx}] duplikat casenavn: {case_name}")
                return
            seen_names.add(case_name)
            if has_expected_lines == has_expected_error:
                mismatch_lines.append(
                    f"[{mode}#{idx}] må ha nøyaktig ett av feltene expected_lines eller expected_error"
                )
                return
            if has_expected_error:
                error_case_count += 1
            else:
                line_case_count += 1
        sources = [str(item["source"]) for item in cases]
        actual_lists = _run_selfhost_parser_disasm_cases(sources, mode=mode)
        if len(actual_lists) != len(cases):
            mismatch_lines.append(f"[{mode}] intern feil: antall resultater avviker")
            return
        for idx, item in enumerate(cases):
            name = str(item.get("name") or f"{mode}_{idx}")
            actual_lines = actual_lists[idx]
            expected_error = item.get("expected_error")
            if expected_error is not None:
                actual_error = actual_lines[0] if actual_lines else ""
                if actual_error != str(expected_error):
                    mismatch_lines.append(f"[{name}] error mismatch")
                    mismatch_lines.append(f"expected: {expected_error}")
                    mismatch_lines.append(f"actual:   {actual_error}")
                continue

            expected_lines = [str(line) for line in item.get("expected_lines", [])]
            if expected_lines != actual_lines:
                mismatch_lines.append(f"[{name}] mismatch")
                mismatch_lines.extend(
                    difflib.unified_diff(
                        expected_lines,
                        actual_lines,
                        fromfile=f"{name}/expected",
                        tofile=f"{name}/actual",
                        lineterm="",
                    )
                )

    try:
        _validate_and_collect(expression_cases, "expression")
        _validate_and_collect(script_cases, "script")
    except Exception as exc:
        mismatch_lines.append(f"Kjøring feilet: {exc}")

    success = len(mismatch_lines) == 0
    return {
        "source": label,
        "c_file": "",
        "exe_file": "",
        "returncode": 0 if success else 1,
        "stdout": "",
        "stderr": "" if success else "\n".join(mismatch_lines) + "\n",
        "success": success,
        "duration_ms": int((time.perf_counter() - started) * 1000),
        "case_count": expression_count + script_count,
        "expression_cases": expression_count,
        "script_cases": script_count,
        "line_cases": line_case_count,
        "error_cases": error_case_count,
    }


def run_selfhost_parser_parity(suite: str = "all") -> dict:
    suites = _selfhost_parity_suite_targets(suite)
    started = time.perf_counter()
    results: list[dict] = []
    for fixture_path, label in suites:
        results.append(run_selfhost_parser_core_checks(fixture_path, label))

    ok = all(item.get("success") for item in results)
    return {
        "suite": suite,
        "ok": ok,
        "case_count": sum(int(item.get("case_count", 0) or 0) for item in results),
        "expression_cases": sum(int(item.get("expression_cases", 0) or 0) for item in results),
        "script_cases": sum(int(item.get("script_cases", 0) or 0) for item in results),
        "line_cases": sum(int(item.get("line_cases", 0) or 0) for item in results),
        "error_cases": sum(int(item.get("error_cases", 0) or 0) for item in results),
        "duration_ms": int((time.perf_counter() - started) * 1000),
        "results": results,
    }


def _count_selfhost_parity_cases(fixture: dict) -> dict:
    expression_cases = fixture.get("expressions", [])
    script_cases = fixture.get("scripts", [])
    all_cases = [*expression_cases, *script_cases]
    error_cases = sum(1 for item in all_cases if "expected_error" in item)
    line_cases = len(all_cases) - error_cases
    return {
        "case_count": len(all_cases),
        "expression_cases": len(expression_cases),
        "script_cases": len(script_cases),
        "line_cases": line_cases,
        "error_cases": error_cases,
    }


def run_selfhost_parity_progress() -> dict:
    started = time.perf_counter()
    mismatch_lines: list[str] = []
    m1_fixture = _load_selfhost_parity_fixture(SELFHOST_PARSER_M1_FIXTURE, mismatch_lines)
    m2_fixture = _load_selfhost_parity_fixture(SELFHOST_PARSER_M2_FIXTURE, mismatch_lines)
    ext_fixture = _load_selfhost_parity_fixture(SELFHOST_PARSER_EXTENDED_FIXTURE, mismatch_lines)
    if m1_fixture is None or m2_fixture is None or ext_fixture is None:
        return {
            "ok": False,
            "duration_ms": int((time.perf_counter() - started) * 1000),
            "stderr": "\n".join(mismatch_lines) + "\n",
        }

    m1_maps, _ = _selfhost_parity_fixture_case_maps(m1_fixture, "m1", mismatch_lines)
    m2_maps, _ = _selfhost_parity_fixture_case_maps(m2_fixture, "m2", mismatch_lines)
    ext_maps, _ = _selfhost_parity_fixture_case_maps(ext_fixture, "extended", mismatch_lines)

    overlap_expr = sorted(set(m1_maps["expressions"].keys()) & set(m2_maps["expressions"].keys()))
    overlap_scripts = sorted(set(m1_maps["scripts"].keys()) & set(m2_maps["scripts"].keys()))
    overlap_total = len(overlap_expr) + len(overlap_scripts)

    union_expr = set(m1_maps["expressions"].keys()) | set(m2_maps["expressions"].keys())
    union_scripts = set(m1_maps["scripts"].keys()) | set(m2_maps["scripts"].keys())
    ext_expr = set(ext_maps["expressions"].keys())
    ext_scripts = set(ext_maps["scripts"].keys())
    union_total = len(union_expr) + len(union_scripts)
    ext_total = len(ext_expr) + len(ext_scripts)

    missing_expr = sorted(ext_expr - union_expr)
    missing_scripts = sorted(ext_scripts - union_scripts)
    missing_total = len(missing_expr) + len(missing_scripts)

    extra_expr = sorted(union_expr - ext_expr)
    extra_scripts = sorted(union_scripts - ext_scripts)
    extra_total = len(extra_expr) + len(extra_scripts)

    consistency = run_selfhost_parser_suite_all_consistency_check(
        SELFHOST_PARSER_M1_FIXTURE,
        SELFHOST_PARSER_M2_FIXTURE,
        SELFHOST_PARSER_EXTENDED_FIXTURE,
    )

    def _pct(part: int, total: int) -> float:
        if total <= 0:
            return 100.0
        return round((part / total) * 100.0, 2)

    coverage_expr = _pct(len(union_expr), len(ext_expr))
    coverage_scripts = _pct(len(union_scripts), len(ext_scripts))
    coverage_total = _pct(union_total, ext_total)
    m1_total = len(m1_maps["expressions"]) + len(m1_maps["scripts"])
    m2_total = len(m2_maps["expressions"]) + len(m2_maps["scripts"])
    coverage_m1 = _pct(m1_total, ext_total)
    coverage_m2 = _pct(m2_total, ext_total)

    ready = (
        consistency.get("success")
        and missing_total == 0
        and extra_total == 0
        and overlap_total == 0
    )
    ok = ready and not mismatch_lines
    return {
        "ok": ok,
        "ready": bool(ready),
        "duration_ms": int((time.perf_counter() - started) * 1000),
        "m1": {
            **_count_selfhost_parity_cases(m1_fixture),
            "coverage_total_pct": coverage_m1,
        },
        "m2": {
            **_count_selfhost_parity_cases(m2_fixture),
            "coverage_total_pct": coverage_m2,
        },
        "extended": _count_selfhost_parity_cases(ext_fixture),
        "coverage": {
            "expression_pct": coverage_expr,
            "script_pct": coverage_scripts,
            "total_pct": coverage_total,
            "union_case_count": union_total,
            "extended_case_count": ext_total,
            "missing_in_m1_m2_count": missing_total,
            "extra_in_m1_m2_count": extra_total,
            "overlap_count": overlap_total,
            "missing_in_m1_m2_examples": (missing_expr + missing_scripts)[:10],
            "extra_in_m1_m2_examples": (extra_expr + extra_scripts)[:10],
            "overlap_examples": (overlap_expr + overlap_scripts)[:10],
        },
        "consistency": {
            "ok": bool(consistency.get("success")),
            "checked_cases": int(consistency.get("checked_cases", 0) or 0),
            "mismatch_count": int(consistency.get("mismatch_count", 0) or 0),
        },
        "stderr": "" if ok else (consistency.get("stderr", "") or "\n".join(mismatch_lines) + "\n"),
    }


def run_selfhost_parity_gate(min_coverage: float | None = None) -> dict:
    progress = run_selfhost_parity_progress()
    coverage_total = None
    if isinstance(progress.get("coverage"), dict):
        coverage_total = progress["coverage"].get("total_pct")

    ready = bool(progress.get("ready"))
    if min_coverage is not None:
        ready = ready and isinstance(coverage_total, (int, float)) and float(coverage_total) >= float(min_coverage)

    return {
        "ok": bool(progress.get("ok")) and ready,
        "ready": ready,
        "min_coverage": min_coverage,
        "coverage_total_pct": coverage_total,
        "progress": progress,
    }


def _load_selfhost_parity_fixture(path: Path, mismatch_lines: list[str]) -> dict | None:
    try:
        return json.loads(path.resolve().read_text(encoding="utf-8"))
    except Exception as exc:
        mismatch_lines.append(f"Kunne ikke lese fixture {path.resolve()}: {exc}")
        return None


def _normalize_selfhost_parity_case(item: dict) -> dict:
    normalized = {"source": str(item.get("source", ""))}
    has_lines = "expected_lines" in item
    has_error = "expected_error" in item
    if has_lines == has_error:
        normalized["invalid"] = True
        return normalized
    if has_error:
        normalized["expected_error"] = str(item.get("expected_error", ""))
    else:
        normalized["expected_lines"] = [str(line) for line in item.get("expected_lines", [])]
    return normalized


def _selfhost_parity_fixture_case_maps(
    fixture: dict,
    fixture_tag: str,
    mismatch_lines: list[str],
) -> tuple[dict[str, dict[str, dict]], int]:
    maps: dict[str, dict[str, dict]] = {"expressions": {}, "scripts": {}}
    checked = 0
    for mode in ("expressions", "scripts"):
        cases = fixture.get(mode, [])
        mode_map = maps[mode]
        for idx, item in enumerate(cases):
            if "name" not in item:
                mismatch_lines.append(f"[{fixture_tag}/{mode}#{idx}] mangler navn")
                continue
            name = str(item["name"])
            if name in mode_map:
                mismatch_lines.append(f"[{fixture_tag}/{mode}#{idx}] duplikat navn: {name}")
                continue
            mode_map[name] = _normalize_selfhost_parity_case(item)
            checked += 1
    return maps, checked


def run_selfhost_parser_suite_subset_consistency_check(
    subset_fixture: Path,
    extended_fixture: Path,
    subset_tag: str,
) -> dict:
    started = time.perf_counter()
    mismatch_lines: list[str] = []

    subset = _load_selfhost_parity_fixture(subset_fixture, mismatch_lines)
    extended = _load_selfhost_parity_fixture(extended_fixture, mismatch_lines)
    if subset is None or extended is None:
        return {
            "success": False,
            "checked_cases": 0,
            "mismatch_count": len(mismatch_lines),
            "stderr": "\n".join(mismatch_lines) + "\n",
            "duration_ms": int((time.perf_counter() - started) * 1000),
            "scope": subset_tag,
        }

    subset_maps, checked = _selfhost_parity_fixture_case_maps(subset, subset_tag, mismatch_lines)
    ext_maps, _ = _selfhost_parity_fixture_case_maps(extended, "extended", mismatch_lines)

    for mode in ("expressions", "scripts"):
        for name, subset_norm in subset_maps[mode].items():
            ext_norm = ext_maps[mode].get(name)
            if ext_norm is None:
                mismatch_lines.append(f"[{subset_tag}/{mode}/{name}] finnes ikke i utvidet fixture")
                continue
            if subset_norm.get("invalid") or ext_norm.get("invalid"):
                mismatch_lines.append(f"[{subset_tag}/{mode}/{name}] ugyldig expected_* format")
                continue
            if subset_norm != ext_norm:
                mismatch_lines.append(f"[{subset_tag}/{mode}/{name}] avviker mot utvidet")
                mismatch_lines.extend(
                    difflib.unified_diff(
                        json.dumps(subset_norm, ensure_ascii=False, indent=2).splitlines(),
                        json.dumps(ext_norm, ensure_ascii=False, indent=2).splitlines(),
                        fromfile=f"{subset_tag}/{name}",
                        tofile=f"extended/{name}",
                        lineterm="",
                    )
                )

    success = len(mismatch_lines) == 0
    return {
        "success": success,
        "checked_cases": checked,
        "mismatch_count": len(mismatch_lines),
        "stderr": "" if success else "\n".join(mismatch_lines) + "\n",
        "duration_ms": int((time.perf_counter() - started) * 1000),
        "scope": subset_tag,
    }


def run_selfhost_parser_suite_all_consistency_check(
    m1_fixture: Path,
    m2_fixture: Path,
    extended_fixture: Path,
) -> dict:
    started = time.perf_counter()
    mismatch_lines: list[str] = []

    m1 = run_selfhost_parser_suite_subset_consistency_check(m1_fixture, extended_fixture, "m1")
    m2 = run_selfhost_parser_suite_subset_consistency_check(m2_fixture, extended_fixture, "m2")

    m1_fixture_obj = _load_selfhost_parity_fixture(m1_fixture, mismatch_lines)
    m2_fixture_obj = _load_selfhost_parity_fixture(m2_fixture, mismatch_lines)
    ext_fixture_obj = _load_selfhost_parity_fixture(extended_fixture, mismatch_lines)

    coverage_checked = 0
    if m1_fixture_obj is not None and m2_fixture_obj is not None and ext_fixture_obj is not None:
        m1_maps, _ = _selfhost_parity_fixture_case_maps(m1_fixture_obj, "m1", mismatch_lines)
        m2_maps, _ = _selfhost_parity_fixture_case_maps(m2_fixture_obj, "m2", mismatch_lines)
        ext_maps, _ = _selfhost_parity_fixture_case_maps(ext_fixture_obj, "extended", mismatch_lines)

        for mode in ("expressions", "scripts"):
            union_names = set(m1_maps[mode].keys()) | set(m2_maps[mode].keys())
            coverage_checked += len(union_names)

            overlap = set(m1_maps[mode].keys()) & set(m2_maps[mode].keys())
            for name in sorted(overlap):
                if m1_maps[mode][name] != m2_maps[mode][name]:
                    mismatch_lines.append(f"[all/{mode}/{name}] finnes i både m1 og m2 med ulikt innhold")

            for name in sorted(union_names):
                union_case = m1_maps[mode].get(name) or m2_maps[mode].get(name)
                ext_case = ext_maps[mode].get(name)
                if ext_case is None:
                    mismatch_lines.append(f"[all/{mode}/{name}] finnes i m1/m2 men ikke i utvidet fixture")
                    continue
                if union_case.get("invalid") or ext_case.get("invalid"):
                    mismatch_lines.append(f"[all/{mode}/{name}] ugyldig expected_* format")
                    continue
                if union_case != ext_case:
                    mismatch_lines.append(f"[all/{mode}/{name}] avviker mellom m1/m2-union og utvidet")

            extra_in_extended = set(ext_maps[mode].keys()) - union_names
            for name in sorted(extra_in_extended):
                mismatch_lines.append(f"[all/{mode}/{name}] finnes i utvidet fixture, men mangler i m1+m2")

    all_success = m1.get("success") and m2.get("success") and not mismatch_lines
    total_mismatch = int(m1.get("mismatch_count", 0) or 0) + int(m2.get("mismatch_count", 0) or 0) + len(mismatch_lines)

    details: list[str] = []
    if not m1.get("success") and m1.get("stderr"):
        details.append("[m1]\n" + str(m1.get("stderr", "")).rstrip())
    if not m2.get("success") and m2.get("stderr"):
        details.append("[m2]\n" + str(m2.get("stderr", "")).rstrip())
    if mismatch_lines:
        details.append("[all]\n" + "\n".join(mismatch_lines))

    return {
        "success": bool(all_success),
        "checked_cases": int(m1.get("checked_cases", 0) or 0)
        + int(m2.get("checked_cases", 0) or 0)
        + coverage_checked,
        "mismatch_count": total_mismatch,
        "stderr": "" if all_success else ("\n\n".join(details).rstrip() + "\n"),
        "duration_ms": int((time.perf_counter() - started) * 1000),
        "scope": "all",
        "checks": {
            "m1": m1,
            "m2": m2,
            "coverage_checked_cases": coverage_checked,
            "coverage_mismatch_count": len(mismatch_lines),
        },
    }


def run_selfhost_parser_suite_consistency_check(m1_fixture: Path, extended_fixture: Path) -> dict:
    return run_selfhost_parser_suite_subset_consistency_check(m1_fixture, extended_fixture, "m1")


def _selfhost_parity_suite_targets(suite: str) -> list[tuple[Path, str]]:
    suites = {
        "m1": [(SELFHOST_PARSER_M1_FIXTURE, "Selfhost parser parity (M1)")],
        "m2": [(SELFHOST_PARSER_M2_FIXTURE, "Selfhost parser parity (M2)")],
        "extended": [(SELFHOST_PARSER_EXTENDED_FIXTURE, "Selfhost parser parity (utvidet)")],
        "all": [
            (SELFHOST_PARSER_M1_FIXTURE, "Selfhost parser parity (M1)"),
            (SELFHOST_PARSER_M2_FIXTURE, "Selfhost parser parity (M2)"),
            (SELFHOST_PARSER_EXTENDED_FIXTURE, "Selfhost parser parity (utvidet)"),
        ],
    }
    if suite not in suites:
        raise RuntimeError(f"Ugyldig suite: {suite}")
    return suites[suite]


def update_selfhost_parser_fixtures(check_only: bool = False, suite: str = "all", sync_m2: bool = True) -> dict:
    targets = _selfhost_parity_suite_targets(suite)
    summaries: list[dict] = []
    total_updated = 0
    total_cases = 0
    m2_sync_payload = None

    if sync_m2 and suite in {"m2", "all"}:
        m2_sync_payload = sync_selfhost_parser_m2_fixture(check_only=check_only)
        total_updated += int(m2_sync_payload.get("updated", 0) or 0)
        total_updated += int(m2_sync_payload.get("missing_m1_from_core_count", 0) or 0)

    for fixture_path, label in targets:
        fixture_abs = fixture_path.resolve()
        fixture = json.loads(fixture_abs.read_text(encoding="utf-8"))
        updated = 0
        case_count = 0

        for mode, key in (("expression", "expressions"), ("script", "scripts")):
            cases = fixture.get(key, [])
            if not cases:
                continue
            sources = [str(item.get("source", "")) for item in cases]
            actual_lists = _run_selfhost_parser_disasm_cases(sources, mode=mode)
            if len(actual_lists) != len(cases):
                raise RuntimeError(f"Intern feil ved oppdatering av {fixture_abs} ({mode}): antall resultater avviker")

            for idx, item in enumerate(cases):
                case_count += 1
                actual_lines = actual_lists[idx]
                old_lines = item.get("expected_lines")
                old_error = item.get("expected_error")

                if actual_lines and actual_lines[0].startswith("/* feil:"):
                    new_error = actual_lines[0]
                    if old_error != new_error or old_lines is not None:
                        updated += 1
                    item["expected_error"] = new_error
                    item.pop("expected_lines", None)
                else:
                    if old_lines != actual_lines or old_error is not None:
                        updated += 1
                    item["expected_lines"] = actual_lines
                    item.pop("expected_error", None)

        total_updated += updated
        total_cases += case_count
        summaries.append(
            {
                "fixture": str(fixture_abs),
                "label": label,
                "cases": case_count,
                "updated": updated,
            }
        )
        if not check_only:
            fixture_abs.write_text(json.dumps(fixture, ensure_ascii=False, indent=2) + "\n", encoding="utf-8")

    return {
        "suite": suite,
        "check_only": check_only,
        "sync_m2": sync_m2,
        "m2_sync": m2_sync_payload,
        "updated": total_updated,
        "cases": total_cases,
        "fixtures": summaries,
    }


def sync_selfhost_parser_m2_fixture(check_only: bool = False) -> dict:
    m1_path = SELFHOST_PARSER_M1_FIXTURE.resolve()
    core_path = SELFHOST_PARSER_EXTENDED_FIXTURE.resolve()
    m2_path = SELFHOST_PARSER_M2_FIXTURE.resolve()

    m1 = json.loads(m1_path.read_text(encoding="utf-8"))
    core = json.loads(core_path.read_text(encoding="utf-8"))
    current_m2 = json.loads(m2_path.read_text(encoding="utf-8")) if m2_path.exists() else {"expressions": [], "scripts": []}

    target_m2: dict[str, list[dict]] = {"expressions": [], "scripts": []}
    missing_from_core: dict[str, list[str]] = {"expressions": [], "scripts": []}

    for mode in ("expressions", "scripts"):
        m1_cases = m1.get(mode, [])
        core_cases = core.get(mode, [])
        m1_names = {str(item.get("name", "")) for item in m1_cases if "name" in item}
        core_names = {str(item.get("name", "")) for item in core_cases if "name" in item}
        target_m2[mode] = [item for item in core_cases if str(item.get("name", "")) not in m1_names]
        missing_from_core[mode] = sorted(name for name in m1_names if name and name not in core_names)

    updated = 1 if current_m2 != target_m2 else 0
    if not check_only and updated:
        m2_path.write_text(json.dumps(target_m2, ensure_ascii=False, indent=2) + "\n", encoding="utf-8")

    missing_count = len(missing_from_core["expressions"]) + len(missing_from_core["scripts"])
    ok = updated == 0 and missing_count == 0

    return {
        "check_only": check_only,
        "ok": ok,
        "updated": updated,
        "fixture": str(m2_path),
        "m1_fixture": str(m1_path),
        "core_fixture": str(core_path),
        "m1_cases": len(m1.get("expressions", [])) + len(m1.get("scripts", [])),
        "core_cases": len(core.get("expressions", [])) + len(core.get("scripts", [])),
        "m2_cases": len(target_m2.get("expressions", [])) + len(target_m2.get("scripts", [])),
        "missing_m1_from_core_count": missing_count,
        "missing_m1_from_core": missing_from_core,
    }


def update_ir_snapshots(check_only: bool = False):
    fixture_path = IR_SNAPSHOT_FIXTURE.resolve()
    fixture = json.loads(fixture_path.read_text(encoding="utf-8"))
    strict_cases = fixture.get("strict", [])

    updated = 0
    for item in strict_cases:
        src = item.get("source", "")
        tokens = tokenize_simple(src)
        try:
            lines = parse_ir_tokens(tokens, strict=True)
            if item.get("expected_lines") != lines:
                updated += 1
            item["expected_lines"] = lines
            item.pop("expected_error", None)
        except Exception as exc:
            err = str(exc)
            if item.get("expected_error") != err:
                updated += 1
            item["expected_error"] = err
            item.pop("expected_lines", None)

    if not check_only:
        fixture_path.write_text(json.dumps(fixture, ensure_ascii=False, indent=2) + "\n", encoding="utf-8")
    return fixture_path, updated, len(strict_cases)


def run_all_tests(verbose: bool = False, quiet: bool = False):
    tests = discover_tests()
    if not tests:
        raise RuntimeError("Fant ingen tester i tests/")

    results = []
    for test_file in tests:
        result = run_test_file(str(test_file))
        results.append(result)
        if not quiet:
            print_test_result(result, verbose=verbose)

    snapshot_result = run_ir_snapshot_checks()
    results.append(snapshot_result)
    if not quiet:
        print_test_result(snapshot_result, verbose=verbose)

    total = len(results)
    passed = sum(1 for r in results if r["success"])
    failed = total - passed

    if not quiet:
        print()
        print(f"Tester kjørt: {total}")
        print(f"Bestått: {passed}")
        print(f"Feilet: {failed}")
        if failed:
            print("Feilede tester:")
            for result in results:
                if not result["success"]:
                    print(f"- {result['source']}")
        else:
            print("Alle tester besto.")

    return results


def summarize_test_results(results: list[dict]) -> dict:
    total = len(results)
    passed = sum(1 for r in results if r.get("success"))
    failed = total - passed
    duration_ms = sum(r.get("duration_ms", 0) for r in results if isinstance(r.get("duration_ms"), int))
    timed_results = [r for r in results if isinstance(r.get("duration_ms"), int)]
    sorted_by_duration = sorted(timed_results, key=lambda r: int(r.get("duration_ms", 0)), reverse=True)
    return {
        "total": total,
        "passed": passed,
        "failed": failed,
        "duration_ms": duration_ms,
        "duration_s": round(duration_ms / 1000.0, 3),
        "ok": failed == 0,
        "passed_sources": [r["source"] for r in results if r.get("success")],
        "failed_sources": [r["source"] for r in results if not r.get("success")],
        "slowest": [
            {
                "source": r["source"],
                "duration_ms": int(r.get("duration_ms", 0)),
            }
            for r in sorted_by_duration[:5]
        ],
    }


def summarize_lint_results(result: dict) -> dict:
    issues = list(result.get("issues", []))
    severity_counts: dict[str, int] = {}
    for issue in issues:
        severity = str(issue.get("severity", "warning"))
        severity_counts[severity] = severity_counts.get(severity, 0) + 1
    return {
        "source": result.get("source"),
        "issue_count": len(issues),
        "ok": not issues,
        "severity_counts": severity_counts,
        "codes": sorted({str(issue.get("code", "lint")) for issue in issues}),
    }


def check_workflow_action_versions(workflows_dir: Path | None = None) -> dict:
    base = workflows_dir or Path(".github/workflows")
    minimum_action_majors = WORKFLOW_ACTION_POLICY["minimum_action_majors"]
    required_norcode_ci_flags = [str(flag) for flag in WORKFLOW_ACTION_POLICY.get("required_norcode_ci_flags", [])]
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
            if run_match:
                run_cmd = run_match.group(1).strip()
                if "norcode ci" in run_cmd or "./bin/nc ci" in run_cmd:
                    saw_ci_command = True
                    for flag in required_norcode_ci_flags:
                        if flag not in run_cmd:
                            payload["issues"].append(
                                {
                                    "file": str(workflow_path),
                                    "line": line_no,
                                    "type": "missing_norcode_ci_flag",
                                    "rule": "require_norcode_ci_flag",
                                    "found": run_cmd,
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
        if required_norcode_ci_flags and not saw_ci_command:
            payload["issues"].append(
                {
                    "file": str(workflow_path),
                    "line": 1,
                    "type": "missing_norcode_ci_command",
                    "rule": "require_norcode_ci_command",
                    "found": "mangler run-linje med 'norcode ci' eller './bin/nc ci'",
                    "expected": "legg til run: norcode ci --check-names --require-selfhost-ready eller run: ./bin/nc ci --check-names --require-selfhost-ready",
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


def run_ci_pipeline(
    json_output: bool = False,
    check_names: bool = False,
    parity_suite: str = "all",
    require_selfhost_ready: bool = False,
):
    if parity_suite not in {"m1", "m2", "all"}:
        raise RuntimeError(f"Ugyldig parity-suite for CI: {parity_suite}")
    run_m2_sync_check = parity_suite in {"m2", "all"}
    pipeline_started = time.perf_counter()
    started_at_utc = dt.datetime.now(dt.timezone.utc).isoformat()
    started_at_epoch_ms = int(time.time() * 1000)
    source_revision = get_current_git_revision()
    source_branch = get_current_git_branch()
    source_tag = get_current_git_exact_tag()
    source_remote = get_current_git_origin_url()
    source_remote_protocol = get_git_remote_protocol(source_remote)
    source_remote_host = get_git_remote_host(source_remote)
    source_remote_provider = get_git_remote_provider(source_remote_host)
    source_repo_slug = get_git_remote_repo_slug(source_remote)
    source_repo_owner, source_repo_name = split_repo_slug(source_repo_slug)
    source_dirty = get_current_git_dirty_state()
    py_major_minor = f"{sys.version_info.major}.{sys.version_info.minor}"
    step_order = ["snapshot_check", "parser_fixture_check", "parity_check"]
    if parity_suite == "m1":
        step_order.append("parser_core_m1_check")
    elif parity_suite == "m2":
        step_order.append("parser_core_m2_check")
    else:
        step_order.append("parser_core_m1_check")
        step_order.append("parser_core_m2_check")
        step_order.append("parser_core_extended_check")
    step_order.append("parser_suite_consistency_check")
    if run_m2_sync_check:
        step_order.append("selfhost_m2_sync_check")
    if require_selfhost_ready:
        step_order.append("selfhost_progress_check")
    step_order.extend(["test_check", "workflow_action_check"])
    if check_names:
        step_order.append("name_migration_check")
    total_steps = len(step_order)
    payload = {
        "schema_version": 1,
        "run_id": uuid.uuid4().hex,
        "ok": False,
        "source_revision": source_revision,
        "source_revision_short": to_short_git_revision(source_revision),
        "source_branch": source_branch,
        "source_tag": source_tag,
        "source_ref": source_tag or source_branch,
        "source_ref_type": "tag" if source_tag else ("branch" if source_branch else "unknown"),
        "source_remote": source_remote,
        "source_remote_protocol": source_remote_protocol,
        "source_remote_is_https": source_remote_protocol == "https",
        "source_remote_is_ssh": source_remote_protocol == "ssh",
        "source_remote_host": source_remote_host,
        "source_remote_is_github": is_github_host(source_remote_host),
        "source_remote_is_gitlab": is_gitlab_host(source_remote_host),
        "source_remote_is_bitbucket": is_bitbucket_host(source_remote_host),
        "source_remote_provider": source_remote_provider,
        "source_remote_is_unknown": source_remote_provider == "unknown",
        "source_repo_slug": source_repo_slug,
        "source_repo_owner": source_repo_owner,
        "source_repo_name": source_repo_name,
        "source_repo_url": get_source_repo_url(source_remote_host, source_repo_slug),
        "source_branch_url": get_source_ref_url(source_remote_host, source_repo_slug, source_branch),
        "source_tag_url": get_source_ref_url(source_remote_host, source_repo_slug, source_tag),
        "source_ref_url": get_source_ref_url(source_remote_host, source_repo_slug, source_tag or source_branch),
        "source_revision_url": get_source_revision_url(source_remote_host, source_repo_slug, source_revision),
        "source_is_tagged": source_tag is not None,
        "source_is_main": source_branch == "main",
        "source_dirty": source_dirty,
        "source_clean": (not source_dirty) if source_dirty is not None else None,
        "invocation": {
            "cmd": "norcode ci",
            "argv0": sys.argv[0] if sys.argv else None,
            "raw": " ".join(shlex.quote(arg) for arg in sys.argv),
            "json_output": json_output,
            "check_names": check_names,
            "parity_suite": parity_suite,
            "require_selfhost_ready": require_selfhost_ready,
            "argv": sys.argv[1:],
        },
        "runtime": {
            "python_version": platform.python_version(),
            "python_major_minor": py_major_minor,
            "python_api_version": sys.api_version,
            "python_hexversion": sys.hexversion,
            "python_implementation": platform.python_implementation(),
            "python_compiler": platform.python_compiler(),
            "python_build": " ".join(platform.python_build()),
            "python_cache_tag": sys.implementation.cache_tag,
            "python_executable": sys.executable,
            "python_prefix": sys.prefix,
            "python_base_prefix": sys.base_prefix,
            "python_is_venv": sys.prefix != sys.base_prefix,
            "byteorder": sys.byteorder,
            "locale": locale.setlocale(locale.LC_CTYPE, None),
            "encoding": locale.getpreferredencoding(False),
            "path_entries": len(os.getenv("PATH", "").split(os.pathsep)) if os.getenv("PATH") else 0,
            "path_separator": os.pathsep,
            "env_var_count": len(os.environ),
            "stdin_isatty": sys.stdin.isatty(),
            "stdout_isatty": sys.stdout.isatty(),
            "stderr_isatty": sys.stderr.isatty(),
            "shell": os.getenv("SHELL"),
            "term": os.getenv("TERM"),
            "virtual_env": os.getenv("VIRTUAL_ENV"),
            "virtual_env_name": Path(os.getenv("VIRTUAL_ENV")).name if os.getenv("VIRTUAL_ENV") else None,
            "is_ci": bool(os.getenv("CI")),
            "is_github_actions": bool(os.getenv("GITHUB_ACTIONS")),
            "github_actions_run_id": os.getenv("GITHUB_RUN_ID"),
            "github_actions_run_number": os.getenv("GITHUB_RUN_NUMBER"),
            "github_actions_run_attempt": os.getenv("GITHUB_RUN_ATTEMPT"),
            "github_actions_workflow": os.getenv("GITHUB_WORKFLOW"),
            "github_actions_job": os.getenv("GITHUB_JOB"),
            "github_actions_ref": os.getenv("GITHUB_REF"),
            "github_actions_sha": os.getenv("GITHUB_SHA"),
            "github_actions_actor": os.getenv("GITHUB_ACTOR"),
            "github_actions_event_name": os.getenv("GITHUB_EVENT_NAME"),
            "os": platform.system(),
            "arch": platform.machine(),
            "platform": platform.platform(),
            "hostname": platform.node(),
            "user": os.getenv("USER") or os.getenv("USERNAME"),
            "uid": os.getuid() if hasattr(os, "getuid") else None,
            "gid": os.getgid() if hasattr(os, "getgid") else None,
            "pid": os.getpid(),
            "ppid": os.getppid(),
            "process_group_id": os.getpgrp() if hasattr(os, "getpgrp") else None,
            "home": os.path.expanduser("~"),
            "tmpdir": tempfile.gettempdir(),
            "cwd": str(Path.cwd()),
            "cwd_has_spaces": " " in str(Path.cwd()),
            "timezone": dt.datetime.now().astimezone().tzname(),
        },
        "steps": {
            "total": total_steps,
            "name_check_enabled": check_names,
            "parity_suite": parity_suite,
            "require_selfhost_ready": require_selfhost_ready,
            "order": step_order,
        },
        "started_at_utc": started_at_utc,
        "started_at_epoch_ms": started_at_epoch_ms,
        "finished_at_utc": None,
        "finished_at_epoch_ms": None,
        "timings_ms": {},
        "timings_s": {},
        "timings_ratio": {},
        "snapshot_check": {"ok": False, "updated": None},
        "parser_fixture_check": {"ok": False, "updated": None, "cases": 0, "suite": parity_suite},
        "parity_check": {"ok": False},
        "parser_core_m1_check": {"ok": False, "case_count": 0, "error_cases": 0},
        "parser_core_m2_check": {"ok": False, "case_count": 0, "error_cases": 0},
        "parser_core_extended_check": {"ok": False, "case_count": 0, "error_cases": 0},
        "parser_suite_consistency_check": {"ok": False, "checked_cases": 0, "mismatch_count": 0},
        "selfhost_m2_sync_check": {"enabled": run_m2_sync_check, "ok": True, "updated": 0, "missing_m1_from_core_count": 0},
        "selfhost_progress_check": {"enabled": require_selfhost_ready, "ok": True, "ready": None, "coverage_total_pct": None},
        "test_check": {"ok": False, "passed": 0, "failed": 0, "total": 0},
        "workflow_action_check": {
            "ok": False,
            "scanned_dir": ".github/workflows",
            "scanned_files": 0,
            "files": [],
            "file_extensions": [".yml", ".yaml"],
            "issue_count": 0,
            "issue_types": {},
            "issues": [],
            "policy": WORKFLOW_ACTION_POLICY,
        },
        "name_migration_check": {"enabled": check_names, "ok": True, "needs_migration": False},
    }

    if not json_output:
        print(f"[1/{total_steps}] Snapshot check")
    started = time.perf_counter()
    _fixture_path, updated, _total = update_ir_snapshots(check_only=True)
    payload["timings_ms"]["snapshot_check"] = int((time.perf_counter() - started) * 1000)
    payload["timings_s"]["snapshot_check"] = round(payload["timings_ms"]["snapshot_check"] / 1000.0, 3)
    payload["snapshot_check"]["updated"] = updated
    if updated > 0:
        raise RuntimeError(f"Snapshots er utdaterte ({updated} avvik). Kjør: norcode update-snapshots")
    payload["snapshot_check"]["ok"] = True
    if not json_output:
        print("OK")

    if not json_output:
        print(f"[2/{total_steps}] Selfhost parity fixture check")
    started = time.perf_counter()
    fixture_suite = parity_suite if parity_suite in {"m1", "m2"} else "all"
    fixture_check = update_selfhost_parser_fixtures(check_only=True, suite=fixture_suite)
    payload["timings_ms"]["parser_fixture_check"] = int((time.perf_counter() - started) * 1000)
    payload["timings_s"]["parser_fixture_check"] = round(payload["timings_ms"]["parser_fixture_check"] / 1000.0, 3)
    payload["parser_fixture_check"]["ok"] = fixture_check["updated"] == 0
    payload["parser_fixture_check"]["updated"] = int(fixture_check["updated"])
    payload["parser_fixture_check"]["cases"] = int(fixture_check["cases"])
    payload["parser_fixture_check"]["suite"] = str(fixture_check["suite"])
    if fixture_check["updated"] > 0:
        raise RuntimeError(
            f"Selfhost parity-fixtures er utdaterte ({fixture_check['updated']} avvik). "
            f"Kjør: norcode update-selfhost-parity-fixtures --suite {fixture_suite}"
        )
    if not json_output:
        print(
            f"OK ({payload['parser_fixture_check']['cases']} cases, "
            f"suite={payload['parser_fixture_check']['suite']})"
        )

    if not json_output:
        print(f"[3/{total_steps}] Engine parity check")
    started = time.perf_counter()
    _, py_ok, py_lines, py_err = ir_disasm_source_captured("tests/ir_sample.nlir", strict=False, engine="python")
    _, sh_ok, sh_lines, sh_err = ir_disasm_source_captured("tests/ir_sample.nlir", strict=False, engine="selfhost")
    if py_ok != sh_ok:
        raise RuntimeError(f"Parity mismatch (status): python={py_ok} selfhost={sh_ok} {py_err or sh_err}")
    if py_ok and py_lines != sh_lines:
        raise RuntimeError("Parity mismatch: python/selfhost disasm avviker for tests/ir_sample.nlir")

    _, py_s_ok, py_s_lines, py_s_err = ir_disasm_source_captured("tests/ir_sample.nlir", strict=True, engine="python")
    _, sh_s_ok, sh_s_lines, sh_s_err = ir_disasm_source_captured("tests/ir_sample.nlir", strict=True, engine="selfhost")
    if py_s_ok != sh_s_ok or py_s_lines != sh_s_lines or py_s_err != sh_s_err:
        raise RuntimeError("Parity mismatch i strict-modus for tests/ir_sample.nlir")
    payload["timings_ms"]["parity_check"] = int((time.perf_counter() - started) * 1000)
    payload["timings_s"]["parity_check"] = round(payload["timings_ms"]["parity_check"] / 1000.0, 3)
    payload["parity_check"]["ok"] = True
    if not json_output:
        print("OK")

    if parity_suite in {"m1", "all"}:
        if not json_output:
            print(f"[4/{total_steps}] Selfhost parser parity (M1)")
        started = time.perf_counter()
        parser_core_m1_result = run_selfhost_parser_core_checks(
            SELFHOST_PARSER_M1_FIXTURE, "Selfhost parser parity (M1)"
        )
        payload["timings_ms"]["parser_core_m1_check"] = int((time.perf_counter() - started) * 1000)
        payload["timings_s"]["parser_core_m1_check"] = round(
            payload["timings_ms"]["parser_core_m1_check"] / 1000.0, 3
        )
        payload["parser_core_m1_check"]["ok"] = parser_core_m1_result["success"]
        payload["parser_core_m1_check"]["case_count"] = int(parser_core_m1_result.get("case_count", 0) or 0)
        payload["parser_core_m1_check"]["error_cases"] = int(parser_core_m1_result.get("error_cases", 0) or 0)
        if not parser_core_m1_result["success"]:
            raise RuntimeError(
                "Selfhost parser parity-feil (M1):\n"
                + (parser_core_m1_result.get("stderr", "").rstrip() or "ukjent feil")
            )
        if not json_output:
            print(
                f"OK ({payload['parser_core_m1_check']['case_count']} cases, "
                f"{payload['parser_core_m1_check']['error_cases']} feil-cases)"
            )
    else:
        payload["parser_core_m1_check"]["ok"] = True
        payload["parser_core_m1_check"]["case_count"] = 0
        payload["parser_core_m1_check"]["error_cases"] = 0

    if parity_suite in {"m2", "all"}:
        if not json_output:
            m2_step = 5 if parity_suite == "all" else 4
            print(f"[{m2_step}/{total_steps}] Selfhost parser parity (M2)")
        started = time.perf_counter()
        parser_core_m2_result = run_selfhost_parser_core_checks(
            SELFHOST_PARSER_M2_FIXTURE, "Selfhost parser parity (M2)"
        )
        payload["timings_ms"]["parser_core_m2_check"] = int((time.perf_counter() - started) * 1000)
        payload["timings_s"]["parser_core_m2_check"] = round(
            payload["timings_ms"]["parser_core_m2_check"] / 1000.0, 3
        )
        payload["parser_core_m2_check"]["ok"] = parser_core_m2_result["success"]
        payload["parser_core_m2_check"]["case_count"] = int(parser_core_m2_result.get("case_count", 0) or 0)
        payload["parser_core_m2_check"]["error_cases"] = int(parser_core_m2_result.get("error_cases", 0) or 0)
        if not parser_core_m2_result["success"]:
            raise RuntimeError(
                "Selfhost parser parity-feil (M2):\n"
                + (parser_core_m2_result.get("stderr", "").rstrip() or "ukjent feil")
            )
        if not json_output:
            print(
                f"OK ({payload['parser_core_m2_check']['case_count']} cases, "
                f"{payload['parser_core_m2_check']['error_cases']} feil-cases)"
            )
    else:
        payload["parser_core_m2_check"]["ok"] = True
        payload["parser_core_m2_check"]["case_count"] = 0
        payload["parser_core_m2_check"]["error_cases"] = 0

    if parity_suite == "all":
        if not json_output:
            print(f"[6/{total_steps}] Selfhost parser parity (utvidet)")
        started = time.perf_counter()
        parser_core_extended_result = run_selfhost_parser_core_checks(
            SELFHOST_PARSER_EXTENDED_FIXTURE, "Selfhost parser parity (utvidet)"
        )
        payload["timings_ms"]["parser_core_extended_check"] = int((time.perf_counter() - started) * 1000)
        payload["timings_s"]["parser_core_extended_check"] = round(
            payload["timings_ms"]["parser_core_extended_check"] / 1000.0, 3
        )
        payload["parser_core_extended_check"]["ok"] = parser_core_extended_result["success"]
        payload["parser_core_extended_check"]["case_count"] = int(
            parser_core_extended_result.get("case_count", 0) or 0
        )
        payload["parser_core_extended_check"]["error_cases"] = int(
            parser_core_extended_result.get("error_cases", 0) or 0
        )
        if not parser_core_extended_result["success"]:
            raise RuntimeError(
                "Selfhost parser parity-feil (utvidet):\n"
                + (parser_core_extended_result.get("stderr", "").rstrip() or "ukjent feil")
            )
        if not json_output:
            print(
                f"OK ({payload['parser_core_extended_check']['case_count']} cases, "
                f"{payload['parser_core_extended_check']['error_cases']} feil-cases)"
            )
    else:
        payload["parser_core_extended_check"]["ok"] = True
        payload["parser_core_extended_check"]["case_count"] = 0
        payload["parser_core_extended_check"]["error_cases"] = 0

    consistency_step = 7 if parity_suite == "all" else 5
    if not json_output:
        print(f"[{consistency_step}/{total_steps}] Parser suite consistency")
    started = time.perf_counter()
    if parity_suite == "m1":
        consistency = run_selfhost_parser_suite_consistency_check(
            SELFHOST_PARSER_M1_FIXTURE,
            SELFHOST_PARSER_EXTENDED_FIXTURE,
        )
    elif parity_suite == "m2":
        consistency = run_selfhost_parser_suite_subset_consistency_check(
            SELFHOST_PARSER_M2_FIXTURE,
            SELFHOST_PARSER_EXTENDED_FIXTURE,
            "m2",
        )
    else:
        consistency = run_selfhost_parser_suite_all_consistency_check(
            SELFHOST_PARSER_M1_FIXTURE,
            SELFHOST_PARSER_M2_FIXTURE,
            SELFHOST_PARSER_EXTENDED_FIXTURE,
        )
    payload["timings_ms"]["parser_suite_consistency_check"] = int((time.perf_counter() - started) * 1000)
    payload["timings_s"]["parser_suite_consistency_check"] = round(
        payload["timings_ms"]["parser_suite_consistency_check"] / 1000.0, 3
    )
    payload["parser_suite_consistency_check"]["ok"] = bool(consistency.get("success"))
    payload["parser_suite_consistency_check"]["checked_cases"] = int(consistency.get("checked_cases", 0) or 0)
    payload["parser_suite_consistency_check"]["mismatch_count"] = int(consistency.get("mismatch_count", 0) or 0)
    if not consistency.get("success"):
        raise RuntimeError(
            "Parser suite consistency-feil:\n"
            + (consistency.get("stderr", "").rstrip() or "ukjent feil")
        )
    if not json_output:
        print(f"OK ({payload['parser_suite_consistency_check']['checked_cases']} cases)")

    post_consistency_offset = 0
    if run_m2_sync_check:
        sync_step = consistency_step + 1
        if not json_output:
            print(f"[{sync_step}/{total_steps}] Selfhost M2 sync check")
        started = time.perf_counter()
        m2_sync = sync_selfhost_parser_m2_fixture(check_only=True)
        payload["timings_ms"]["selfhost_m2_sync_check"] = int((time.perf_counter() - started) * 1000)
        payload["timings_s"]["selfhost_m2_sync_check"] = round(
            payload["timings_ms"]["selfhost_m2_sync_check"] / 1000.0, 3
        )
        payload["selfhost_m2_sync_check"]["enabled"] = True
        payload["selfhost_m2_sync_check"]["updated"] = int(m2_sync.get("updated", 0) or 0)
        payload["selfhost_m2_sync_check"]["missing_m1_from_core_count"] = int(
            m2_sync.get("missing_m1_from_core_count", 0) or 0
        )
        payload["selfhost_m2_sync_check"]["ok"] = (
            payload["selfhost_m2_sync_check"]["updated"] == 0
            and payload["selfhost_m2_sync_check"]["missing_m1_from_core_count"] == 0
        )
        payload["selfhost_m2_sync_check"]["result"] = m2_sync
        if not payload["selfhost_m2_sync_check"]["ok"]:
            raise RuntimeError(
                "Selfhost M2 sync-feil: M2-fixture er ute av synk med core minus M1. "
                "Kjør: norcode sync-selfhost-parity-m2"
            )
        if not json_output:
            print("OK")
        post_consistency_offset += 1

    progress_step = consistency_step + 1 + post_consistency_offset
    if require_selfhost_ready:
        if not json_output:
            print(f"[{progress_step}/{total_steps}] Selfhost parity progress check")
        started = time.perf_counter()
        progress = run_selfhost_parity_progress()
        payload["timings_ms"]["selfhost_progress_check"] = int((time.perf_counter() - started) * 1000)
        payload["timings_s"]["selfhost_progress_check"] = round(
            payload["timings_ms"]["selfhost_progress_check"] / 1000.0, 3
        )
        payload["selfhost_progress_check"]["enabled"] = True
        payload["selfhost_progress_check"]["ok"] = bool(progress.get("ok"))
        payload["selfhost_progress_check"]["ready"] = bool(progress.get("ready"))
        payload["selfhost_progress_check"]["coverage_total_pct"] = (
            progress.get("coverage", {}).get("total_pct") if isinstance(progress.get("coverage"), dict) else None
        )
        payload["selfhost_progress_check"]["result"] = progress
        if not progress.get("ok"):
            raise RuntimeError(
                "Selfhost parity progress-feil:\n"
                + (str(progress.get("stderr", "")).rstrip() or "ukjent feil")
            )
        if not progress.get("ready"):
            coverage_pct = (
                progress.get("coverage", {}).get("total_pct")
                if isinstance(progress.get("coverage"), dict)
                else None
            )
            raise RuntimeError(
                "Selfhost parity progress er ikke klar for full coverage"
                + (f" (dekning={coverage_pct}%)" if coverage_pct is not None else "")
            )
        if not json_output:
            print(
                "OK "
                f"(ready=ja, total_dekning={payload['selfhost_progress_check']['coverage_total_pct']}%)"
            )

    test_step = progress_step + (1 if require_selfhost_ready else 0)
    if not json_output:
        print(f"[{test_step}/{total_steps}] Full test")
    started = time.perf_counter()
    results = run_all_tests(verbose=False, quiet=json_output)
    payload["timings_ms"]["test_check"] = int((time.perf_counter() - started) * 1000)
    payload["timings_s"]["test_check"] = round(payload["timings_ms"]["test_check"] / 1000.0, 3)
    failed = sum(1 for r in results if not r["success"])
    total = len(results)
    passed = total - failed
    payload["test_check"]["ok"] = failed == 0
    payload["test_check"]["passed"] = passed
    payload["test_check"]["failed"] = failed
    payload["test_check"]["total"] = total
    if failed != 0:
        raise RuntimeError("Testfeil i CI-pipeline")
    if not json_output:
        print("OK")

    workflow_step = test_step + 1
    if not json_output:
        print(f"[{workflow_step}/{total_steps}] Workflow action version check")
    started = time.perf_counter()
    workflow_check = check_workflow_action_versions()
    payload["timings_ms"]["workflow_action_check"] = int((time.perf_counter() - started) * 1000)
    payload["timings_s"]["workflow_action_check"] = round(payload["timings_ms"]["workflow_action_check"] / 1000.0, 3)
    payload["workflow_action_check"] = workflow_check
    if not workflow_check["ok"]:
        issue = workflow_check["issues"][0]
        issue_type = issue.get("type", "unknown")
        raise RuntimeError(
            f"Workflow policy-brudd ({issue_type}) oppdaget: "
            f"{issue['found']} i {issue['file']}:{issue['line']} "
            f"(forventet: {issue['expected']})"
        )
    if not json_output:
        print(f"OK ({workflow_check['scanned_files']} filer)")

    if check_names:
        name_step = workflow_step + 1
        if not json_output:
            print(f"[{name_step}/{total_steps}] Name migration check")
        started = time.perf_counter()
        migration = migrate_names(apply_changes=False, cleanup_legacy=True)
        payload["timings_ms"]["name_migration_check"] = int((time.perf_counter() - started) * 1000)
        payload["timings_s"]["name_migration_check"] = round(payload["timings_ms"]["name_migration_check"] / 1000.0, 3)
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

    payload["timings_ms"]["total"] = int((time.perf_counter() - pipeline_started) * 1000)
    step_keys = [k for k in payload["timings_ms"].keys() if k not in ("total", "wallclock_total")]
    payload["timings_ms"]["step_sum"] = sum(
        payload["timings_ms"][k] for k in step_keys if isinstance(payload["timings_ms"].get(k), int)
    )
    payload["timings_ms"]["overhead"] = payload["timings_ms"]["total"] - payload["timings_ms"]["step_sum"]
    payload["timings_s"]["total"] = round(payload["timings_ms"]["total"] / 1000.0, 3)
    payload["timings_s"]["step_sum"] = round(payload["timings_ms"]["step_sum"] / 1000.0, 3)
    payload["timings_s"]["overhead"] = round(payload["timings_ms"]["overhead"] / 1000.0, 3)
    total_ms = payload["timings_ms"]["total"]
    if total_ms > 0:
        payload["timings_ratio"]["step_coverage"] = round(payload["timings_ms"]["step_sum"] / total_ms, 4)
        payload["timings_ratio"]["overhead_share"] = round(payload["timings_ms"]["overhead"] / total_ms, 4)
        payload["timings_ratio"]["step_coverage_pct"] = round(payload["timings_ratio"]["step_coverage"] * 100.0, 2)
        payload["timings_ratio"]["overhead_share_pct"] = round(payload["timings_ratio"]["overhead_share"] * 100.0, 2)
    else:
        payload["timings_ratio"]["step_coverage"] = 0.0
        payload["timings_ratio"]["overhead_share"] = 0.0
        payload["timings_ratio"]["step_coverage_pct"] = 0.0
        payload["timings_ratio"]["overhead_share_pct"] = 0.0
    payload["timings_ratio"]["ratio_sum"] = round(
        payload["timings_ratio"]["step_coverage"] + payload["timings_ratio"]["overhead_share"], 4
    )
    payload["timings_ratio"]["ratio_delta"] = round(abs(1.0 - payload["timings_ratio"]["ratio_sum"]), 6)
    payload["timings_ratio"]["percent_sum"] = round(
        payload["timings_ratio"]["step_coverage_pct"] + payload["timings_ratio"]["overhead_share_pct"], 2
    )
    payload["timings_ratio"]["percent_delta"] = round(abs(100.0 - payload["timings_ratio"]["percent_sum"]), 4)
    payload["timings_ratio"]["overhead_policy"] = {
        "low_max": 0.02,
        "medium_max": 0.05,
        "unit": "share",
    }
    overhead_share = payload["timings_ratio"]["overhead_share"]
    if overhead_share <= payload["timings_ratio"]["overhead_policy"]["low_max"]:
        payload["timings_ratio"]["overhead_level"] = "low"
    elif overhead_share <= payload["timings_ratio"]["overhead_policy"]["medium_max"]:
        payload["timings_ratio"]["overhead_level"] = "medium"
    else:
        payload["timings_ratio"]["overhead_level"] = "high"
    payload["timings_ratio"]["overhead_within_medium"] = (
        overhead_share <= payload["timings_ratio"]["overhead_policy"]["medium_max"]
    )
    payload["finished_at_utc"] = dt.datetime.now(dt.timezone.utc).isoformat()
    payload["finished_at_epoch_ms"] = int(time.time() * 1000)
    payload["timings_ms"]["wallclock_total"] = payload["finished_at_epoch_ms"] - payload["started_at_epoch_ms"]
    payload["timings_s"]["wallclock_total"] = round(payload["timings_ms"]["wallclock_total"] / 1000.0, 3)
    payload["ok"] = True
    return payload


def main():
    parser = argparse.ArgumentParser(prog="norcode", description="Norscode CLI")
    sub = parser.add_subparsers(dest="cmd")

    def _command_overview() -> list[dict[str, str]]:
        overview: list[dict[str, str]] = []
        for choice in sub._choices_actions:
            overview.append(
                {
                    "name": choice.dest,
                    "help": choice.help or "",
                }
            )
        return sorted(overview, key=lambda row: row["name"])

    run = sub.add_parser("run", help="Bygg og kjør en .no-fil")
    run.add_argument("file")

    repl = sub.add_parser("repl", help="Start en enkel interaktiv Norscode-REPL")

    check = sub.add_parser("check", help="Parser og valider en .no-fil uten å bygge")
    check.add_argument("file")

    build = sub.add_parser("build", help="Generer C og bygg kjørbar fil")
    build.add_argument("file")

    add = sub.add_parser("add", help="Legg til pakkeavhengighet i norcode.toml")
    add.add_argument("package", nargs="?", help="Pakkenavn eller pakkesti")
    add.add_argument("path", nargs="?", help="Valgfri pakkesti (hvis package er navn)")
    add.add_argument("--name", help="Overstyr dependency-navn")
    add.add_argument("--list", action="store_true", help="Vis tilgjengelige pakker i registry")
    add.add_argument("--git", help="Direkte Git-kilde (f.eks. https://github.com/org/repo.git)")
    add.add_argument("--ref", help="Git ref (tag/branch/commit) brukt sammen med --git")
    add.add_argument("--url", help="Direkte URL-kilde (f.eks. tarball/zip)")
    add.add_argument("--fetch", action="store_true", help="Last ned/cach ekstern Git/URL-kilde til lokal mappe")
    add.add_argument("--refresh", action="store_true", help="Tving ny nedlasting ved --fetch")
    add.add_argument("--pin", action="store_true", help="Krev låst versjon/ref for ekstern kilde")
    add.add_argument("--sha256", help="Forventet SHA256 for URL-arkiv ved --fetch")
    add.add_argument("--allow-untrusted", action="store_true", help="Overstyr trusted host-policy for denne kommandoen")

    debug = sub.add_parser("debug", help="Vis debug-info (tokens/AST/symboler) for en .no-fil")
    debug.add_argument("file")
    debug.add_argument("--tokens", action="store_true", help="Vis lexer-tokens")
    debug.add_argument("--ast", action="store_true", help="Vis AST")
    debug.add_argument("--symbols", action="store_true", help="Vis semantiske symboler/funksjoner")
    debug.add_argument("--json", action="store_true", help="Skriv debug-output som JSON")

    disasm = sub.add_parser("disasm", help="Vis generert C-kode for en .no-fil")
    disasm.add_argument("file")

    ir_disasm = sub.add_parser("ir-disasm", help="Vis IR-disassembly fra tekstfil")
    ir_disasm.add_argument("file")
    ir_disasm.add_argument("--json", action="store_true", help="Skriv output som JSON")
    ir_disasm.add_argument("--strict", action="store_true", help="Feil ved ukjente opcodes/ugyldige argumenter")
    ir_disasm.add_argument("--engine", choices=["python", "selfhost"], default="python", help="Velg disasm-motor")
    ir_disasm.add_argument("--diff", action="store_true", help="Sammenlign python og selfhost disasm")
    ir_disasm.add_argument("--fail-on-warning", action="store_true", help="Feil hvis strict-resultat avviker mellom motorene")
    ir_disasm.add_argument("--save-diff", help="Lagre diff-output til fil ved --diff")

    update_snapshots = sub.add_parser("update-snapshots", help="Regenerer IR snapshot-forventninger")
    update_snapshots.add_argument("--check", action="store_true", help="Feil hvis snapshots er utdaterte (skriv ikke)")
    update_snapshots.add_argument("--json", action="store_true", help="Skriv resultat som JSON")

    update_selfhost_parity = sub.add_parser(
        "update-selfhost-parity-fixtures",
        help="Regenerer selfhost parser parity-forventninger",
    )
    update_selfhost_parity.add_argument("--suite", choices=["m1", "m2", "extended", "all"], default="all", help="Velg fixtures å oppdatere")
    update_selfhost_parity.add_argument("--check", action="store_true", help="Feil hvis parity-fixtures er utdaterte (skriv ikke)")
    update_selfhost_parity.add_argument("--no-sync-m2", action="store_true", help="Hopp over automatisk M2-sync (core minus M1)")
    update_selfhost_parity.add_argument("--json", action="store_true", help="Skriv resultat som JSON")

    sync_selfhost_parity_m2 = sub.add_parser(
        "sync-selfhost-parity-m2",
        help="Synkroniser M2-fixture som core minus M1",
    )
    sync_selfhost_parity_m2.add_argument("--check", action="store_true", help="Feil hvis M2-fixture er ute av synk")
    sync_selfhost_parity_m2.add_argument("--json", action="store_true", help="Skriv resultat som JSON")

    ci = sub.add_parser("ci", help="Kjør lokal CI-sekvens (snapshot, parity, test)")
    ci.add_argument("--json", action="store_true", help="Skriv CI-resultat som JSON")
    ci.add_argument("--check-names", action="store_true", help="Inkluder sjekk for navnemigrering (legacy -> Norscode)")
    ci.add_argument("--parity-suite", choices=["m1", "m2", "all"], default="all", help="Velg parity-scope i CI")
    ci.add_argument(
        "--require-selfhost-ready",
        action="store_true",
        help="Feil hvis selfhost parity progress ikke er fullført/ready",
    )

    selfhost_parity = sub.add_parser("selfhost-parity", help="Kjør selfhost parser parity-suiter")
    selfhost_parity.add_argument("--suite", choices=["m1", "m2", "extended", "all"], default="all", help="Velg parity-suite")
    selfhost_parity.add_argument("--json", action="store_true", help="Skriv resultat som JSON")

    selfhost_parity_progress = sub.add_parser(
        "selfhost-parity-progress",
        help="Vis fremdrift for M1/M2 dekning mot utvidet parity-suite",
    )
    selfhost_parity_progress.add_argument("--json", action="store_true", help="Skriv resultat som JSON")
    selfhost_parity_progress.add_argument(
        "--require-ready",
        action="store_true",
        help="Feil hvis progress ikke er ready",
    )
    selfhost_parity_progress.add_argument(
        "--min-coverage",
        type=float,
        help="Krev minimum total dekningsprosent (0-100)",
    )

    selfhost_parity_gate = sub.add_parser(
        "selfhost-parity-gate",
        help="Kjør parity-progress som en eksplisitt gate",
    )
    selfhost_parity_gate.add_argument("--json", action="store_true", help="Skriv resultat som JSON")
    selfhost_parity_gate.add_argument(
        "--min-coverage",
        type=float,
        help="Krev minimum total dekningsprosent (0-100)",
    )

    selfhost_parity_consistency = sub.add_parser(
        "selfhost-parity-consistency",
        help="Sjekk consistency mellom parity-suiter og utvidet suite",
    )
    selfhost_parity_consistency.add_argument(
        "--scope",
        choices=["m1", "m2", "all"],
        default="m1",
        help="Velg hvilke consistency-sjekker som kjøres",
    )
    selfhost_parity_consistency.add_argument("--json", action="store_true", help="Skriv resultat som JSON")

    lock = sub.add_parser("lock", help="Generer dependency lockfile (norcode.lock)")
    lock.add_argument("--check", action="store_true", help="Feil hvis lockfile er manglende/utdatert")
    lock.add_argument("--verify", action="store_true", help="Verifiser path-digests i eksisterende lockfile")
    lock.add_argument("--json", action="store_true", help="Skriv lock-resultat som JSON")

    update = sub.add_parser("update", help="Oppdater dependencies fra registry")
    update.add_argument("package", nargs="?", help="Valgfri dependency å oppdatere")
    update.add_argument("--check", action="store_true", help="Feil hvis en dependency ville blitt oppdatert")
    update.add_argument("--json", action="store_true", help="Skriv update-resultat som JSON")
    update.add_argument("--pin", action="store_true", help="Krev låst ref for registry git-kilder")
    update.add_argument("--fetch", action="store_true", help="Materialiser registry git/url-kilder til lokal cache")
    update.add_argument("--refresh", action="store_true", help="Tving ny nedlasting ved --fetch")
    update.add_argument("--lock", action="store_true", help="Regenerer lockfile etter oppdatering")
    update.add_argument("--allow-untrusted", action="store_true", help="Overstyr trusted host-policy for denne kommandoen")

    registry_sign_cmd = sub.add_parser("registry-sign", help="Beregn/pinn SHA256 for packages/registry.toml")
    registry_sign_cmd.add_argument("--write-config", action="store_true", help="Skriv hash til [security].trusted_registry_sha256")
    registry_sign_cmd.add_argument("--json", action="store_true", help="Skriv resultat som JSON")

    registry_sync_cmd = sub.add_parser("registry-sync", help="Synkroniser remote registry-indeks til lokal cache")
    registry_sync_cmd.add_argument("--source", help="Overstyr registry source for denne kjøringen")
    registry_sync_cmd.add_argument("--allow-untrusted", action="store_true", help="Overstyr trusted host-policy for denne kommandoen")
    registry_sync_cmd.add_argument("--require-all", action="store_true", help="Feil hvis en eneste source feiler")
    registry_sync_cmd.add_argument("--no-fallback", action="store_true", help="Ikke bruk eksisterende cache ved source-feil")
    registry_sync_cmd.add_argument("--json", action="store_true", help="Skriv resultat som JSON")

    registry_mirror_cmd = sub.add_parser("registry-mirror", help="Bygg distribuerbar registry-speilfil fra lokale+remote entries")
    registry_mirror_cmd.add_argument("--output", help="Output-fil for mirror (default: build/registry_mirror.json)")
    registry_mirror_cmd.add_argument("--json", action="store_true", help="Skriv resultat som JSON")

    migrate_names_cmd = sub.add_parser("migrate-names", help="Migrer legacy navn (norsklang*) til Norscode-navn")
    migrate_names_cmd.add_argument("--apply", action="store_true", help="Utfør migrering (default er dry-run)")
    migrate_names_cmd.add_argument("--cleanup", action="store_true", help="Fjern legacy-filer etter vellykket migrering")
    migrate_names_cmd.add_argument("--check", action="store_true", help="Feil hvis migrering/cleanup fortsatt gjenstår")
    migrate_names_cmd.add_argument("--json", action="store_true", help="Skriv resultat som JSON")

    release = sub.add_parser("release", help="Forbered release (versjonsbump + changelog)")
    release.add_argument("--bump", choices=["major", "minor", "patch"], default="patch", help="Type semver-bump")
    release.add_argument("--version", help="Sett eksakt versjon (overstyrer --bump)")
    release.add_argument("--date", help="Release-dato (YYYY-MM-DD)")
    release.add_argument("--dry-run", action="store_true", help="Vis endringer uten å skrive filer")
    release.add_argument("--json", action="store_true", help="Skriv release-resultat som JSON")

    selfhost_ast_export = sub.add_parser("selfhost-ast-export", help="Eksporter .no via selfhost-parser til AST-json (.shast.json)")
    selfhost_ast_export.add_argument("file")
    selfhost_ast_export.add_argument("--output", help="Valgfri output-fil")

    ast_export = sub.add_parser("ast-export", help="Eksporter .no til AST-json (.nast.json)")
    ast_export.add_argument("file")
    ast_export.add_argument("--output", help="Valgfri output-fil")

    bytecode_build = sub.add_parser("bytecode-build", help="Bygg .no eller .nast.json til bytecode-json (.ncb.json)")
    bytecode_build.add_argument("file")
    bytecode_build.add_argument("--output", help="Valgfri output-fil")
    bytecode_build.add_argument("--ast", action="store_true", help="Tolker input som .nast.json i stedet for .no")

    bytecode_run = sub.add_parser("bytecode-run", help="Kjør bytecode-backenden fra .no, .nast.json eller .ncb.json")
    bytecode_run.add_argument("file")
    bytecode_run.add_argument("--bytecode", action="store_true", help="Tolker input som .ncb.json i stedet for .no")
    bytecode_run.add_argument("--ast", action="store_true", help="Tolker input som .nast.json i stedet for .no")

    selfhost_chain_export = sub.add_parser("selfhost-chain-export", help="Eksporter full selfhost AST-bundle inkl. imports")
    selfhost_chain_export.add_argument("file")
    selfhost_chain_export.add_argument("--output", help="Valgfri output-fil")

    selfhost_chain_run = sub.add_parser("selfhost-chain-run", help="Kjør full selfhost-kjede (.no -> selfhost AST -> bytecode -> VM)")
    selfhost_chain_run.add_argument("file")
    selfhost_chain_run.add_argument("--trace", action="store_true", help="Vis sporlogg ved feil")
    selfhost_chain_run.add_argument("--max-steps", type=int, default=5000000, help="Maks VM-steg før kjøring avbrytes")
    selfhost_chain_run.add_argument("--trace-focus", help="Logg kun funksjoner som matcher denne teksten")
    selfhost_chain_run.add_argument("--repeat-limit", type=int, default=0, help="Avbryt hvis samme VM-tilstand gjentas mer enn N ganger")
    selfhost_chain_run.add_argument("--expr-probe", help="Logg uttrykkstokens som matcher denne teksten")
    selfhost_chain_run.add_argument("--expr-probe-log", help="Skriv uttrykksprobe til fil")

    selfhost_chain_check = sub.add_parser("selfhost-chain-check", help="Sjekk et sett filer gjennom full selfhost-kjede")
    selfhost_chain_check.add_argument("files", nargs="*")
    selfhost_chain_check.add_argument("--trace", action="store_true", help="Ta med sporlogg ved feil")
    selfhost_chain_check.add_argument("--max-steps", type=int, default=5000000, help="Maks VM-steg per fil")
    selfhost_chain_check.add_argument("--trace-focus", help="Logg kun funksjoner som matcher denne teksten")
    selfhost_chain_check.add_argument("--repeat-limit", type=int, default=0, help="Avbryt hvis samme VM-tilstand gjentas mer enn N ganger")
    selfhost_chain_check.add_argument("--expr-probe", help="Logg uttrykkstokens som matcher denne teksten")
    selfhost_chain_check.add_argument("--expr-probe-log", help="Skriv uttrykksprobe til fil")

    test = sub.add_parser("test", help="Kjør én testfil eller alle i tests/")
    test.add_argument("file", nargs="?", help="Valgfri testfil")
    test.add_argument("--verbose", action="store_true", help="Vis output også for tester som består")
    test.add_argument("--json", action="store_true", help="Skriv testresultat som JSON")

    format_cmd = sub.add_parser("format", help="Formater en .no-fil")
    format_cmd.add_argument("file", help="Kildefil å formatere")
    format_cmd.add_argument("--check", action="store_true", help="Feil hvis filen ikke er formatert")
    format_cmd.add_argument("--diff", action="store_true", help="Vis diff uten å skrive filen")
    format_cmd.add_argument("--json", action="store_true", help="Skriv format-resultat som JSON")

    lint = sub.add_parser("lint", help="Kjør en enkel linter på en .no-fil")
    lint.add_argument("file", help="Kildefil å lint'e")
    lint.add_argument("--verbose", action="store_true", help="Vis alle funn eksplisitt")
    lint.add_argument("--json", action="store_true", help="Skriv lint-resultat som JSON")
    lint.add_argument("--check", action="store_true", help="Feil hvis linteren finner noe")

    bench = sub.add_parser("bench", help="Kjør faste ytelsesmålinger")
    bench.add_argument("--json", action="store_true", help="Skriv benchmark-resultat som JSON")

    smoke = sub.add_parser("smoke", help="Kjør fresh install/release smoke-test")
    smoke.add_argument("--json", action="store_true", help="Skriv smoke-resultat som JSON")

    fuzz = sub.add_parser("fuzz", help="Kjør negativ parser- og runtime-korpus")
    fuzz.add_argument("--json", action="store_true", help="Skriv fuzz-resultat som JSON")

    commands = sub.add_parser("commands", help="Vis stabil kommandooversikt")
    commands.add_argument("--json", action="store_true", help="Skriv kommandooversikt som JSON")

    serve = sub.add_parser("serve", help="Start en lokal webserver for en Norscode-app")
    serve.add_argument("file", help="Kildefil å kjøre som webapp")
    serve.add_argument("--host", default="127.0.0.1", help="Bind-adresse for serveren")
    serve.add_argument("--port", type=int, default=8000, help="Port for serveren")
    serve.add_argument("--reload", action="store_true", help="Rekompiler når kildefilen endrer seg")
    serve.add_argument("--once", action="store_true", help="Stopp etter første request (nyttig for smoke-test)")

    args = parser.parse_args()

    try:
        if args.cmd == "run":
            run_program(args.file)

        elif args.cmd == "repl":
            run_repl()

        elif args.cmd == "check":
            source_path, _program, alias_map, analyzer = check_program(args.file)
            print(f"Kilde: {source_path}")
            print(f"Aliaser: {alias_map}")
            print("Semantikk: OK")
            print(f"Funksjoner: {list(analyzer.functions.keys())}")

        elif args.cmd == "build":
            _source_path, c_path, exe_path, _alias_map, _analyzer = build_program(args.file)
            print(f"Generert C-fil: {c_path}")
            print("Kompilert med: clang")
            print(f"Kjørbar fil: {exe_path}")

        elif args.cmd == "add":
            if args.list:
                config_path, entries = list_registry_packages()
                print(f"Prosjektkonfig: {config_path}")
                if not entries:
                    print("Registry: ingen pakker funnet")
                else:
                    print(f"Registry: {len(entries)} pakker")
                    for name, meta in sorted(entries.items(), key=lambda item: item[0]):
                        desc = meta.get("description")
                        desc_text = f" - {desc}" if isinstance(desc, str) and desc.strip() else ""
                        version_text = f" @ {meta.get('version')}" if isinstance(meta.get("version"), str) else ""
                        source_text = f" [{meta.get('source')}]" if isinstance(meta.get("source"), str) else ""
                        if meta.get("kind") == "path":
                            target = str(meta["path"])
                        elif meta.get("kind") == "git":
                            target = _render_git_dependency(meta["git"], meta.get("ref"))
                        elif meta.get("kind") == "url":
                            target = _render_url_dependency(meta["url"])
                        else:
                            target = "<ukjent>"
                        print(f"  {name}{version_text} => {target}{source_text}{desc_text}")
                return

            if not args.package:
                raise RuntimeError("Mangler pakkenavn. Bruk: add <pakke> eller add --list")

            config_path, dep_name, dep_value, package_name, dep_kind, changed = add_dependency(
                args.package,
                package_path=args.path,
                dep_name_override=args.name,
                git_url=args.git,
                git_ref=args.ref,
                tarball_url=args.url,
                fetch=args.fetch,
                refresh=args.refresh,
                pin=args.pin,
                expected_sha256=args.sha256,
                allow_untrusted=args.allow_untrusted,
            )
            print(f"Konfig: {config_path}")
            print(f"Pakke: {package_name}")
            print(f"Kilde: {dep_kind}")
            print(f"Dependency: {dep_name} = \"{dep_value}\"")
            print("Status: oppdatert" if changed else "Status: uendret")

        elif args.cmd == "debug":
            show_tokens = args.tokens
            show_ast = args.ast
            show_symbols = args.symbols
            if not (show_tokens or show_ast or show_symbols):
                show_symbols = True

            payload = debug_source(args.file, show_tokens=show_tokens, show_ast=show_ast, show_symbols=show_symbols)
            if args.json:
                print(json.dumps(payload, ensure_ascii=False, indent=2))
            else:
                print(f"Kilde: {payload['source']}")
                print(f"Imports: {len(payload.get('imports', []))}")
                for imp in payload.get("imports", []):
                    alias_text = f" som {imp['alias']}" if imp.get("alias") else ""
                    print(f"  bruk {imp['module']}{alias_text}")

                print(f"Funksjoner: {len(payload.get('functions', []))}")
                for fn in payload.get("functions", []):
                    print(f"  {fn['name']} (params: {fn['params']})")

                if "tokens" in payload:
                    print("Tokens:")
                    for tok in payload["tokens"]:
                        print(f"  {tok['line']}:{tok['column']} {tok['type']} {repr(tok['value'])}")

                if "symbols" in payload:
                    print("Symboler:")
                    for sym in payload["symbols"]:
                        print(
                            f"  {sym['name']} -> modul={sym['module']} "
                            f"params={sym['params']} return={sym['return_type']}"
                        )
                    if payload.get("aliases"):
                        print("Aliaser:")
                        for alias, module_name in payload["aliases"].items():
                            print(f"  {alias} => {module_name}")

                if "ast" in payload:
                    print("AST:")
                    print(json.dumps(payload["ast"], ensure_ascii=False, indent=2))

        elif args.cmd == "disasm":
            source_path, code = disasm_program(args.file)
            print(f"Kilde: {source_path}")
            print("Generert C:")
            print(code)

        elif args.cmd == "ir-disasm":
            if args.diff:
                source_path, py_ok, py_lines, py_err = ir_disasm_source_captured(args.file, strict=args.strict, engine="python")
                _source_path2, sh_ok, sh_lines, sh_err = ir_disasm_source_captured(args.file, strict=args.strict, engine="selfhost")

                diff_lines: list[str] = []
                diff_text = ""

                if py_ok != sh_ok:
                    if args.json:
                        payload = {
                            "source": str(source_path),
                            "strict": args.strict,
                            "match": False,
                            "python_ok": py_ok,
                            "python_error": py_err,
                            "selfhost_ok": sh_ok,
                            "selfhost_error": sh_err,
                        }
                        if args.save_diff:
                            diff_text = (
                                "MISMATCH (ulik feilstatus)\n"
                                f"python: {'OK' if py_ok else py_err}\n"
                                f"selfhost: {'OK' if sh_ok else sh_err}\n"
                            )
                            Path(args.save_diff).expanduser().write_text(diff_text, encoding="utf-8")
                            payload["saved_diff"] = str(Path(args.save_diff).expanduser().resolve())
                        print(json.dumps(payload, ensure_ascii=False, indent=2))
                    else:
                        print(f"Kilde: {source_path}")
                        print("Motor: diff (python vs selfhost)")
                        print("IR disasm: MISMATCH (ulik feilstatus)")
                        print(f"python: {'OK' if py_ok else py_err}")
                        print(f"selfhost: {'OK' if sh_ok else sh_err}")
                        if args.save_diff:
                            diff_text = (
                                "MISMATCH (ulik feilstatus)\n"
                                f"python: {'OK' if py_ok else py_err}\n"
                                f"selfhost: {'OK' if sh_ok else sh_err}\n"
                            )
                            save_path = Path(args.save_diff).expanduser()
                            save_path.write_text(diff_text, encoding="utf-8")
                            print(f"Diff lagret: {save_path.resolve()}")
                    sys.exit(1)

                if not py_ok and not sh_ok:
                    if py_err != sh_err:
                        if args.json:
                            payload = {
                                "source": str(source_path),
                                "strict": args.strict,
                                "match": False,
                                "python_error": py_err,
                                "selfhost_error": sh_err,
                            }
                            if args.save_diff:
                                diff_text = (
                                    "MISMATCH (ulik feilmelding)\n"
                                    f"python: {py_err}\n"
                                    f"selfhost: {sh_err}\n"
                                )
                                Path(args.save_diff).expanduser().write_text(diff_text, encoding="utf-8")
                                payload["saved_diff"] = str(Path(args.save_diff).expanduser().resolve())
                            print(json.dumps(payload, ensure_ascii=False, indent=2))
                        else:
                            print(f"Kilde: {source_path}")
                            print("Motor: diff (python vs selfhost)")
                            print("IR disasm: MISMATCH (ulik feilmelding)")
                            print(f"python: {py_err}")
                            print(f"selfhost: {sh_err}")
                            if args.save_diff:
                                diff_text = (
                                    "MISMATCH (ulik feilmelding)\n"
                                    f"python: {py_err}\n"
                                    f"selfhost: {sh_err}\n"
                                )
                                save_path = Path(args.save_diff).expanduser()
                                save_path.write_text(diff_text, encoding="utf-8")
                                print(f"Diff lagret: {save_path.resolve()}")
                        sys.exit(1)
                    raise RuntimeError(py_err)

                if args.json:
                    payload = {
                        "source": str(source_path),
                        "strict": args.strict,
                        "match": py_lines == sh_lines,
                        "python_lines": py_lines,
                        "selfhost_lines": sh_lines,
                    }
                    if args.fail_on_warning:
                        _src_w1, py_strict_ok, py_strict_lines, py_strict_err = ir_disasm_source_captured(args.file, strict=True, engine="python")
                        _src_w2, sh_strict_ok, sh_strict_lines, sh_strict_err = ir_disasm_source_captured(args.file, strict=True, engine="selfhost")
                        payload["strict_warning_match"] = (
                            py_strict_ok == sh_strict_ok
                            and py_strict_lines == sh_strict_lines
                            and py_strict_err == sh_strict_err
                        )
                        payload["python_strict_ok"] = py_strict_ok
                        payload["python_strict_error"] = py_strict_err
                        payload["selfhost_strict_ok"] = sh_strict_ok
                        payload["selfhost_strict_error"] = sh_strict_err
                    print(json.dumps(payload, ensure_ascii=False, indent=2))
                else:
                    print(f"Kilde: {source_path}")
                    print("Motor: diff (python vs selfhost)")
                    if py_lines == sh_lines:
                        print("IR disasm: MATCH")
                        for line in py_lines:
                            print(line)
                    else:
                        print("IR disasm: MISMATCH")
                        diff_lines = list(difflib.unified_diff(
                            py_lines,
                            sh_lines,
                            fromfile="python",
                            tofile="selfhost",
                            lineterm="",
                        ))
                        for line in diff_lines:
                            print(line)
                        if args.save_diff:
                            save_path = Path(args.save_diff).expanduser()
                            save_path.write_text("\n".join(diff_lines) + "\n", encoding="utf-8")
                            print(f"Diff lagret: {save_path.resolve()}")
                        sys.exit(1)

                    if args.fail_on_warning:
                        _src_w1, py_strict_ok, py_strict_lines, py_strict_err = ir_disasm_source_captured(args.file, strict=True, engine="python")
                        _src_w2, sh_strict_ok, sh_strict_lines, sh_strict_err = ir_disasm_source_captured(args.file, strict=True, engine="selfhost")
                        warning_match = (
                            py_strict_ok == sh_strict_ok
                            and py_strict_lines == sh_strict_lines
                            and py_strict_err == sh_strict_err
                        )
                        if warning_match:
                            print("Warning check: MATCH")
                        else:
                            print("Warning check: MISMATCH")
                            print(f"python strict: {'OK' if py_strict_ok else py_strict_err}")
                            print(f"selfhost strict: {'OK' if sh_strict_ok else sh_strict_err}")
                            if args.save_diff:
                                diff_text = (
                                    "Warning check mismatch\n"
                                    f"python strict: {'OK' if py_strict_ok else py_strict_err}\n"
                                    f"selfhost strict: {'OK' if sh_strict_ok else sh_strict_err}\n"
                                )
                                save_path = Path(args.save_diff).expanduser()
                                save_path.write_text(diff_text, encoding="utf-8")
                                print(f"Diff lagret: {save_path.resolve()}")
                            sys.exit(1)
            else:
                source_path, lines = ir_disasm_source(args.file, strict=args.strict, engine=args.engine)
                if args.json:
                    payload = {
                        "source": str(source_path),
                        "engine": args.engine,
                        "lines": lines,
                    }
                    print(json.dumps(payload, ensure_ascii=False, indent=2))
                else:
                    print(f"Kilde: {source_path}")
                    print(f"Motor: {args.engine}")
                    print("IR disasm:")
                    for line in lines:
                        print(line)

        elif args.cmd == "update-snapshots":
            fixture_path, updated, total = update_ir_snapshots(check_only=args.check)
            payload = {
                "fixture": str(Path(fixture_path).resolve()),
                "check_only": bool(args.check),
                "updated": int(updated),
                "strict_cases": int(total),
            }
            if args.json:
                print(json.dumps(payload, ensure_ascii=False, indent=2))
            else:
                print(f"Oppdatert snapshot-fixture: {fixture_path}")
                print(f"Strict-cases: {total}")
                if args.check:
                    print(f"Avvik funnet: {updated}")
                else:
                    print(f"Endringer skrevet: {updated}")
            if args.check and updated > 0:
                sys.exit(1)

        elif args.cmd == "update-selfhost-parity-fixtures":
            payload = update_selfhost_parser_fixtures(
                check_only=args.check,
                suite=args.suite,
                sync_m2=(not args.no_sync_m2),
            )
            if args.json:
                print(json.dumps(payload, ensure_ascii=False, indent=2))
            else:
                print(f"Suite: {payload['suite']}")
                print(f"Cases: {payload['cases']}")
                print(f"Avvik: {payload['updated']}")
                if payload.get("m2_sync") is not None:
                    m2_sync = payload["m2_sync"]
                    print(
                        f"- M2 sync: {m2_sync.get('m2_cases', 0)} cases, "
                        f"{m2_sync.get('updated', 0)} oppdateringer ({m2_sync.get('fixture')})"
                    )
                for row in payload["fixtures"]:
                    print(
                        f"- {row['label']}: {row['cases']} cases, "
                        f"{row['updated']} oppdateringer ({row['fixture']})"
                    )
                if args.check:
                    print("Status: check-only")
                else:
                    print("Status: skrevet")
            if args.check and payload["updated"] > 0:
                sys.exit(1)

        elif args.cmd == "sync-selfhost-parity-m2":
            payload = sync_selfhost_parser_m2_fixture(check_only=args.check)
            if args.json:
                print(json.dumps(payload, ensure_ascii=False, indent=2))
            else:
                print(f"OK: {'ja' if payload.get('ok') else 'nei'}")
                print(f"M2 fixture: {payload['fixture']}")
                print(f"M1 cases: {payload['m1_cases']}")
                print(f"Core cases: {payload['core_cases']}")
                print(f"M2 cases (beregnet): {payload['m2_cases']}")
                print(f"Avvik: {payload['updated']}")
                print(f"M1-mangler i core: {payload['missing_m1_from_core_count']}")
                if args.check:
                    print("Status: check-only")
                else:
                    print("Status: synkronisert")
            if args.check and not payload.get("ok"):
                sys.exit(1)

        elif args.cmd == "ci":
            payload = run_ci_pipeline(
                json_output=args.json,
                check_names=args.check_names,
                parity_suite=args.parity_suite,
                require_selfhost_ready=args.require_selfhost_ready,
            )
            if args.json:
                print(json.dumps(payload, ensure_ascii=False, indent=2))

        elif args.cmd == "selfhost-parity":
            payload = run_selfhost_parser_parity(suite=args.suite)
            if args.json:
                print(json.dumps(payload, ensure_ascii=False, indent=2))
            else:
                print(f"Suite: {payload['suite']}")
                print(f"OK: {'ja' if payload['ok'] else 'nei'}")
                print(f"Cases: {payload['case_count']}")
                print(
                    f"Fordeling: uttrykk={payload['expression_cases']} "
                    f"skript={payload['script_cases']} "
                    f"linje={payload['line_cases']} feil={payload['error_cases']}"
                )
                print(f"Tid: {payload['duration_ms']} ms")
                for item in payload["results"]:
                    status = "OK" if item.get("success") else "FEIL"
                    print(
                        f"- {status}: {item['source']} "
                        f"({item.get('case_count', 0)} cases, "
                        f"{item.get('error_cases', 0)} feil)"
                    )
                    if not item.get("success") and item.get("stderr"):
                        print(item["stderr"].rstrip())
            if not payload["ok"]:
                sys.exit(1)

        elif args.cmd == "selfhost-parity-progress":
            payload = run_selfhost_parity_progress()
            coverage = payload.get("coverage", {}) if isinstance(payload.get("coverage"), dict) else {}
            total_coverage = coverage.get("total_pct")
            if args.min_coverage is not None:
                threshold = float(args.min_coverage)
                if threshold < 0 or threshold > 100:
                    raise RuntimeError("--min-coverage må være mellom 0 og 100")
                payload["min_coverage_required"] = threshold
                payload["min_coverage_ok"] = (
                    isinstance(total_coverage, (int, float)) and float(total_coverage) >= threshold
                )
            else:
                payload["min_coverage_required"] = None
                payload["min_coverage_ok"] = True
            if args.json:
                print(json.dumps(payload, ensure_ascii=False, indent=2))
            else:
                print(f"OK: {'ja' if payload.get('ok') else 'nei'}")
                print(f"Klar for full coverage: {'ja' if payload.get('ready') else 'nei'}")
                print(
                    "Dekning: "
                    f"uttrykk={coverage.get('expression_pct', 0)}% "
                    f"skript={coverage.get('script_pct', 0)}% "
                    f"total={coverage.get('total_pct', 0)}%"
                )
                if args.min_coverage is not None:
                    print(
                        "Coverage-gate: "
                        f"min={payload.get('min_coverage_required')}% "
                        f"status={'OK' if payload.get('min_coverage_ok') else 'FEIL'}"
                    )
                print(
                    "Cases: "
                    f"m1={payload.get('m1', {}).get('case_count', 0)} "
                    f"m2={payload.get('m2', {}).get('case_count', 0)} "
                    f"utvidet={payload.get('extended', {}).get('case_count', 0)}"
                )
                print(
                    "Avvik: "
                    f"missing={coverage.get('missing_in_m1_m2_count', 0)} "
                    f"extra={coverage.get('extra_in_m1_m2_count', 0)} "
                    f"overlap={coverage.get('overlap_count', 0)}"
                )
                consistency = payload.get("consistency", {})
                print(
                    "Consistency(all): "
                    f"{'OK' if consistency.get('ok') else 'FEIL'} "
                    f"({consistency.get('checked_cases', 0)} cases, "
                    f"{consistency.get('mismatch_count', 0)} avvik)"
                )
                print(f"Tid: {payload.get('duration_ms', 0)} ms")
                if payload.get("stderr") and not payload.get("ok"):
                    print(str(payload.get("stderr", "")).rstrip())
            if not payload.get("ok"):
                sys.exit(1)
            if args.require_ready and not payload.get("ready"):
                if not args.json:
                    print("Gate-feil: progress er ikke ready")
                sys.exit(1)
            if not payload.get("min_coverage_ok"):
                if not args.json:
                    print(
                        "Gate-feil: total dekningsprosent under minimum "
                        f"({coverage.get('total_pct', 0)} < {payload.get('min_coverage_required')})"
                    )
                sys.exit(1)

        elif args.cmd == "selfhost-parity-gate":
            threshold = None
            if args.min_coverage is not None:
                threshold = float(args.min_coverage)
                if threshold < 0 or threshold > 100:
                    raise RuntimeError("--min-coverage må være mellom 0 og 100")
            payload = run_selfhost_parity_gate(min_coverage=threshold)
            if args.json:
                print(json.dumps(payload, ensure_ascii=False, indent=2))
            else:
                print(f"OK: {'ja' if payload.get('ok') else 'nei'}")
                print(f"Klar for gate: {'ja' if payload.get('ready') else 'nei'}")
                if payload.get("coverage_total_pct") is not None:
                    print(f"Dekning: {payload['coverage_total_pct']}%")
                if payload.get("min_coverage") is not None:
                    print(f"Min coverage: {payload['min_coverage']}%")
            if not payload.get("ok"):
                sys.exit(1)

        elif args.cmd == "selfhost-parity-consistency":
            if args.scope == "m1":
                payload = run_selfhost_parser_suite_consistency_check(
                    SELFHOST_PARSER_M1_FIXTURE,
                    SELFHOST_PARSER_EXTENDED_FIXTURE,
                )
            elif args.scope == "m2":
                payload = run_selfhost_parser_suite_subset_consistency_check(
                    SELFHOST_PARSER_M2_FIXTURE,
                    SELFHOST_PARSER_EXTENDED_FIXTURE,
                    "m2",
                )
            else:
                payload = run_selfhost_parser_suite_all_consistency_check(
                    SELFHOST_PARSER_M1_FIXTURE,
                    SELFHOST_PARSER_M2_FIXTURE,
                    SELFHOST_PARSER_EXTENDED_FIXTURE,
                )
            if args.json:
                print(json.dumps(payload, ensure_ascii=False, indent=2))
            else:
                print(f"Scope: {payload.get('scope', 'm1')}")
                print(f"OK: {'ja' if payload.get('success') else 'nei'}")
                print(f"Sjekkede cases: {payload.get('checked_cases', 0)}")
                print(f"Avvik: {payload.get('mismatch_count', 0)}")
                print(f"Tid: {payload.get('duration_ms', 0)} ms")
                checks = payload.get("checks")
                if isinstance(checks, dict):
                    m1 = checks.get("m1", {})
                    m2 = checks.get("m2", {})
                    print(
                        f"- m1: {'OK' if m1.get('success') else 'FEIL'} "
                        f"({m1.get('checked_cases', 0)} cases, {m1.get('mismatch_count', 0)} avvik)"
                    )
                    print(
                        f"- m2: {'OK' if m2.get('success') else 'FEIL'} "
                        f"({m2.get('checked_cases', 0)} cases, {m2.get('mismatch_count', 0)} avvik)"
                    )
                    print(
                        f"- coverage: {checks.get('coverage_checked_cases', 0)} cases, "
                        f"{checks.get('coverage_mismatch_count', 0)} avvik"
                    )
                if not payload.get("success") and payload.get("stderr"):
                    print(payload["stderr"].rstrip())
            if not payload.get("success"):
                sys.exit(1)

        elif args.cmd == "lock":
            if args.verify:
                lock_path, ok, results = verify_lockfile()
                if args.json:
                    print(json.dumps({"lockfile": str(lock_path), "ok": ok, "verify": True, "results": results}, ensure_ascii=False, indent=2))
                else:
                    print(f"Lockfile: {lock_path}")
                    print("Verify:")
                    for row in results:
                        print(f"  {row['name']}: {row['status']}")
                if not ok:
                    sys.exit(1)
            else:
                lock_path, ok, status = generate_lockfile(check_only=args.check)
                if args.json:
                    print(json.dumps({"lockfile": str(lock_path), "ok": ok, "status": status, "check": args.check}, ensure_ascii=False, indent=2))
                else:
                    print(f"Lockfile: {lock_path}")
                    print(f"Status: {status}")
                if args.check and not ok:
                    sys.exit(1)

        elif args.cmd == "update":
            payload = update_dependencies(
                package=args.package,
                check_only=args.check,
                pin=args.pin,
                fetch=args.fetch,
                refresh=args.refresh,
                with_lock=args.lock,
                allow_untrusted=args.allow_untrusted,
            )
            if args.json:
                print(json.dumps(payload, ensure_ascii=False, indent=2))
            else:
                print(f"Konfig: {payload['config']}")
                print(f"Target: {payload['target']}")
                print(f"Updated: {payload['updated']}")
                print(f"Unchanged: {payload['unchanged']}")
                print(f"Skipped: {payload['skipped']}")
                for item in payload["items"]:
                    name = item["name"]
                    status = item["status"]
                    if status == "updated":
                        print(f"  {name}: oppdatert -> {item['to']}")
                    elif status == "unchanged":
                        print(f"  {name}: uendret")
                    else:
                        print(f"  {name}: hoppet over ({item.get('reason', 'ukjent')})")
                if payload.get("lock"):
                    print(f"Lockfile: {payload['lock']['path']} ({payload['lock']['status']})")
            if args.check and payload["updated"] > 0:
                sys.exit(1)

        elif args.cmd == "registry-sign":
            payload = registry_sign(write_config=args.write_config)
            if args.json:
                print(json.dumps(payload, ensure_ascii=False, indent=2))
            else:
                print(f"Registry: {payload['registry']}")
                print(f"SHA256: {payload['sha256']}")
                if payload["written_to_config"]:
                    print(f"Konfig: {payload['config']}")
                    print("Status: oppdatert" if payload["config_changed"] else "Status: uendret")

        elif args.cmd == "registry-sync":
            payload = registry_sync(
                source_override=args.source,
                allow_untrusted=args.allow_untrusted,
                require_all=args.require_all,
                fallback_to_cache=not args.no_fallback,
            )
            if args.json:
                print(json.dumps(payload, ensure_ascii=False, indent=2))
            else:
                print(f"Cache: {payload['cache']}")
                print(f"Kilder: {len(payload['sources'])}")
                for src in payload["sources"]:
                    print(f"  - {src}")
                print(f"Pakker i cache: {payload['count']}")
                if payload.get("failed_sources"):
                    print("Feilede kilder:")
                    for row in payload["failed_sources"]:
                        print(f"  - {row['source']}: {row['error']}")
                if payload.get("stale_fallback_used"):
                    print("Fallback: bruker eksisterende cache")

        elif args.cmd == "registry-mirror":
            payload = registry_mirror(output_file=args.output)
            if args.json:
                print(json.dumps(payload, ensure_ascii=False, indent=2))
            else:
                print(f"Mirror: {payload['output']}")
                print(f"Pakker: {payload['count']}")

        elif args.cmd == "migrate-names":
            payload = migrate_names(apply_changes=args.apply, cleanup_legacy=args.cleanup)
            if args.json:
                print(json.dumps(payload, ensure_ascii=False, indent=2))
            else:
                mode = "apply+cleanup" if args.apply and args.cleanup else ("apply" if args.apply else ("dry-run+cleanup" if args.cleanup else "dry-run"))
                print(f"Prosjekt: {payload['project_dir']}")
                print(f"Modus: {mode}")
                for action in payload["actions"]:
                    reason = action.get("reason")
                    if reason:
                        print(
                            f"  {action['kind']}: {action['legacy']} -> {action['primary']} "
                            f"[{action['status']}: {reason}]"
                        )
                    else:
                        print(
                            f"  {action['kind']}: {action['legacy']} -> {action['primary']} "
                            f"[{action['status']}]"
                        )
                print(
                    "Oppsummert: "
                    f"copied={payload['copied']} planned={payload['planned']} "
                    f"removed={payload['removed']} planned_remove={payload['planned_remove']} "
                    f"skipped={payload['skipped']}"
                )
            if args.check and payload["needs_migration"]:
                sys.exit(1)

        elif args.cmd == "release":
            payload = prepare_release(
                version=args.version,
                bump=args.bump,
                dry_run=args.dry_run,
                release_date=args.date,
            )
            if args.json:
                print(json.dumps(payload, ensure_ascii=False, indent=2))
            else:
                mode = "DRY-RUN" if payload["dry_run"] else "SKREVET"
                print(f"Release: {mode}")
                print(f"Versjon: {payload['old_version']} -> {payload['new_version']}")
                print(f"Dato: {payload['release_date']}")
                print(f"Pyproject: {'oppdatert' if payload['changed_pyproject'] else 'uendret'} ({payload['pyproject']})")
                print(f"Changelog: {'oppdatert' if payload['changed_changelog'] else 'uendret'} ({payload['changelog']})")

        elif args.cmd == "selfhost-ast-export":
            from compiler.selfhost_ast_bridge import export_selfhost_ast
            out_path = export_selfhost_ast(args.file, output=args.output)
            print(f"Selfhost AST: {out_path}")

        elif args.cmd == "ast-export":
            from compiler.ast_bridge import export_ast
            out_path = export_ast(args.file, output=args.output)
            print(f"AST: {out_path}")

        elif args.cmd == "bytecode-build":
            from compiler.bytecode_backend import build_command
            if args.ast:
                out_path = build_command(ast_file=args.file, output=args.output)
            else:
                out_path = build_command(source_file=args.file, output=args.output)
            print(f"Bytecode: {out_path}")

        elif args.cmd == "bytecode-run":
            from compiler.bytecode_backend import run_command
            if args.bytecode:
                result = run_command(bytecode_file=args.file)
            elif args.ast:
                result = run_command(ast_file=args.file)
            else:
                result = run_command(source_file=args.file)
            if result is not None:
                print(f"Return: {result}")

        elif args.cmd == "selfhost-chain-export":
            out_path = export_selfhost_ast_bundle(args.file, output=args.output)
            print(f"Selfhost chain AST: {out_path}")

        elif args.cmd == "selfhost-chain-run":
            result = run_chain(
                args.file,
                trace=args.trace,
                max_steps=args.max_steps,
                trace_focus=args.trace_focus,
                repeat_limit=args.repeat_limit,
                expr_probe=args.expr_probe,
                expr_probe_log=args.expr_probe_log,
            )
            if result is not None:
                print(f"Return: {result}")

        elif args.cmd == "selfhost-chain-check":
            payload = check_chain(
                args.files,
                trace=args.trace,
                max_steps=args.max_steps,
                trace_focus=args.trace_focus,
                repeat_limit=args.repeat_limit,
                expr_probe=args.expr_probe,
                expr_probe_log=args.expr_probe_log,
            )
            print(f"{payload['passed']}/{payload['total']} OK")
            for row in payload['results']:
                status = "OK" if row.get("ok") else "FEIL"
                detail = row.get("result") if row.get("ok") else row.get("error")
                print(f"- {status}: {row['file']}" + (f" -> {detail}" if detail is not None else ""))
            if not payload['ok']:
                sys.exit(1)

        elif args.cmd == "test":
            if args.file:
                result = run_test_file(args.file)
                if args.json:
                    payload = {
                        "mode": "single",
                        "results": [result],
                        "summary": summarize_test_results([result]),
                    }
                    print(json.dumps(payload, ensure_ascii=False, indent=2))
                else:
                    print_test_result(result, verbose=args.verbose)
                if not result["success"]:
                    sys.exit(1)
            else:
                results = run_all_tests(verbose=args.verbose, quiet=args.json)
                if args.json:
                    payload = {
                        "mode": "all",
                        "results": results,
                        "summary": summarize_test_results(results),
                    }
                    print(json.dumps(payload, ensure_ascii=False, indent=2))
                if any(not r["success"] for r in results):
                    sys.exit(1)

        elif args.cmd == "lint":
            result = lint_program(args.file)
            if args.json:
                payload = {
                    "mode": "single",
                    "result": result,
                    "summary": summarize_lint_results(result),
                }
                print(json.dumps(payload, ensure_ascii=False, indent=2))
            else:
                print_lint_result(result, verbose=args.verbose)
            if args.check and result["issues"]:
                sys.exit(1)

        elif args.cmd == "bench":
            payload = run_benchmark_suite()
            if args.json:
                print(json.dumps(payload, ensure_ascii=False, indent=2))
            else:
                print(f"OK: {'ja' if payload['ok'] else 'nei'}")
                print(f"Tid: {payload['total_duration_ms']} ms")
                print(f"Topptid: {payload['max_duration_ms']} ms")
                print(f"Budget-avvik: {payload['budget_exceeded_count']}")
                for row in payload["benchmarks"]:
                    status = "OK" if row["ok"] and row["within_budget"] else "FEIL"
                    print(
                        f"- {status}: {row['name']} "
                        f"({row['duration_ms']} ms, budsjett {row['budget_ms']} ms)"
                    )
            if not payload["ok"]:
                sys.exit(1)

        elif args.cmd == "smoke":
            payload = run_smoke_suite()
            if args.json:
                print(json.dumps(payload, ensure_ascii=False, indent=2))
            else:
                print(f"OK: {'ja' if payload['ok'] else 'nei'}")
                print(f"Release: {payload['release_version']}")
                print(f"Prefix: {payload['temp_prefix']}")
                for row in payload["steps"]:
                    status = "OK" if row.get("ok") else "FEIL"
                    print(f"- {status}: {row['name']} ({row.get('duration_ms', 0)} ms)")
            if not payload["ok"]:
                sys.exit(1)

        elif args.cmd == "fuzz":
            payload = run_negative_suite()
            if args.json:
                print(json.dumps(payload, ensure_ascii=False, indent=2))
            else:
                print(f"OK: {'ja' if payload['ok'] else 'nei'}")
                print(f"Parser-feil fanget: {len(payload['parser_cases']) - payload['parser_failures']}/{len(payload['parser_cases'])}")
                print(f"Runtime-feil fanget: {len(payload['runtime_cases']) - payload['runtime_failures']}/{len(payload['runtime_cases'])}")
                for row in payload["parser_cases"]:
                    status = "OK" if row["ok"] else "FEIL"
                    print(f"- {status}: parser/{row['name']} ({row['duration_ms']} ms)")
                for row in payload["runtime_cases"]:
                    status = "OK" if row["ok"] else "FEIL"
                    print(f"- {status}: runtime/{row['name']} ({row['duration_ms']} ms)")
            if not payload["ok"]:
                sys.exit(1)

        elif args.cmd == "commands":
            payload = {
                "prog": parser.prog,
                "commands": _command_overview(),
            }
            if args.json:
                print(json.dumps(payload, ensure_ascii=False, indent=2))
            else:
                print(f"CLI: {payload['prog']}")
                for row in payload["commands"]:
                    print(f"- {row['name']}: {row['help']}")

        elif args.cmd == "serve":
            serve_program(
                args.file,
                host=args.host,
                port=args.port,
                reload_enabled=args.reload,
                once=args.once,
            )

        elif args.cmd == "format":
            result = format_program_file(args.file, check=args.check, diff=args.diff)
            if args.json:
                payload = {
                    "mode": "single",
                    "result": result,
                    "summary": {
                        "source": result["source"],
                        "changed": result["changed"],
                        "written": result["written"],
                    },
                }
                print(json.dumps(payload, ensure_ascii=False, indent=2))
            else:
                if result["changed"]:
                    if args.check:
                        print(f"Uformatert: {result['source']}")
                    elif not args.diff:
                        print(f"Formatert: {result['source']}")
                else:
                    print(f"Allerede formatert: {result['source']}")
            if args.check and result["changed"]:
                sys.exit(1)

        else:
            parser.print_help()
            sys.exit(1)

    except Exception as e:
        print(f"Feil: {e}")
        sys.exit(1)


if __name__ == "__main__":
    main()
