"""Python-fri fuzz suite — bruke nc-vm for negative testing."""
from __future__ import annotations
import json, os, subprocess, tempfile, time
from pathlib import Path

def _nc_vm():
    here = Path(__file__).resolve().parents[1]
    return str(here / "dist" / "nc-vm")

def _run_source_via_ncvm(source: str, expect_failure: bool = True) -> tuple[bool, str]:
    """Køyr kjeldekode via nc-vm. Returner (ok, error_message)."""
    nc_vm = _nc_vm()
    if not os.path.isfile(nc_vm):
        return False, "nc-vm ikkje funne"
    with tempfile.NamedTemporaryFile(suffix=".no", mode="w", delete=False) as f:
        f.write(source)
        tmp = f.name
    try:
        result = subprocess.run([nc_vm, "--nc-run", tmp], capture_output=True, text=True, timeout=10)
        if expect_failure:
            failed = result.returncode != 0
            error = (result.stderr or result.stdout or "").strip()
            return failed, error
        else:
            return result.returncode == 0, result.stderr.strip()
    except Exception as exc:
        return True, str(exc)
    finally:
        os.unlink(tmp)


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
        ok, error = _run_source_via_ncvm(case["source"], expect_failure=True)
        duration_ms = int((time.perf_counter() - started) * 1000)
        payload["parser_cases"].append({
            "name": case["name"],
            "ok": ok,
            "duration_ms": duration_ms,
            "error": error,
        })
        if not ok:
            payload["parser_failures"] += 1

    runtime_source = 'funksjon start() -> heltall { kast("boom") returner 0 }'
    started = time.perf_counter()
    ok, error = _run_source_via_ncvm(runtime_source, expect_failure=True)
    duration_ms = int((time.perf_counter() - started) * 1000)
    payload["runtime_cases"].append({
        "name": "unhandled-throw",
        "ok": ok,
        "duration_ms": duration_ms,
        "error": error,
    })
    if not ok:
        payload["runtime_failures"] += 1

    payload["ok"] = payload["parser_failures"] == 0 and payload["runtime_failures"] == 0
    return payload


def run_fuzz_suite() -> dict:
    return run_negative_suite()
