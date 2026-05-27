"""Modular selfhost artifact and chain commands."""

from __future__ import annotations

import json
import os
from pathlib import Path

from compiler.ast_bridge import export_ast
from compiler.selfhost_ast_bridge import export_selfhost_ast
from compiler.selfhost_chain import build_ncb_cache, check_chain, export_selfhost_ast_bundle, export_selfhost_ncb, run_from_ncb

from norcode.bootstrap_support import _format_cli_exception
from norcode.commands.base import CommandModule


def register_selfhost_ast_export_arguments(parser) -> None:
    parser.add_argument("file")
    parser.add_argument("--output", help="Valgfri output-fil")


def run_selfhost_ast_export(args) -> int:
    try:
        out_path = export_selfhost_ast(args.file, output=args.output)
        print(f"Selfhost AST: {out_path}")
        return 0
    except Exception as exc:
        print(f"Feil: {_format_cli_exception(exc)}")
        return 1


def register_ast_export_arguments(parser) -> None:
    parser.add_argument("file")
    parser.add_argument("--output", help="Valgfri output-fil")


def run_ast_export(args) -> int:
    try:
        out_path = export_ast(args.file, output=args.output)
        print(f"AST: {out_path}")
        return 0
    except Exception as exc:
        print(f"Feil: {_format_cli_exception(exc)}")
        return 1


def register_selfhost_chain_export_arguments(parser) -> None:
    parser.add_argument("file")
    parser.add_argument("--output", help="Valgfri output-fil")


def run_selfhost_chain_export(args) -> int:
    try:
        out_path = export_selfhost_ast_bundle(args.file, output=args.output)
        print(f"Selfhost chain AST: {out_path}")
        return 0
    except Exception as exc:
        print(f"Feil: {_format_cli_exception(exc)}")
        return 1


def register_selfhost_chain_check_arguments(parser) -> None:
    parser.add_argument("files", nargs="*")
    parser.add_argument("--trace", action="store_true", help="Ta med sporlogg ved feil")
    parser.add_argument("--max-steps", type=int, default=5000000, help="Maks VM-steg per fil")
    parser.add_argument("--trace-focus", help="Logg kun funksjoner som matcher denne teksten")
    parser.add_argument("--repeat-limit", type=int, default=0, help="Avbryt hvis samme VM-tilstand gjentas mer enn N ganger")
    parser.add_argument("--expr-probe", help="Logg uttrykkstokens som matcher denne teksten")
    parser.add_argument("--expr-probe-log", help="Skriv uttrykksprobe til fil")
    parser.add_argument("--use-cache", action="store_true", help="Bruk NCB-cache (.chain.ncb.json) om tilgjengelig og fersk")
    parser.add_argument("--write-cache", action="store_true", help="Skriv NCB-cache etter kompilering")


def run_selfhost_chain_check(args) -> int:
    try:
        payload = check_chain(
            args.files,
            trace=args.trace,
            max_steps=args.max_steps,
            trace_focus=args.trace_focus,
            repeat_limit=args.repeat_limit,
            expr_probe=args.expr_probe,
            expr_probe_log=args.expr_probe_log,
            use_ncb_cache=getattr(args, "use_cache", False),
            write_ncb_cache=getattr(args, "write_cache", False),
        )
        print(f"{payload['passed']}/{payload['total']} OK")
        for row in payload["results"]:
            status = "OK" if row.get("ok") else "FEIL"
            detail = row.get("result") if row.get("ok") else row.get("error")
            print(f"- {status}: {row['file']}" + (f" -> {detail}" if detail is not None else ""))
        return 0 if payload["ok"] else 1
    except Exception as exc:
        print(f"Feil: {_format_cli_exception(exc)}")
        return 1


def register_selfhost_ncb_export_arguments(parser) -> None:
    parser.add_argument("file")
    parser.add_argument("--output", help="Valgfri output-fil (default: <fil>.chain.ncb.json)")


