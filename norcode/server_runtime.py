"""Shared server/runtime helpers for `serve` and fallback execution."""

from __future__ import annotations

import json
import signal
import sys
import threading
import time
import urllib.parse
from http.server import BaseHTTPRequestHandler, ThreadingHTTPServer
from pathlib import Path

from compiler.bytecode_backend import BytecodeVM, compile_source_to_bytecode


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


def _lowercase_text_map(values: dict[str, str]) -> dict[str, str]:
    return {str(key).lower(): str(value) for key, value in values.items()}


def _parse_forwarded_header(value: str) -> dict[str, str]:
    result: dict[str, str] = {}
    for part in value.split(";"):
        if "=" not in part:
            continue
        key, raw_value = part.split("=", 1)
        key = key.strip().lower()
        raw_value = raw_value.strip().strip('"')
        if key:
            result[key] = raw_value
    return result


def _proxy_trusted(client_ip: str, trusted_proxies: set[str]) -> bool:
    if not trusted_proxies:
        return True
    return client_ip in trusted_proxies


def _proxy_enrich_headers(
    raw_headers: dict[str, str],
    client_ip: str,
    path: str,
    proxy_headers: bool,
    trusted_proxies: set[str],
) -> dict[str, str]:
    headers = _lowercase_text_map(raw_headers)
    headers["x-client-ip"] = client_ip
    if not proxy_headers or not _proxy_trusted(client_ip, trusted_proxies):
        return headers

    forwarded = _parse_forwarded_header(headers.get("forwarded", ""))
    proto = forwarded.get("proto") or headers.get("x-forwarded-proto", "")
    host = forwarded.get("host") or headers.get("x-forwarded-host", headers.get("host", ""))
    forwarded_for = forwarded.get("for") or headers.get("x-forwarded-for", client_ip)
    prefix = headers.get("x-forwarded-prefix", "")
    real_ip = headers.get("x-real-ip", forwarded_for or client_ip)

    if proto:
        headers["x-forwarded-proto"] = proto
    if host:
        headers["x-forwarded-host"] = host
    if forwarded_for:
        headers["x-forwarded-for"] = forwarded_for
    if prefix:
        headers["x-forwarded-prefix"] = prefix

    headers["x-real-ip"] = real_ip
    headers["x-external-scheme"] = proto or "http"
    headers["x-external-host"] = host or ""
    headers["x-external-prefix"] = prefix or ""
    if proto and host:
        headers["x-external-url"] = f"{proto}://{host}{prefix}{path}"
    else:
        headers["x-external-url"] = ""
    return headers


def _split_csv_text(value: str | None, *, upper: bool = False, lower: bool = False) -> list[str]:
    if value is None:
        return []
    items: list[str] = []
    for part in str(value).split(","):
        item = part.strip()
        if not item:
            continue
        if upper:
            item = item.upper()
        elif lower:
            item = item.lower()
        items.append(item)
    return items


def _build_cors_policy(
    enabled: bool,
    origins: list[str] | None,
    allow_methods: str | None,
    allow_headers: str | None,
    expose_headers: str | None,
    allow_credentials: bool,
    max_age_seconds: int | None,
) -> dict[str, object]:
    if not enabled:
        return {"enabled": False}
    origin_list = [str(origin).strip() for origin in (origins or []) if str(origin).strip()]
    if not origin_list:
        origin_list = ["*"]
    if allow_credentials and "*" in origin_list:
        raise RuntimeError("CORS med credentials krever eksplisitte origins, ikke '*'")
    return {
        "enabled": True,
        "origins": origin_list,
        "allow_methods": _split_csv_text(allow_methods, upper=True) or ["GET", "POST", "PUT", "PATCH", "DELETE", "HEAD", "OPTIONS"],
        "allow_headers": _split_csv_text(allow_headers, lower=True) or ["content-type", "authorization", "x-requested-with", "x-csrf-token"],
        "expose_headers": _split_csv_text(expose_headers, lower=True),
        "allow_credentials": bool(allow_credentials),
        "max_age": int(max_age_seconds) if max_age_seconds is not None else 600,
    }


def _cors_allowed_origin(request_origin: str, policy: dict[str, object]) -> str:
    if not policy.get("enabled"):
        return ""
    request_origin = request_origin.strip()
    if not request_origin:
        return ""
    origins = [str(origin) for origin in policy.get("origins", [])]
    if "*" in origins:
        return "*"
    if request_origin in origins:
        return request_origin
    return ""


