"""Serve a Norscode app using the legacy server runtime."""

from __future__ import annotations

import importlib

from norcode.commands.base import CommandModule


def register_arguments(parser) -> None:
    parser.add_argument("file", help="Kildefil å kjøre som webapp")
    parser.add_argument("--host", default="127.0.0.1", help="Bind-adresse for serveren")
    parser.add_argument("--port", type=int, default=8000, help="Port for serveren")
    parser.add_argument("--reload", action="store_true", help="Rekompiler når kildefilen endrer seg")
    parser.add_argument("--once", action="store_true", help="Stopp etter første request (nyttig for smoke-test)")
    parser.add_argument("--production", action="store_true", help="Kjør i produksjonsmodus med signalstyrt shutdown")
    parser.add_argument("--keep-alive", action="store_true", help="Bruk HTTP/1.1 og hold forbindelsen åpen når mulig")
    parser.add_argument("--request-timeout", type=float, help="Timeout i sekunder for en enkelt request/connection")
    parser.add_argument("--proxy-headers", action="store_true", help="Tolk og normaliser forwarded headers fra en reverse proxy")
    parser.add_argument("--trusted-proxy", action="append", default=[], help="Kjente proxy-IP-er som får lov til å sende forwarded headers")
    parser.add_argument("--restart-on-crash", action="store_true", help="Restart serveren hvis den krasjer")
    parser.add_argument("--max-restarts", type=int, default=0, help="Maks antall auto-restarts ved krasj")
    parser.add_argument("--restart-delay", type=float, default=1.0, help="Forsinkelse i sekunder før restart")
    parser.add_argument("--no-cors", action="store_true", help="Skru av standard CORS-hoder")
    parser.add_argument("--cors-origin", action="append", default=[], help="Tillatt origin for CORS (kan gjentas)")
    parser.add_argument("--cors-allow-methods", default=None, help="Komma-separert liste over CORS-metoder")
    parser.add_argument("--cors-allow-headers", default=None, help="Komma-separert liste over CORS request-headers")
    parser.add_argument("--cors-expose-headers", default=None, help="Komma-separert liste over CORS response-headers")
    parser.add_argument("--cors-allow-credentials", action="store_true", help="Tillat credentials i CORS-svar")
    parser.add_argument("--cors-max-age", type=int, default=600, help="Max-Age for CORS preflight i sekunder")
    parser.add_argument("--no-rate-limit", action="store_true", help="Skru av standard rate limiting")
    parser.add_argument("--rate-limit-requests", type=int, default=120, help="Antall forespørsler per vindu før 429")
    parser.add_argument("--rate-limit-window", type=int, default=60, help="Vindustid i sekunder for rate limiting")
    parser.add_argument("--rate-limit-burst", type=int, default=30, help="Maks burst før refill begynner")
    parser.add_argument("--health-path", default="/healthz", help="Sti for health-endepunkt")
    parser.add_argument("--ready-path", default="/readyz", help="Sti for readiness-endepunkt")
    parser.add_argument("--live-path", default="/livez", help="Sti for liveness-endepunkt")


def run(args) -> int:
    legacy_main = importlib.import_module("main")
    return legacy_main.serve_program(
        args.file,
        host=args.host,
        port=args.port,
        reload_enabled=args.reload,
        once=args.once,
        production=args.production,
        keep_alive=args.keep_alive,
        request_timeout_seconds=args.request_timeout,
        proxy_headers=args.proxy_headers,
        trusted_proxies=set(args.trusted_proxy or []),
        restart_on_crash=args.restart_on_crash,
        max_restarts=args.max_restarts,
        restart_delay_seconds=args.restart_delay,
        cors_enabled=not args.no_cors,
        cors_origins=args.cors_origin,
        cors_allow_methods=args.cors_allow_methods,
        cors_allow_headers=args.cors_allow_headers,
        cors_expose_headers=args.cors_expose_headers,
        cors_allow_credentials=args.cors_allow_credentials,
        cors_max_age_seconds=args.cors_max_age,
        rate_limit_enabled=not args.no_rate_limit,
        rate_limit_requests=args.rate_limit_requests,
        rate_limit_window_seconds=args.rate_limit_window,
        rate_limit_burst=args.rate_limit_burst,
        health_path=args.health_path,
        readiness_path=args.ready_path,
        liveness_path=args.live_path,
    )


SERVE_COMMAND = CommandModule(
    name="serve",
    help="Start en lokal webserver for en Norscode-app",
    register_arguments=register_arguments,
    run=run,
)
