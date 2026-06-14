#!/usr/bin/env python3
"""
nc_http_server.py — Async HTTP-server for Norscode-appar

Brukar asyncio + ThreadPoolExecutor for ekte concurrent I/O:
  - Event loop handterer TCP-tilkoplingar utan å blokkere
  - Kvar Norscode-kall køyrer i eigen tråd (CPU-bound isolert)
  - Graceful shutdown via SIGTERM/SIGINT

Bruk:
    python3 tools/nc_http_server.py <kilde.no> [--port 4173] [--workers 8]

Eller via nc:
    nc serve <kilde.no> [--port 4173]
"""

import asyncio
import json
import os
import signal
import subprocess
import sys
import time
from concurrent.futures import ThreadPoolExecutor
from urllib.parse import urlparse

ROOT_DIR = os.path.dirname(os.path.dirname(os.path.abspath(__file__)))
NC_NATIVE = os.path.join(ROOT_DIR, "dist", "norscode_native")


def find_nc_native():
    if os.path.isfile(NC_NATIVE) and os.access(NC_NATIVE, os.X_OK):
        return NC_NATIVE
    import shutil
    return shutil.which("norscode_native")


# ─── Norscode-kall (blokkerer — køyrer i tråd) ────────────────────────────────

def call_norscode(src_file, method, path, headers, body, query_string=""):
    nc_bin = find_nc_native()
    if not nc_bin:
        return 500, {"content-type": "text/plain"}, b"norscode_native ikkje funne"

    full_path = f"{path}?{query_string}" if query_string else path
    header_lines = "\r\n".join(f"{k}: {v}" for k, v in headers.items())
    raw_request = f"{method} {full_path} HTTP/1.1\r\n{header_lines}\r\n\r\n{body}"

    env = os.environ.copy()
    env["NORSCODE_CMD"]               = "serve"
    env["NORSCODE_FILE"]              = os.path.abspath(src_file)
    env["NORSCODE_FAKE_HTTP_REQUEST"] = raw_request

    try:
        result = subprocess.run(
            [nc_bin], env=env, capture_output=True, text=True, timeout=30
        )
        return parse_http_response(result.stdout.strip())
    except subprocess.TimeoutExpired:
        return 504, {"content-type": "application/json"}, b'{"error":"timeout"}'
    except Exception as e:
        return 500, {"content-type": "application/json"}, json.dumps({"error": str(e)}).encode()


def parse_http_response(raw):
    if not raw:
        return 200, {"content-type": "text/plain"}, b""

    try:
        data = json.loads(raw)
        if isinstance(data, dict) and "__status__" in data:
            status = int(data.get("__status__", 200))
            hdrs   = data.get("__headers__", {})
            body   = data.get("__body__", "")
            return status, hdrs, body.encode("utf-8")
    except (json.JSONDecodeError, TypeError, ValueError):
        pass

    if raw.startswith("HTTP/"):
        lines  = raw.split("\r\n")
        parts  = lines[0].split(" ", 2)
        status = int(parts[1]) if len(parts) > 1 else 200
        hdrs, body_start = {}, 1
        for i, line in enumerate(lines[1:], 1):
            if line == "":
                body_start = i + 1
                break
            if ":" in line:
                k, v = line.split(":", 1)
                hdrs[k.strip().lower()] = v.strip()
        return status, hdrs, "\r\n".join(lines[body_start:]).encode("utf-8")

    return 200, {"content-type": "text/plain"}, raw.encode("utf-8")


# ─── Async HTTP-protokoll-handterar ───────────────────────────────────────────

STATUS_TEXT = {
    200: "OK", 201: "Created", 204: "No Content",
    301: "Moved Permanently", 302: "Found", 304: "Not Modified",
    400: "Bad Request", 401: "Unauthorized", 403: "Forbidden",
    404: "Not Found", 405: "Method Not Allowed",
    422: "Unprocessable Entity", 429: "Too Many Requests",
    500: "Internal Server Error", 502: "Bad Gateway",
    503: "Service Unavailable", 504: "Gateway Timeout",
}