def run_selfhost_ncb_export(args) -> int:
    try:
        out_path = export_selfhost_ncb(args.file, output=getattr(args, "output", None))
        print(f"NCB: {out_path}")
        return 0
    except Exception as exc:
        print(f"Feil: {_format_cli_exception(exc)}")
        return 1


def register_selfhost_ncb_run_arguments(parser) -> None:
    parser.add_argument("file", help="Sti til .chain.ncb.json-fil")
    parser.add_argument("--trace", action="store_true")
    parser.add_argument("--max-steps", type=int, default=5000000)


def run_selfhost_ncb_run(args) -> int:
    try:
        result = run_from_ncb(
            args.file,
            trace=getattr(args, "trace", False),
            max_steps=getattr(args, "max_steps", 5000000),
        )
        if result is not None and os.environ.get("NORCODE_SUPPRESS_RETURN") not in {"1", "true", "TRUE", "yes", "YES"}:
            print(f"Return: {result}")
        return 0
    except Exception as exc:
        print(f"Feil: {_format_cli_exception(exc)}")
        return 1


def register_selfhost_ncb_build_cache_arguments(parser) -> None:
    parser.add_argument("files", nargs="*", help="Filer å pre-kompilere (default: standard chain-testfiler)")


def run_selfhost_ncb_build_cache(args) -> int:
    try:
        report = build_ncb_cache(args.files or None)
        print(f"Bygd: {len(report['built'])}, Hoppa over: {len(report['skipped'])}, Feil: {len(report['errors'])}")
        for item in report["built"]:
            print(f"  + {item}")
        for err in report["errors"]:
            print(f"  ! FEIL {err['file']}: {err['error']}")
        return 0 if not report["errors"] else 1
    except Exception as exc:
        print(f"Feil: {_format_cli_exception(exc)}")
        return 1


SELFHOST_AST_EXPORT_COMMAND = CommandModule(
    name="selfhost-ast-export",
    help="Eksporter .no via selfhost-parser til AST-json (.shast.json)",
    register_arguments=register_selfhost_ast_export_arguments,
    run=run_selfhost_ast_export,
)

AST_EXPORT_COMMAND = CommandModule(
    name="ast-export",
    help="Eksporter .no til AST-json (.nast.json)",
    register_arguments=register_ast_export_arguments,
    run=run_ast_export,
)

SELFHOST_CHAIN_EXPORT_COMMAND = CommandModule(
    name="selfhost-chain-export",
    help="Eksporter full selfhost AST-bundle inkl. imports",
    register_arguments=register_selfhost_chain_export_arguments,
    run=run_selfhost_chain_export,
)

SELFHOST_CHAIN_CHECK_COMMAND = CommandModule(
    name="selfhost-chain-check",
    help="Sjekk et sett filer gjennom full selfhost-kjede",
    register_arguments=register_selfhost_chain_check_arguments,
    run=run_selfhost_chain_check,
)

SELFHOST_NCB_EXPORT_COMMAND = CommandModule(
    name="selfhost-ncb-export",
    help="Kompiler .no heilt til NCB-bytecode og skriv til .chain.ncb.json",
    register_arguments=register_selfhost_ncb_export_arguments,
    run=run_selfhost_ncb_export,
)

SELFHOST_NCB_RUN_COMMAND = CommandModule(
    name="selfhost-ncb-run",
    help="Køyr pre-kompilert NCB-fil via BytecodeVM (utan Python-parsing)",
    register_arguments=register_selfhost_ncb_run_arguments,
    run=run_selfhost_ncb_run,
)

SELFHOST_NCB_BUILD_CACHE_COMMAND = CommandModule(
    name="selfhost-ncb-build-cache",
    help="Pre-kompiler alle chain-testfiler til NCB-cache",
    register_arguments=register_selfhost_ncb_build_cache_arguments,
    run=run_selfhost_ncb_build_cache,
)
