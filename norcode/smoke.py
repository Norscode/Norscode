from __future__ import annotations

import datetime as dt
import subprocess
import tempfile
import time
from pathlib import Path


def _repo_root() -> Path:
    return Path(__file__).resolve().parents[1]


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