class AsyncNorscodeHandler(asyncio.Protocol):
    """
    asyncio.Protocol med run_in_executor for Norscode-kall.
    Kvar innkommande tilkopling får eigen instans.
    """

    def __init__(self, src_file, executor, access_log=True):
        self.src_file   = src_file
        self.executor   = executor
        self.access_log = access_log
        self._buf       = b""
        self._transport = None
        self._peer      = ("?", 0)

    def connection_made(self, transport):
        self._transport = transport
        self._peer      = transport.get_extra_info("peername", ("?", 0))

    def data_received(self, data):
        self._buf += data
        if b"\r\n\r\n" not in self._buf:
            return
        asyncio.ensure_future(self._process())

    async def _process(self):
        buf      = self._buf
        self._buf = b""
        try:
            status, hdrs, body = await self._dispatch(buf)
        except Exception as e:
            status = 500
            hdrs   = {"content-type": "application/json"}
            body   = json.dumps({"error": str(e)}).encode()
        self._write_response(status, hdrs, body)

    async def _dispatch(self, raw):
        sep          = raw.index(b"\r\n\r\n")
        header_part  = raw[:sep].decode("utf-8", errors="replace")
        body_bytes   = raw[sep + 4:]
        lines        = header_part.split("\r\n")
        req_parts    = lines[0].split(" ")
        method       = req_parts[0] if req_parts else "GET"
        full_path    = req_parts[1] if len(req_parts) > 1 else "/"
        parsed       = urlparse(full_path)
        path         = parsed.path
        query_str    = parsed.query

        req_headers = {}
        for line in lines[1:]:
            if ":" in line:
                k, v = line.split(":", 1)
                req_headers[k.strip().lower()] = v.strip()

        content_len = int(req_headers.get("content-length", 0))
        body_str    = body_bytes[:content_len].decode("utf-8", errors="replace")

        loop = asyncio.get_event_loop()
        t0   = time.monotonic()
        result = await loop.run_in_executor(
            self.executor, call_norscode,
            self.src_file, method, path, req_headers, body_str, query_str
        )
        ms = int((time.monotonic() - t0) * 1000)

        if self.access_log:
            print(f"  {self._peer[0]} — {method} {path} → {result[0]} ({ms}ms)")
        return result

    def _write_response(self, status, hdrs, body):
        if self._transport is None or self._transport.is_closing():
            return
        phrase = STATUS_TEXT.get(status, "OK")
        merged = {"content-length": str(len(body)), "connection": "close"}
        merged.update({k.lower(): v for k, v in hdrs.items()})
        lines  = [f"HTTP/1.1 {status} {phrase}"]
        lines += [f"{k}: {v}" for k, v in merged.items()]
        lines += ["", ""]
        self._transport.write("\r\n".join(lines).encode("utf-8"))
        self._transport.write(body)
        self._transport.close()

    def connection_lost(self, exc):
        pass

    def eof_received(self):
        return False


# ─── Oppstart ─────────────────────────────────────────────────────────────────

def verify_app(src_file, nc_bin):
    env = os.environ.copy()
    env["NORSCODE_CMD"]    = "compile"
    env["NORSCODE_FILE"]   = os.path.abspath(src_file)
    env["NORSCODE_OUTPUT"] = "/dev/null"
    r = subprocess.run([nc_bin], env=env, capture_output=True, text=True, timeout=15)
    if r.returncode != 0:
        print(r.stderr or r.stdout, file=sys.stderr)
        return False
    return True


async def _serve(src_file, host, port, workers, access_log):
    executor = ThreadPoolExecutor(max_workers=workers, thread_name_prefix="nc-worker")
    loop     = asyncio.get_event_loop()

    def make_handler():
        return AsyncNorscodeHandler(src_file, executor, access_log)

    server = await loop.create_server(make_handler, host, port, reuse_port=True)

    print(f"\n  Norscode async HTTP-server")
    print(f"  App:      {src_file}")
    print(f"  URL:      http://localhost:{port}/")
    print(f"  Workers:  {workers} trådar")
    print(f"  Trykk Ctrl+C for å stoppe\n")

    stop = asyncio.Event()

    def _shutdown():
        print("\nGraceful shutdown…")
        stop.set()

    for sig in (signal.SIGTERM, signal.SIGINT):
        try:
            loop.add_signal_handler(sig, _shutdown)
        except NotImplementedError:
            pass  # Windows

    async with server:
        await stop.wait()

    executor.shutdown(wait=True)
    print("Server stoppa.")


def run_server(src_file, port=4173, host="0.0.0.0", workers=8, access_log=True):
    nc_bin = find_nc_native()
    if not nc_bin:
        print("FEIL: norscode_native ikkje funne.", file=sys.stderr)
        print("Køyr: bash tools/build_norscode_native.sh", file=sys.stderr)
        sys.exit(1)

    if not os.path.isfile(src_file):
        print(f"FEIL: finn ikkje fil: {src_file}", file=sys.stderr)
        sys.exit(1)

    print(f"Sjekker {src_file}...", end=" ", flush=True)
    if not verify_app(src_file, nc_bin):
        sys.exit(1)
    print("OK")

    asyncio.run(_serve(src_file, host, port, workers, access_log))


# ─── CLI ──────────────────────────────────────────────────────────────────────

if __name__ == "__main__":
    import argparse
    parser = argparse.ArgumentParser(description="Norscode async HTTP-server")
    parser.add_argument("src",                 help="Norscode-kjeldefil (.no)")
    parser.add_argument("--port",    type=int, default=int(os.environ.get("NORSCODE_PORT", "4173")))
    parser.add_argument("--host",              default="0.0.0.0")
    parser.add_argument("--workers", type=int, default=int(os.environ.get("NORSCODE_WORKERS", "8")))
    parser.add_argument("--no-log",  action="store_true")
    args = parser.parse_args()
    run_server(args.src, port=args.port, host=args.host,
               workers=args.workers, access_log=not args.no_log)