def _apply_cors_headers(
    response_headers: dict[str, str],
    request_headers: dict[str, str],
    policy: dict[str, object],
    *,
    preflight: bool = False,
) -> None:
    if not policy.get("enabled"):
        return
    request_origin = str(request_headers.get("origin", "")).strip()
    allowed_origin = _cors_allowed_origin(request_origin, policy)
    if not allowed_origin:
        return
    response_headers["access-control-allow-origin"] = allowed_origin
    if policy.get("allow_credentials"):
        response_headers["access-control-allow-credentials"] = "true"
    if request_origin:
        response_headers["vary"] = "Origin"
    if preflight:
        allow_methods = [str(item) for item in policy.get("allow_methods", []) if str(item).strip()]
        allow_headers = [str(item) for item in policy.get("allow_headers", []) if str(item).strip()]
        if allow_methods:
            response_headers["access-control-allow-methods"] = ", ".join(allow_methods)
        if allow_headers:
            response_headers["access-control-allow-headers"] = ", ".join(allow_headers)
        response_headers["access-control-max-age"] = str(policy.get("max_age", 600))
    else:
        expose_headers = [str(item) for item in policy.get("expose_headers", []) if str(item).strip()]
        if expose_headers:
            response_headers["access-control-expose-headers"] = ", ".join(expose_headers)


def _build_rate_limit_policy(
    enabled: bool,
    requests_per_window: int,
    window_seconds: int,
    burst: int | None,
) -> dict[str, object]:
    if not enabled:
        return {"enabled": False}
    limit = max(1, int(requests_per_window))
    window = max(1, int(window_seconds))
    capacity = max(1, int(burst if burst is not None else limit))
    return {
        "enabled": True,
        "requests_per_window": limit,
        "window_seconds": window,
        "burst": capacity,
        "tokens_per_second": limit / float(window),
    }


def _rate_limit_identity(headers: dict[str, str]) -> str:
    client_ip = str(headers.get("x-client-ip", "")).strip()
    if client_ip:
        return client_ip
    real_ip = str(headers.get("x-real-ip", "")).strip()
    if real_ip:
        return real_ip
    forwarded = str(headers.get("x-forwarded-for", "")).strip()
    if forwarded:
        return forwarded.split(",", 1)[0].strip()
    return "unknown"


def _rate_limit_check(
    state: dict[str, dict[str, float]],
    lock: threading.Lock,
    policy: dict[str, object],
    identity: str,
    now: float,
) -> tuple[bool, dict[str, str]]:
    if not policy.get("enabled"):
        return True, {}
    key = identity or "unknown"
    capacity = float(policy.get("burst", 1))
    refill = float(policy.get("tokens_per_second", 1.0))
    window_seconds = int(policy.get("window_seconds", 60))
    with lock:
        bucket = state.get(key)
        if bucket is None:
            bucket = {"tokens": capacity, "updated": now}
            state[key] = bucket
        else:
            elapsed = max(0.0, now - float(bucket.get("updated", now)))
            bucket["tokens"] = min(capacity, float(bucket.get("tokens", capacity)) + elapsed * refill)
            bucket["updated"] = now
        tokens = float(bucket.get("tokens", capacity))
        if tokens < 1.0:
            retry_after = max(1, int((1.0 - tokens) / refill)) if refill > 0 else window_seconds
            headers = {
                "retry-after": str(retry_after),
                "x-rate-limit-limit": str(int(policy.get("requests_per_window", 0))),
                "x-rate-limit-remaining": "0",
                "x-rate-limit-reset": str(int(now + retry_after)),
            }
            return False, headers
        bucket["tokens"] = tokens - 1.0
        remaining = max(0, int(bucket["tokens"]))
        headers = {
            "x-rate-limit-limit": str(int(policy.get("requests_per_window", 0))),
            "x-rate-limit-remaining": str(remaining),
            "x-rate-limit-reset": str(int(now + window_seconds)),
        }
        return True, headers


def _normalize_serve_response(response: object) -> tuple[int, dict[str, str], bytes]:
    if isinstance(response, tuple):
        if len(response) == 3:
            status, headers, body = response
        elif len(response) == 2:
            status, body = response
            headers = {}
        else:
            raise RuntimeError("Ugyldig serve-respons")
    else:
        status, headers, body = 200, {}, response
    if isinstance(headers, list):
        headers = {str(k): str(v) for k, v in headers}
    elif not isinstance(headers, dict):
        headers = {}
    if body is None:
        body_bytes = b""
    elif isinstance(body, bytes):
        body_bytes = body
    else:
        body_bytes = str(body).encode("utf-8")
    return int(status), {str(k): str(v) for k, v in headers.items()}, body_bytes


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


