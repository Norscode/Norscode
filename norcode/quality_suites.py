# AVVIKLA: Behalde for --legacy-python-fallback. Erstatta av nc-vm.
from __future__ import annotations

import signal
import subprocess
import sys
import threading
import time
import urllib.error
import urllib.request
from concurrent.futures import ThreadPoolExecutor, as_completed
from pathlib import Path


def _repo_root() -> Path:
    return Path(__file__).resolve().parents[1]


def _run_checked_command(args: list[str], cwd: Path | None = None) -> subprocess.CompletedProcess[str]:
    return subprocess.run(
        args,
        cwd=str(cwd or _repo_root()),
        check=True,
        text=True,
        capture_output=True,
    )


def _run_norcode(args: list[str], cwd: Path | None = None) -> subprocess.CompletedProcess[str]:
    return _run_checked_command([sys.executable, "-m", "norcode", *args], cwd=cwd)


def run_benchmark_suite() -> dict:
    root = _repo_root()
    benchmarks = [
        {"name": "check-map", "kind": "check", "file": "tests/test_map.no", "budget_ms": 4000},
        {"name": "test-json", "kind": "test", "file": "tests/test_json.no", "budget_ms": 4000},
        {"name": "test-selfhost", "kind": "test", "file": "tests/test_selfhost.no", "budget_ms": 12000},
        {"name": "commands-json", "kind": "cli", "args": ["commands", "--json"], "budget_ms": 2000},
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
            completed = _run_norcode(["check", item["file"]], cwd=root)
            case_ok = completed.returncode == 0
            details = {"cmd": f"norcode check {item['file']}", "stdout_lines": len(completed.stdout.splitlines())}
        elif item["kind"] == "test":
            completed = _run_norcode(["test", item["file"]], cwd=root)
            case_ok = completed.returncode == 0
            details = {"cmd": f"norcode test {item['file']}", "stdout_lines": len(completed.stdout.splitlines())}
        else:
            completed = _run_norcode(item["args"], cwd=root)
            case_ok = completed.returncode == 0
            details = {"cmd": " ".join(item["args"]), "stdout_lines": len(completed.stdout.splitlines())}
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


def _read_http_response_from_url(url: str, headers: dict[str, str] | None = None) -> tuple[int, dict[str, str], str]:
    request = urllib.request.Request(url, headers=headers or {}, method="GET")
    try:
        with urllib.request.urlopen(request, timeout=10) as response:
            status = int(getattr(response, "status", response.getcode()))
            response_headers = {str(key).lower(): str(value) for key, value in response.headers.items()}
            body = response.read().decode("utf-8", errors="replace")
            return status, response_headers, body
    except urllib.error.HTTPError as exc:
        response_headers = {str(key).lower(): str(value) for key, value in exc.headers.items()}
        body = exc.read().decode("utf-8", errors="replace")
        return int(exc.code), response_headers, body


def _serve_e2e_case(
    source_file: str,
    serve_args: list[str],
    request_path: str,
    expected_status: int = 200,
    expected_body_contains: str | None = None,
    request_headers: dict[str, str] | None = None,
) -> dict:
    root = _repo_root()
    cmd = [sys.executable, "-u", str(root / "main.py"), "serve", source_file, *serve_args]
    proc = subprocess.Popen(
        cmd,
        cwd=str(root),
        stdout=subprocess.PIPE,
        stderr=subprocess.STDOUT,
        text=True,
        bufsize=1,
    )
    captured_lines: list[str] = []
    bind_url: str | None = None
    started = time.perf_counter()
    try:
        assert proc.stdout is not None
        while True:
            if proc.poll() is not None:
                remainder = proc.stdout.read() or ""
                if remainder:
                    captured_lines.extend(line.rstrip("\n") for line in remainder.splitlines())
                raise RuntimeError("Serveren avsluttet før oppstart:\n" + "\n".join(captured_lines))
            line = proc.stdout.readline()
            if line:
                stripped = line.rstrip("\n")
                captured_lines.append(stripped)
                if stripped.startswith("Lytter på "):
                    bind_url = stripped.split("Lytter på ", 1)[1].strip()
                    break
            if time.perf_counter() - started > 15:
                raise RuntimeError("Timeout mens serveren startet")
        if not bind_url:
            raise RuntimeError("Fant ikke start-URL for serveren")
        request_url = bind_url.rstrip("/") + request_path
        status, response_headers, body = _read_http_response_from_url(request_url, headers=request_headers)
        if status != expected_status:
            raise RuntimeError(f"Forventet HTTP {expected_status}, fikk {status}")
        if expected_body_contains is not None and expected_body_contains not in body:
            raise RuntimeError(f"Svarbody manglet forventet tekst {expected_body_contains!r}: {body!r}")
        try:
            proc.wait(timeout=10)
        except subprocess.TimeoutExpired as exc:
            proc.terminate()
            try:
                proc.wait(timeout=5)
            except subprocess.TimeoutExpired:
                proc.kill()
            raise RuntimeError("Serveren avsluttet ikke etter e2e-kjøring") from exc
        if proc.returncode not in (0, None):
            raise RuntimeError(f"Serveren returnerte kode {proc.returncode}")
        return {
            "source": source_file,
            "serve_args": serve_args,
            "request_path": request_path,
            "status": status,
            "body": body,
            "headers": response_headers,
        }
    finally:
        if proc.poll() is None:
            proc.kill()


def run_server_e2e_suite() -> dict:
    payload = {"ok": False, "cases": []}
    cases = [
        ("dev-route", "examples/web_routes.no", ["--host", "127.0.0.1", "--port", "8127", "--once"], "/brukere/42", 200, "42", None),
        ("production-health", "examples/web_routes.no", ["--host", "127.0.0.1", "--port", "8128", "--production", "--once"], "/healthz", 200, '"mode": "production"', None),
        ("proxy-route", "tests/test_web_proxy.no", ["--host", "127.0.0.1", "--port", "8129", "--proxy-headers", "--trusted-proxy", "127.0.0.1", "--once"], "/proxy", 200, "https|api.example.com|203.0.113.10|https://api.example.com/proxy", {"x-forwarded-proto": "https", "x-forwarded-host": "api.example.com", "x-forwarded-for": "203.0.113.10"}),
    ]
    try:
        for name, source, serve_args, request_path, expected_status, expected_body_contains, request_headers in cases:
            started = time.perf_counter()
            result = _serve_e2e_case(
                source,
                serve_args,
                request_path,
                expected_status=expected_status,
                expected_body_contains=expected_body_contains,
                request_headers=request_headers,
            )
            payload["cases"].append(
                {
                    "name": name,
                    "ok": True,
                    "duration_ms": int((time.perf_counter() - started) * 1000),
                    "request_path": request_path,
                    "source": source,
                    "status": result["status"],
                }
            )
        payload["ok"] = True
        return payload
    except Exception as exc:
        payload["cases"].append({"name": name if "name" in locals() else "unknown", "ok": False, "error": str(exc)})
        raise RuntimeError("Serveradapter-e2e feilet") from exc


def _start_server_for_http_case(source_file: str, serve_args: list[str]) -> tuple[subprocess.Popen[str], str]:
    root = _repo_root()
    cmd = [sys.executable, "-u", str(root / "main.py"), "serve", source_file, *serve_args]
    proc = subprocess.Popen(
        cmd,
        cwd=str(root),
        stdout=subprocess.PIPE,
        stderr=subprocess.STDOUT,
        text=True,
        bufsize=1,
    )
    captured_lines: list[str] = []
    bind_url: str | None = None
    started = time.perf_counter()
    assert proc.stdout is not None
    while True:
        if proc.poll() is not None:
            remainder = proc.stdout.read() or ""
            if remainder:
                captured_lines.extend(line.rstrip("\n") for line in remainder.splitlines())
            raise RuntimeError("Serveren avsluttet før oppstart:\n" + "\n".join(captured_lines))
        line = proc.stdout.readline()
        if line:
            stripped = line.rstrip("\n")
            captured_lines.append(stripped)
            if stripped.startswith("Lytter på "):
                bind_url = stripped.split("Lytter på ", 1)[1].strip()
                break
        if time.perf_counter() - started > 15:
            raise RuntimeError("Timeout mens serveren startet")
    if not bind_url:
        raise RuntimeError("Fant ikke start-URL for serveren")
    return proc, bind_url


def _run_concurrent_http_gets(
    base_url: str,
    request_path: str,
    total_requests: int,
    concurrency: int,
    expected_status: int,
    expected_body_contains: str | None = None,
    request_headers: dict[str, str] | None = None,
) -> dict:
    url = base_url.rstrip("/") + request_path
    headers = request_headers or {}
    ok_count = 0
    errors: list[str] = []
    statuses: dict[int, int] = {}

    def _one_request() -> tuple[int, str]:
        status, _response_headers, body = _read_http_response_from_url(url, headers=headers)
        return status, body

    with ThreadPoolExecutor(max_workers=max(1, concurrency)) as pool:
        futures = [pool.submit(_one_request) for _ in range(max(1, total_requests))]
        for future in as_completed(futures):
            try:
                status, body = future.result()
            except Exception as exc:
                errors.append(str(exc))
                continue
            statuses[status] = statuses.get(status, 0) + 1
            if status == expected_status and (expected_body_contains is None or expected_body_contains in body):
                ok_count += 1
            else:
                errors.append(f"status={status}, body={body!r}")

    return {
        "ok": ok_count == total_requests and not errors,
        "url": url,
        "requests": total_requests,
        "concurrency": concurrency,
        "ok_count": ok_count,
        "status_counts": statuses,
        "errors": errors[:5],
    }


def run_stress_suite() -> dict:
    payload = {"ok": False, "cases": []}
    cases = [
        {
            "name": "production-route-load",
            "source": "examples/web_routes.no",
            "serve_args": ["--host", "127.0.0.1", "--port", "8130", "--production", "--keep-alive", "--request-timeout", "5", "--no-rate-limit"],
            "request_path": "/brukere/42",
            "expected_status": 200,
            "expected_body_contains": "42",
            "request_headers": None,
            "requests": 60,
            "concurrency": 12,
        },
        {
            "name": "production-health-load",
            "source": "examples/web_routes.no",
            "serve_args": ["--host", "127.0.0.1", "--port", "8131", "--production", "--keep-alive", "--request-timeout", "5", "--no-rate-limit"],
            "request_path": "/healthz",
            "expected_status": 200,
            "expected_body_contains": '"status": "ok"',
            "request_headers": None,
            "requests": 40,
            "concurrency": 10,
        },
    ]
    try:
        for case in cases:
            started = time.perf_counter()
            proc, bind_url = _start_server_for_http_case(case["source"], case["serve_args"])
            try:
                result = _run_concurrent_http_gets(
                    bind_url,
                    case["request_path"],
                    total_requests=case["requests"],
                    concurrency=case["concurrency"],
                    expected_status=case["expected_status"],
                    expected_body_contains=case["expected_body_contains"],
                    request_headers=case["request_headers"],
                )
            finally:
                if proc.poll() is None:
                    proc.send_signal(signal.SIGINT)
                    try:
                        proc.wait(timeout=10)
                    except subprocess.TimeoutExpired:
                        proc.kill()
                        proc.wait(timeout=5)
            case_payload = {
                "name": case["name"],
                "ok": bool(result["ok"]),
                "duration_ms": int((time.perf_counter() - started) * 1000),
                "requests": case["requests"],
                "concurrency": case["concurrency"],
                "status_counts": result["status_counts"],
                "ok_count": result["ok_count"],
                "errors": result["errors"],
            }
            payload["cases"].append(case_payload)
            if not case_payload["ok"]:
                raise RuntimeError(f"Stress-feil i {case['name']}")
        payload["ok"] = True
        return payload
    except Exception as exc:
        payload["cases"].append({"name": case["name"] if "case" in locals() else "unknown", "ok": False, "error": str(exc)})
        raise RuntimeError("Produksjonsnær stresstest feilet") from exc


def run_security_suite() -> dict:
    checks = [
        ("auth", "tests/test_web_auth.no"),
        ("csrf", "tests/test_csrf.no"),
        ("input-sanitize", "tests/test_web_sanitize.no"),
        ("cookies", "tests/test_web_cookies.no"),
        ("guards", "tests/test_web_guard.no"),
    ]
    payload = {"ok": False, "checks": [], "total": len(checks), "passed": 0, "failed": 0}
    for name, file in checks:
        started = time.perf_counter()
        completed = _run_norcode(["test", file])
        ok = completed.returncode == 0
        payload["checks"].append(
            {
                "name": name,
                "file": file,
                "ok": ok,
                "duration_ms": int((time.perf_counter() - started) * 1000),
            }
        )
        if ok:
            payload["passed"] += 1
        else:
            payload["failed"] += 1
    payload["ok"] = payload["failed"] == 0
    return payload
