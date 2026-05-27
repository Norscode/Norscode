"""Bootstrap and runtime command modules."""

from __future__ import annotations

import json
import time

from compiler.native.pipeline import verify_bootstrap_compiler
from compiler.selfhost_chain import run_chain
from compiler.selfhost_whole_compile import DEFAULT_ROOTS, WholeCompileOptions, compile_whole_norscode

from norcode.bootstrap_support import _format_cli_exception
from norcode.commands.base import CommandModule
from norcode.package_registry import registry_host


def register_bootstrap_verify_arguments(parser) -> None:
    parser.add_argument("--json", action="store_true", help="Skriv bootstrap-verifikasjon som JSON")


def run_bootstrap_verify(args) -> int:
    try:
        payload = verify_bootstrap_compiler()
        if args.json:
            print(json.dumps(payload, ensure_ascii=False, indent=2))
        else:
            status = "OK" if payload.get("ok") else "FEIL"
            print(f"Bootstrap compiler verify {status}")
            print(f"- source: {payload['source']}")
            print(f"- output: {payload['output']}")
            print(f"- executable: {'ja' if payload['executable'] else 'nei'}")
            print(f"- ran: {'ja' if payload['ran'] else 'nei'}")
        return 0 if payload.get("ok") else 1
    except Exception as exc:
        print(f"Feil: {_format_cli_exception(exc)}")
        return 1


def register_registry_host_arguments(parser) -> None:
    parser.add_argument("--host", default="127.0.0.1", help="Bind-adresse for registry host")
    parser.add_argument("--port", type=int, default=8765, help="Port for registry host")
    parser.add_argument("--mirror", help="Mirror-fil å skrive/servere (default: build/registry_mirror.json)")
    parser.add_argument("--once", action="store_true", help="Start, hent index én gang og stopp")
    parser.add_argument("--json", action="store_true", help="Skriv resultat som JSON")


def run_registry_host(args) -> int:
    try:
        payload = registry_host(host=args.host, port=args.port, mirror_file=args.mirror, once=args.once)
        if args.once:
            if args.json:
                print(json.dumps(payload, ensure_ascii=False, indent=2))
            else:
                print(f"Registry host { 'OK' if payload.get('ok') else 'FEIL' }")
                print(f"- url: {payload['url']}")
                print(f"- mirror: {payload['mirror']}")
                print(f"- count: {payload['count']}")
                print(f"- once: ja")
            return 0 if payload.get("ok") else 1

        server = payload["server"]
        if args.json:
            print(
                json.dumps(
                    {k: v for k, v in payload.items() if k != "server"},
                    ensure_ascii=False,
                    indent=2,
                )
            )
        else:
            print(f"Registry host aktiv: {payload['url']}")
        try:
            server.serve_forever()
        except KeyboardInterrupt:
            pass
        finally:
            server.shutdown()
            server.server_close()
        return 0
    except Exception as exc:
        print(f"Feil: {_format_cli_exception(exc)}")
        return 1


def register_selfhost_chain_run_arguments(parser) -> None:
    parser.add_argument("file")
    parser.add_argument("--trace", action="store_true", help="Vis sporlogg ved feil")
    parser.add_argument("--max-steps", type=int, default=5000000, help="Maks VM-steg før kjøring avbrytes")
    parser.add_argument("--trace-focus", help="Logg kun funksjoner som matcher denne teksten")
    parser.add_argument("--repeat-limit", type=int, default=0, help="Avbryt hvis samme VM-tilstand gjentas mer enn N ganger")
    parser.add_argument("--expr-probe", help="Logg uttrykkstokens som matcher denne teksten")
    parser.add_argument("--expr-probe-log", help="Skriv uttrykksprobe til fil")


def run_selfhost_chain_run(args) -> int:
    try:
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
        return 0
    except Exception as exc:
        print(f"Feil: {_format_cli_exception(exc)}")
        return 1


def register_selfhost_compile_all_arguments(parser) -> None:
    parser.add_argument("--root", action="append", dest="roots", help="Root å kompilere (kan gjentas, default: selfhost/compiler/std)")
    parser.add_argument("--output-dir", default="build/selfhost-whole", help="Output-katalog for AST, bytecode og manifest")
    parser.add_argument("--fail-fast", action="store_true", help="Stopp ved første compile-feil")
    parser.add_argument("--json", action="store_true", help="Skriv compile-manifest som JSON")


def run_selfhost_compile_all(args) -> int:
    try:
        options = WholeCompileOptions(
            roots=tuple(args.roots) if args.roots else DEFAULT_ROOTS,
            output_dir=args.output_dir,
            fail_fast=args.fail_fast,
        )
        started = time.perf_counter()
        payload = compile_whole_norscode(options)
        payload["duration_ms"] = int((time.perf_counter() - started) * 1000)
        if args.json:
            print(json.dumps(payload, ensure_ascii=False, indent=2))
        else:
            status = "OK" if payload.get("ok") else "FEIL"
            print(f"Selfhost compile-all {status}")
            print(f"- manifest: {payload['manifest']}")
            print(f"- total: {payload['total']}")
            print(f"- passed: {payload['passed']}")
            print(f"- failed: {payload['failed']}")
        return 0 if payload.get("ok") else 1
    except Exception as exc:
        print(f"Feil: {_format_cli_exception(exc)}")
        return 1


BOOTSTRAP_COMPILER_VERIFY_COMMAND = CommandModule(
    name="bootstrap-compiler-verify",
    help="Verifiser real bootstrap compiler native lane",
    register_arguments=register_bootstrap_verify_arguments,
    run=run_bootstrap_verify,
    bootstrap_only=True,
)

REGISTRY_HOST_COMMAND = CommandModule(
    name="registry-host",
    help="Host registry-speilfil over HTTP",
    register_arguments=register_registry_host_arguments,
    run=run_registry_host,
    bootstrap_only=True,
)

SELFHOST_CHAIN_RUN_COMMAND = CommandModule(
    name="selfhost-chain-run",
    help="Kjør full selfhost-kjede",
    register_arguments=register_selfhost_chain_run_arguments,
    run=run_selfhost_chain_run,
    experimental=True,
)

SELFHOST_COMPILE_ALL_COMMAND = CommandModule(
    name="selfhost-compile-all",
    help="Kompiler hele Norscode-koden med selfhost compiler-broen",
    register_arguments=register_selfhost_compile_all_arguments,
    run=run_selfhost_compile_all,
    bootstrap_only=True,
    experimental=True,
)
