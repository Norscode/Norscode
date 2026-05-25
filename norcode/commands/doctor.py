"""Distribution/installation doctor command.

Checks the release-ish layout, version visibility and a small compilation smoke
path so users can verify an installed Norscode without knowing bootstrap
internals.
"""

from __future__ import annotations

import re
import subprocess
import tempfile
from pathlib import Path

from norcode.commands.base import CommandModule
from norcode.diagnostics import run_diagnostics


VERSION_RE = re.compile(r"^Norscode\s+(.+?)\s*$")


def register_arguments(parser) -> None:
    parser.add_argument("--json", action="store_true", help="Skriv resultat som JSON")


def _repo_root() -> Path:
    return Path(__file__).resolve().parents[2]


def _read_version_from_toml(root: Path) -> str:
    toml_path = root / "norcode.toml"
    if not toml_path.exists():
        return "ukjent"
    for line in toml_path.read_text(encoding="utf-8").splitlines():
        if line.strip().startswith("version"):
            parts = line.split("=", 1)
            if len(parts) == 2:
                return parts[1].strip().strip('"')
    return "ukjent"


def _run(cmd: list[str], cwd: Path) -> subprocess.CompletedProcess[str]:
    return subprocess.run(cmd, cwd=str(cwd), text=True, capture_output=True, check=False)


def _check_version(root: Path, version: str) -> dict[str, object]:
    completed = _run(["./bin/nc", "--version"], root)
    ok = completed.returncode == 0 and version in completed.stdout
    return {
        "name": "version",
        "ok": ok,
        "returncode": completed.returncode,
        "stdout": completed.stdout,
        "stderr": completed.stderr,
    }


def _check_layout(root: Path) -> dict[str, object]:
    required = ["bin", "std", "selfhost", "examples", "docs"]
    missing = [name for name in required if not (root / name).exists()]
    return {
        "name": "layout",
        "ok": not missing,
        "missing": missing,
    }


def _check_temp_write() -> dict[str, object]:
    temp_dir = Path(tempfile.mkdtemp(prefix="norscode-doctor-"))
    probe = temp_dir / "probe.txt"
    try:
        probe.write_text("ok", encoding="utf-8")
        ok = probe.read_text(encoding="utf-8") == "ok"
        return {
            "name": "temp_write",
            "ok": ok,
            "path": str(probe),
        }
    finally:
        try:
            probe.unlink()
        except FileNotFoundError:
            pass
        try:
            temp_dir.rmdir()
        except OSError:
            pass


def _check_diagnose(root: Path) -> dict[str, object]:
    payload = run_diagnostics(path=str(root))
    return {
        "name": "diagnose",
        "ok": bool(payload.get("ok")),
        "returncode": 0 if payload.get("ok") else 1,
        "stdout": "",
        "stderr": "",
        "payload": payload,
    }


def _check_compile_smoke(root: Path) -> dict[str, object]:
    completed = _run(["./bin/nc", "run", "tests/test_json.no"], root)
    return {
        "name": "compile_smoke",
        "ok": completed.returncode == 0,
        "returncode": completed.returncode,
        "stdout": completed.stdout,
        "stderr": completed.stderr,
    }


def run(args) -> int:
    root = _repo_root()
    version = _read_version_from_toml(root)
    checks = [
        _check_layout(root),
        _check_version(root, version),
        _check_temp_write(),
        _check_diagnose(root),
        _check_compile_smoke(root),
    ]
    ok = all(check.get("ok") for check in checks)
    summary = {
        "ok": ok,
        "version": version,
        "checks": checks,
    }

    if getattr(args, "json", False):
        import json

        print(json.dumps(summary, ensure_ascii=False, indent=2))
    else:
        print(f"Doctor OK: {'ja' if ok else 'nei'}")
        print(f"Versjon: {version}")
        for check in checks:
            status = "OK" if check["ok"] else "FEIL"
            print(f"- {status}: {check['name']}")
            if check["name"] == "layout" and check.get("missing"):
                print(f"  mangler: {', '.join(check['missing'])}")
            if check["name"] == "diagnose" and isinstance(check.get("payload"), dict):
                payload = check["payload"]
                print(f"  Diagnose OK: {'ja' if payload.get('ok') else 'nei'}")
                print(f"  Sti: {payload.get('root')}")
                print(f"  Prosjektrot: {payload.get('project_root') or 'fant ikke'}")
            if not check["ok"] and check.get("stderr"):
                for line in str(check["stderr"]).splitlines():
                    print(f"  {line}")

    return 0 if ok else 1


DOCTOR_COMMAND = CommandModule(
    name="doctor",
    help="Kjør en rask installasjons- og distribusjonskontroll",
    register_arguments=register_arguments,
    run=run,
)