def serve_program(
    source_file: str,
    host: str = "127.0.0.1",
    port: int = 8000,
    reload_enabled: bool = False,
    once: bool = False,
    production: bool = False,
    keep_alive: bool = False,
    request_timeout_seconds: float | None = None,
    proxy_headers: bool = False,
    trusted_proxies: set[str] | None = None,
    restart_on_crash: bool = False,
    max_restarts: int = 0,
    restart_delay_seconds: float = 1.0,
    cors_enabled: bool = True,
    cors_origins: list[str] | None = None,
    cors_allow_methods: str | None = None,
    cors_allow_headers: str | None = None,
    cors_expose_headers: str | None = None,
    cors_allow_credentials: bool = False,
    cors_max_age_seconds: int | None = 600,
    rate_limit_enabled: bool = True,
    rate_limit_requests: int = 120,
    rate_limit_window_seconds: int = 60,
    rate_limit_burst: int | None = 30,
    health_path: str = "/healthz",
    readiness_path: str = "/readyz",
    liveness_path: str = "/livez",
):
    cors_policy = _build_cors_policy(
        cors_enabled,
        cors_origins,
        cors_allow_methods,
        cors_allow_headers,
        cors_expose_headers,
        cors_allow_credentials,
        cors_max_age_seconds,
    )
    rate_limit_policy = _build_rate_limit_policy(
        rate_limit_enabled,
        rate_limit_requests,
        rate_limit_window_seconds,
        rate_limit_burst,
    )
    rate_limit_state: dict[str, dict[str, float]] = {}
    rate_limit_lock = threading.Lock()

    def _serve_once():
        try:
            runtime = _ServeRuntime(source_file, reload_enabled=(reload_enabled and not production))
        except Exception as exc:
            raise RuntimeError(f"Oppstartfeil: kunne ikke laste {Path(source_file).expanduser().resolve()}: {exc}") from exc

        class Handler(BaseHTTPRequestHandler):
            protocol_version = "HTTP/1.1" if keep_alive else "HTTP/1.0"

            def setup(self):
                super().setup()
                if request_timeout_seconds is not None:
                    try:
                        timeout = max(0.1, float(request_timeout_seconds))
                    except Exception:
                        timeout = None
                    if timeout is not None:
                        self.connection.settimeout(timeout)

            def _request_headers(self, path: str) -> dict[str, str]:
                client_ip = str(self.client_address[0]) if getattr(self, "client_address", None) else ""
                return _proxy_enrich_headers(
                    {str(key): str(value) for key, value in self.headers.items()},
                    client_ip=client_ip,
                    path=path,
                    proxy_headers=proxy_headers,
                    trusted_proxies=trusted_proxies or set(),
                )

            def _dispatch(self):
                parsed = urllib.parse.urlsplit(self.path)
                query = {str(key): str(value) for key, value in urllib.parse.parse_qsl(parsed.query, keep_blank_values=True)}
                headers = self._request_headers(parsed.path or "/")
                identity = _rate_limit_identity(headers)
                allowed, rate_headers = _rate_limit_check(
                    rate_limit_state,
                    rate_limit_lock,
                    rate_limit_policy,
                    identity,
                    time.monotonic(),
                )
                if not allowed:
                    response_headers = {
                        "content-type": "application/json; charset=utf-8",
                        **rate_headers,
                    }
                    response_body = json.dumps({"error": "For mange forespørsler"}, ensure_ascii=False).encode("utf-8")
                    _apply_cors_headers(response_headers, headers, cors_policy, preflight=self.command == "OPTIONS")
                    self.send_response(429)
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
                    return
                length_text = self.headers.get("Content-Length", "0")
                try:
                    length = max(0, int(length_text))
                except Exception:
                    length = 0
                body_bytes = self.rfile.read(length) if length else b""
                body = body_bytes.decode("utf-8", errors="replace")
                preflight = self.command == "OPTIONS"
                try:
                    response = runtime.handle(self.command, parsed.path or "/", query, headers, body)
                    status, response_headers, response_body = _normalize_serve_response(response)
                except Exception as exc:
                    status = 500
                    response_headers = {"content-type": "application/json; charset=utf-8"}
                    response_body = json.dumps({"error": str(exc)}, ensure_ascii=False).encode("utf-8")
                response_headers.update(rate_headers)
                _apply_cors_headers(response_headers, headers, cors_policy, preflight=preflight)
                self.send_response(status)
                for key, value in response_headers.items():
                    if key.lower() == "content-length":
                        continue
                    if key.lower() == "set-cookie":
                        for cookie_value in str(value).splitlines():
                            cookie_text = cookie_value.strip()
                            if cookie_text:
                                self.send_header(key, cookie_text)
                        continue
                    self.send_header(key, value)
                self.send_header("Content-Length", str(len(response_body)))
                self.end_headers()
                if self.command != "HEAD":
                    self.wfile.write(response_body)
                if once:
                    threading.Thread(target=self.server.shutdown, daemon=True).start()

            def _health_response(self, kind: str):
                headers = self._request_headers(self.path.split("?", 1)[0] or "/")
                if kind == "health":
                    payload = {"status": "ok", "mode": "production" if production else "development"}
                elif kind == "ready":
                    payload = {"ready": True, "mode": "production" if production else "development"}
                else:
                    payload = {"alive": True, "mode": "production" if production else "development"}
                body = json.dumps(payload, ensure_ascii=False).encode("utf-8")
                response_headers = {"content-type": "application/json; charset=utf-8"}
                _apply_cors_headers(response_headers, headers, cors_policy, preflight=False)
                self.send_response(200)
                for key, value in response_headers.items():
                    self.send_header(key, value)
                self.send_header("Content-Length", str(len(body)))
                self.end_headers()
                if self.command != "HEAD":
                    self.wfile.write(body)
                if once:
                    threading.Thread(target=self.server.shutdown, daemon=True).start()

            def do_GET(self):
                if self.path.split("?", 1)[0] == health_path:
                    return self._health_response("health")
                if self.path.split("?", 1)[0] == readiness_path:
                    return self._health_response("ready")
                if self.path.split("?", 1)[0] == liveness_path:
                    return self._health_response("live")
                self._dispatch()

            def do_HEAD(self):
                if self.path.split("?", 1)[0] == health_path:
                    return self._health_response("health")
                if self.path.split("?", 1)[0] == readiness_path:
                    return self._health_response("ready")
                if self.path.split("?", 1)[0] == liveness_path:
                    return self._health_response("live")
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

        try:
            server = _NorscodeThreadingHTTPServer((host, port), Handler)
        except OSError as exc:
            raise RuntimeError(f"Oppstartfeil: kunne ikke binde {host}:{port}: {exc}") from exc
        bind_host, bind_port = server.server_address
        print(f"Starter Norscode server fra {Path(source_file).expanduser().resolve()}")
        print(f"Lytter på http://{bind_host}:{bind_port}")
        print(f"Health: {health_path} ready={readiness_path} live={liveness_path}")
        if production:
            print("Modus: production")
        if keep_alive:
            print("Keep-alive: på")
        if request_timeout_seconds is not None:
            print(f"Timeout: {request_timeout_seconds} s")
        if proxy_headers:
            if trusted_proxies:
                trusted = ", ".join(sorted(trusted_proxies))
                print(f"Proxy headers: på (trusted: {trusted})")
            else:
                print("Proxy headers: på")
        if cors_policy.get("enabled"):
            cors_origins_text = ", ".join(str(origin) for origin in cors_policy.get("origins", []))
            cors_methods_text = ", ".join(str(method) for method in cors_policy.get("allow_methods", []))
            cors_headers_text = ", ".join(str(header) for header in cors_policy.get("allow_headers", []))
            print(f"CORS: på (origins: {cors_origins_text}; methods: {cors_methods_text}; headers: {cors_headers_text})")
        if reload_enabled:
            print("Reload: på")
        stop_lock = threading.Lock()
        shutdown_requested = {"done": False}

        def _request_shutdown(signum=None, frame=None):
            with stop_lock:
                if shutdown_requested["done"]:
                    return
                shutdown_requested["done"] = True
            print("Stopper serveren ...")
            threading.Thread(target=server.shutdown, daemon=True).start()

        previous_sigint = signal.getsignal(signal.SIGINT)
        previous_sigterm = signal.getsignal(signal.SIGTERM) if hasattr(signal, "SIGTERM") else None
        signal.signal(signal.SIGINT, _request_shutdown)
        if hasattr(signal, "SIGTERM"):
            signal.signal(signal.SIGTERM, _request_shutdown)
        try:
            server.serve_forever()
        finally:
            signal.signal(signal.SIGINT, previous_sigint)
            if hasattr(signal, "SIGTERM") and previous_sigterm is not None:
                signal.signal(signal.SIGTERM, previous_sigterm)
            runtime.shutdown()
            server.server_close()

    attempts = 0
    while True:
        try:
            _serve_once()
            return
        except KeyboardInterrupt:
            print()
            return
        except Exception as exc:
            if not restart_on_crash or attempts >= max_restarts:
                raise
            attempts += 1
            print(f"Advarsel: serveren krasjet ({exc}); restart {attempts}/{max_restarts}", file=sys.stderr)
            time.sleep(max(0.0, float(restart_delay_seconds)))
