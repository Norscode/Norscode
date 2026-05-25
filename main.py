from __future__ import annotations

import argparse
import datetime as dt
import hashlib
import json
import locale
import os
import re
import shutil
import shlex
import sys
import tarfile
import tempfile
import time
import platform
from concurrent.futures import ThreadPoolExecutor, as_completed
from http.server import BaseHTTPRequestHandler, ThreadingHTTPServer
import threading
import signal
import urllib.parse
import urllib.error
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
from compiler.loader import ModuleLoader
from compiler.native.pipeline import compile_source_to_native_elf, run_native_elf, verify_bootstrap_compiler
from compiler.parser import Parser
from compiler.semantic import SemanticAnalyzer
from compiler.selfhost_chain import export_selfhost_ast_bundle, export_selfhost_ncb, run_chain, run_from_ncb, check_chain, build_ncb_cache
from compiler.selfhost_whole_compile import DEFAULT_ROOTS, WholeCompileOptions, compile_whole_norscode
from compiler.toml_compat import loads as toml_loads
from norcode.ci_pipeline import run_ci_pipeline as run_ci_pipeline_module
from norcode.bootstrap_ci import (
    WORKFLOW_ACTION_POLICY,
    check_workflow_action_versions,
    run_ci_bootstrap_lane as run_bootstrap_ci_lane,
    run_selfhost_bootstrap_gate,
)
from norcode.bootstrap_support import (
    _format_cli_exception,
)
from norcode.compiler_service import build_program, check_program, disasm_program, load_program, parse_source, run_program
from norcode.ir_tools import (
    SELFHOST_PARSER_EXTENDED_FIXTURE,
    SELFHOST_PARSER_M1_FIXTURE,
    SELFHOST_PARSER_M2_FIXTURE,
    ir_disasm_source,
    ir_disasm_source_captured,
    run_ir_snapshot_checks,
    update_ir_snapshots,
)
from norcode.parity_tools import (
    run_selfhost_parity_gate,
    run_selfhost_parity_progress,
    run_selfhost_parser_core_checks,
    run_selfhost_parser_parity,
    run_selfhost_parser_suite_all_consistency_check,
    run_selfhost_parser_suite_consistency_check,
    run_selfhost_parser_suite_subset_consistency_check,
    sync_selfhost_parser_m2_fixture,
    update_selfhost_parser_fixtures,
)
from norcode.server_runtime import serve_program as serve_program
from norcode.command_dispatch import dispatch_command
from norcode.diagnostics import run_diagnostics as run_project_diagnostics
from norcode.package_registry import (
    add_dependency as package_add_dependency,
    generate_lockfile as package_generate_lockfile,
    list_registry_packages as package_list_registry_packages,
    registry_host as package_registry_host,
    registry_mirror as package_registry_mirror,
    registry_sign as package_registry_sign,
    registry_sync as package_registry_sync,
    update_dependencies as package_update_dependencies,
    verify_lockfile as package_verify_lockfile,
)
from norcode.migrations import migrate_names as run_migrate_names
from norcode.repl import run_repl
from norcode.testing_support import run_all_tests
from norcode.commands.registry import COMMANDS


MODULAR_COMMANDS = {
    command.name: command
    for command in COMMANDS
    if command.run is not None
}
SEMVER_RE = re.compile(r"^\d+\.\d+\.\d+$")

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
    MODULAR_COMMANDS["check"].register_arguments(check)

    build = sub.add_parser("build", help="Generer C og bygg kjørbar fil")
    build.add_argument("file")

    native_build = sub.add_parser("native-build", help="Bygg ekte Linux x86_64 ELF fra enkel Norscode-entry")
    native_build.add_argument("file", help="Kildefil å bygge")
    native_build.add_argument("--output", "-o", help="Output ELF-fil (default: <file>.elf)")
    native_build.add_argument("--json", action="store_true", help="Skriv native build-resultat som JSON")

    native_run = sub.add_parser("native-run", help="Bygg og kjør ekte Linux x86_64 ELF når host støtter det")
    native_run.add_argument("file", help="Kildefil å bygge og kjøre")
    native_run.add_argument("--output", "-o", help="Output ELF-fil (default: <file>.elf)")
    native_run.add_argument("--json", action="store_true", help="Skriv native run-resultat som JSON")

    bootstrap_verify = sub.add_parser("bootstrap-compiler-verify", help="Verifiser real bootstrap compiler native lane")
    bootstrap_verify.add_argument("--json", action="store_true", help="Skriv bootstrap-verifikasjon som JSON")

    selfhost_bootstrap_gate = sub.add_parser("selfhost-bootstrap-gate", help="Kjør whole selfhost compile + native bootstrap gate")
    selfhost_bootstrap_gate.add_argument("--output-dir", default="build/selfhost-bootstrap-gate", help="Output-katalog for whole-compile artefakter")
    selfhost_bootstrap_gate.add_argument("--no-determinism", action="store_true", help="Hopp over dobbelkompilering/digest-sjekk")
    selfhost_bootstrap_gate.add_argument("--json", action="store_true", help="Skriv gate-resultat som JSON")

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
    ci.add_argument("--bootstrap-lane", action="store_true", help="Kjør bare fysisk selfhost bootstrap gate + workflow-policy")
    ci.add_argument("--bootstrap-output-dir", default="build/selfhost-bootstrap-gate", help="Output-katalog for --bootstrap-lane artefakter")
    ci.add_argument(
        "--require-selfhost-ready",
        action="store_true",
        help="Feil hvis selfhost parity eller whole-compile ikke er fullført/ready",
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

    registry_host_cmd = sub.add_parser("registry-host", help="Host registry-speilfil over HTTP")
    registry_host_cmd.add_argument("--host", default="127.0.0.1", help="Bind-adresse for registry host")
    registry_host_cmd.add_argument("--port", type=int, default=8765, help="Port for registry host")
    registry_host_cmd.add_argument("--mirror", help="Mirror-fil å skrive/servere (default: build/registry_mirror.json)")
    registry_host_cmd.add_argument("--once", action="store_true", help="Start, hent index én gang og stopp")
    registry_host_cmd.add_argument("--json", action="store_true", help="Skriv resultat som JSON")

    migrate_names_cmd = sub.add_parser("migrate-names", help="Migrer legacy navn (norsklang*) til Norscode-navn")
    MODULAR_COMMANDS["migrate-names"].register_arguments(migrate_names_cmd)

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
    selfhost_chain_check.add_argument("--use-cache", action="store_true", help="Bruk NCB-cache (.chain.ncb.json) om tilgjengelig og fersk")
    selfhost_chain_check.add_argument("--write-cache", action="store_true", help="Skriv NCB-cache etter kompilering")

    selfhost_ncb_export = sub.add_parser("selfhost-ncb-export", help="Kompiler .no heilt til NCB-bytecode og skriv til .chain.ncb.json")
    selfhost_ncb_export.add_argument("file")
    selfhost_ncb_export.add_argument("--output", help="Valgfri output-fil (default: <fil>.chain.ncb.json)")

    selfhost_ncb_run = sub.add_parser("selfhost-ncb-run", help="Køyr pre-kompilert NCB-fil via BytecodeVM (utan Python-parsing)")
    selfhost_ncb_run.add_argument("file", help="Sti til .chain.ncb.json-fil")
    selfhost_ncb_run.add_argument("--trace", action="store_true")
    selfhost_ncb_run.add_argument("--max-steps", type=int, default=5000000)

    selfhost_ncb_build_cache = sub.add_parser("selfhost-ncb-build-cache", help="Pre-kompiler alle chain-testfiler til NCB-cache")
    selfhost_ncb_build_cache.add_argument("files", nargs="*", help="Filer å pre-kompilere (default: standard chain-testfiler)")

    selfhost_compile_all = sub.add_parser("selfhost-compile-all", help="Kompiler hele Norscode-koden med selfhost compiler-broen")
    selfhost_compile_all.add_argument("--root", action="append", dest="roots", help="Root å kompilere (kan gjentas, default: selfhost/compiler/std)")
    selfhost_compile_all.add_argument("--output-dir", default="build/selfhost-whole", help="Output-katalog for AST, bytecode og manifest")
    selfhost_compile_all.add_argument("--fail-fast", action="store_true", help="Stopp ved første compile-feil")
    selfhost_compile_all.add_argument("--json", action="store_true", help="Skriv compile-manifest som JSON")

    selfhost_parser_suite = sub.add_parser("selfhost-parser-suite", help="Kjør minimal selfhost parser suite")
    MODULAR_COMMANDS["selfhost-parser-suite"].register_arguments(selfhost_parser_suite)

    selfhost_bytecode_suite = sub.add_parser("selfhost-bytecode-suite", help="Kjør minimal selfhost bytecode suite")
    MODULAR_COMMANDS["selfhost-bytecode-suite"].register_arguments(selfhost_bytecode_suite)

    selfhost_bootstrap_build = sub.add_parser("selfhost-build", help="Bygg selfhost bootstrap-kjeden med Python-first compiler")
    MODULAR_COMMANDS["selfhost-build"].register_arguments(selfhost_bootstrap_build)

    selfhost_bootstrap_check = sub.add_parser("selfhost-check", help="Sjekk deterministisk selfhost bootstrap-kjede mot golden hash")
    MODULAR_COMMANDS["selfhost-check"].register_arguments(selfhost_bootstrap_check)

    selfhost_semantic_suite = sub.add_parser("selfhost-semantic-suite", help="Kjør minimal selfhost semantic suite")
    MODULAR_COMMANDS["selfhost-semantic-suite"].register_arguments(selfhost_semantic_suite)

    selfhost_stdlib_suite = sub.add_parser("selfhost-stdlib-suite", help="Kjør minimal selfhost stdlib/compiler suite")
    MODULAR_COMMANDS["selfhost-stdlib-suite"].register_arguments(selfhost_stdlib_suite)

    test = sub.add_parser("test", help="Kjør én testfil eller alle i tests/")
    MODULAR_COMMANDS["test"].register_arguments(test)

    format_cmd = sub.add_parser("format", help="Formater en .no-fil")
    MODULAR_COMMANDS["format"].register_arguments(format_cmd)

    lint = sub.add_parser("lint", help="Kjør en enkel linter på en .no-fil")
    MODULAR_COMMANDS["lint"].register_arguments(lint)

    bench = sub.add_parser("bench", help="Kjør faste ytelsesmålinger")
    MODULAR_COMMANDS["bench"].register_arguments(bench)

    smoke = sub.add_parser("smoke", help="Kjør fresh install/release smoke-test")
    MODULAR_COMMANDS["smoke"].register_arguments(smoke)

    serve_e2e = sub.add_parser("serve-e2e", help="Kjør e2e-tester av serveradapteren i flere miljøer")
    MODULAR_COMMANDS["serve-e2e"].register_arguments(serve_e2e)

    stress = sub.add_parser("stress", help="Kjør produksjonsnære stresstester av serveradapteren")
    MODULAR_COMMANDS["stress"].register_arguments(stress)

    security = sub.add_parser("security", help="Kjør auth- og input-sikkerhetstester")
    MODULAR_COMMANDS["security"].register_arguments(security)

    scaffold_api = sub.add_parser("scaffold-api", help="Lag et nytt API-prosjekt med standard struktur")
    MODULAR_COMMANDS["scaffold-api"].register_arguments(scaffold_api)

    diagnose = sub.add_parser("diagnose", help="Vis samlet diagnose for prosjekt og runtime")
    MODULAR_COMMANDS["diagnose"].register_arguments(diagnose)

    doctor = sub.add_parser("doctor", help="Kjør installasjons- og distribusjonskontroll")
    MODULAR_COMMANDS["doctor"].register_arguments(doctor)

    fuzz = sub.add_parser("fuzz", help="Kjør negativ parser- og runtime-korpus")
    MODULAR_COMMANDS["fuzz"].register_arguments(fuzz)

    commands = sub.add_parser("commands", help="Vis stabil kommandooversikt")
    commands.add_argument("--json", action="store_true", help="Skriv kommandooversikt som JSON")

    serve = sub.add_parser("serve", help="Start en lokal webserver for en Norscode-app")
    MODULAR_COMMANDS["serve"].register_arguments(serve)

    ui_render = sub.add_parser("ui-render", help="Render Native UI-syntax til HTML")
    MODULAR_COMMANDS["ui-render"].register_arguments(ui_render)

    args = parser.parse_args()

    try:
        modular_command = MODULAR_COMMANDS.get(args.cmd)
        if modular_command is not None:
            exit_code = dispatch_command(SimpleNamespace(**vars(args), command_module=modular_command))
            if exit_code != 0:
                sys.exit(exit_code)
            return

        if args.cmd == "run":
            run_program(args.file)

        elif args.cmd == "repl":
            run_repl()

        elif args.cmd == "build":
            _source_path, c_path, exe_path, _alias_map, _analyzer = build_program(args.file)
            print(f"Generert C-fil: {c_path}")
            print("Kompilert med: clang")
            print(f"Kjørbar fil: {exe_path}")

        elif args.cmd == "native-build":
            result = compile_source_to_native_elf(args.file, output_path=args.output)
            payload = {
                "source": str(result.source),
                "output": str(result.output),
                "exit_code": result.exit_code,
                "machine_code_hex": result.machine_code.hex(),
                "elf_magic": result.elf_image[:4].hex(),
                "entry_address": hex(result.entry_address),
                "executable": result.executable,
                "size": len(result.elf_image),
            }
            if args.json:
                print(json.dumps(payload, ensure_ascii=False, indent=2))
            else:
                print(f"Native ELF: {payload['output']}")
                print(f"Machine code: {payload['machine_code_hex']}")
                print(f"ELF magic: {payload['elf_magic']}")
                print(f"Entry: {payload['entry_address']}")
                print(f"Exit code: {payload['exit_code']}")

        elif args.cmd == "native-run":
            result = run_native_elf(args.file, output_path=args.output)
            payload = {
                "source": str(result.build.source),
                "output": str(result.build.output),
                "exit_code": result.build.exit_code,
                "machine_code_hex": result.build.machine_code.hex(),
                "elf_magic": result.build.elf_image[:4].hex(),
                "entry_address": hex(result.build.entry_address),
                "executable": result.build.executable,
                "ran": result.ran,
                "returncode": result.returncode,
                "stdout": result.stdout,
                "stderr": result.stderr,
                "skip_reason": result.reason,
            }
            if args.json:
                print(json.dumps(payload, ensure_ascii=False, indent=2))
            else:
                print(f"Native ELF: {payload['output']}")
                print(f"Ran: {'ja' if payload['ran'] else 'nei'}")
                if payload["ran"]:
                    print(f"Returncode: {payload['returncode']}")
                else:
                    print(f"Årsak: {payload['skip_reason']}")
            if result.ran and result.returncode != result.build.exit_code:
                sys.exit(1)

        elif args.cmd == "bootstrap-compiler-verify":
            payload = verify_bootstrap_compiler()
            if args.json:
                print(json.dumps(payload, ensure_ascii=False, indent=2))
            else:
                print(f"OK: {'ja' if payload['ok'] else 'nei'}")
                print(f"Machine code: {payload['machine_code_hex']}")
                print(f"ELF magic: {payload['elf_magic']}")
                print(f"Entry: {payload['entry_address']}")
                print(f"Ran: {'ja' if payload['ran'] else 'nei'}")
                if payload.get("skip_reason"):
                    print(f"Årsak: {payload['skip_reason']}")
            if not payload["ok"]:
                sys.exit(1)

        elif args.cmd == "selfhost-bootstrap-gate":
            payload = run_selfhost_bootstrap_gate(
                output_dir=args.output_dir,
                verify_deterministic=not args.no_determinism,
            )
            if args.json:
                print(json.dumps(payload, ensure_ascii=False, indent=2))
            else:
                whole = payload["selfhost_whole_compile"]
                native = payload["native_bootstrap_verify"]
                print(f"Selfhost bootstrap gate: {'OK' if payload['ok'] else 'FEIL'}")
                print(f"Whole compile: {whole.get('passed')}/{whole.get('total')} OK")
                print(f"Manifest: {whole.get('manifest')}")
                if payload.get("determinism_check", {}).get("enabled"):
                    print(
                        "Determinisme: "
                        f"{'OK' if payload['determinism_check'].get('ok') else 'FEIL'} "
                        f"({payload['determinism_check'].get('digest')})"
                    )
                execution = payload.get("selfhost_execution_smoke", {})
                print(
                    "Selfhost execution: "
                    f"{'OK' if execution.get('ok') else 'FEIL'} "
                    f"(return={execution.get('result')})"
                )
                package_hosting = payload.get("package_hosting_smoke", {})
                print(
                    "Package hosting: "
                    f"{'OK' if package_hosting.get('ok') else 'FEIL'} "
                    f"({package_hosting.get('url')})"
                )
                if package_hosting.get("skip_reason"):
                    print(f"Package hosting hoppet over: {package_hosting.get('skip_reason')}")
                print(f"Native bootstrap: {'OK' if native.get('ok') else 'FEIL'}")
                if native.get("skip_reason"):
                    print(f"Native run hoppet over: {native.get('skip_reason')}")
            if not payload["ok"]:
                sys.exit(1)

        elif args.cmd == "add":
            if args.list:
                config_path, entries = package_list_registry_packages()
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

            config_path, dep_name, dep_value, package_name, dep_kind, changed = package_add_dependency(
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
            if args.bootstrap_lane:
                payload = run_bootstrap_ci_lane(
                    json_output=args.json,
                    check_names=args.check_names,
                    output_dir=args.bootstrap_output_dir,
                    argv=sys.argv[1:],
                    migrate_names_fn=run_migrate_names,
                )
            else:
                payload = run_ci_pipeline_module(
                    json_output=args.json,
                    check_names=args.check_names,
                    parity_suite=args.parity_suite,
                    require_selfhost_ready=args.require_selfhost_ready,
                    argv=sys.argv[1:],
                    update_ir_snapshots_fn=update_ir_snapshots,
                    update_selfhost_parser_fixtures_fn=update_selfhost_parser_fixtures,
                    run_all_tests_fn=run_all_tests,
                    run_selfhost_parser_core_checks_fn=run_selfhost_parser_core_checks,
                    run_selfhost_parser_suite_consistency_check_fn=run_selfhost_parser_suite_consistency_check,
                    run_selfhost_parser_suite_subset_consistency_check_fn=run_selfhost_parser_suite_subset_consistency_check,
                    run_selfhost_parser_suite_all_consistency_check_fn=run_selfhost_parser_suite_all_consistency_check,
                    sync_selfhost_parser_m2_fixture_fn=sync_selfhost_parser_m2_fixture,
                    run_selfhost_parity_progress_fn=run_selfhost_parity_progress,
                    migrate_names_fn=run_migrate_names,
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
                lock_path, ok, results = package_verify_lockfile()
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
                lock_path, ok, status = package_generate_lockfile(check_only=args.check)
                if args.json:
                    print(json.dumps({"lockfile": str(lock_path), "ok": ok, "status": status, "check": args.check}, ensure_ascii=False, indent=2))
                else:
                    print(f"Lockfile: {lock_path}")
                    print(f"Status: {status}")
                if args.check and not ok:
                    sys.exit(1)

        elif args.cmd == "update":
            payload = package_update_dependencies(
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
            payload = package_registry_sign(write_config=args.write_config)
            if args.json:
                print(json.dumps(payload, ensure_ascii=False, indent=2))
            else:
                print(f"Registry: {payload['registry']}")
                print(f"SHA256: {payload['sha256']}")
                if payload["written_to_config"]:
                    print(f"Konfig: {payload['config']}")
                    print("Status: oppdatert" if payload["config_changed"] else "Status: uendret")

        elif args.cmd == "registry-sync":
            payload = package_registry_sync(
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
            payload = package_registry_mirror(output_file=args.output)
            if args.json:
                print(json.dumps(payload, ensure_ascii=False, indent=2))
            else:
                print(f"Mirror: {payload['output']}")
                print(f"Pakker: {payload['count']}")

        elif args.cmd == "registry-host":
            payload = package_registry_host(host=args.host, port=args.port, mirror_file=args.mirror, once=args.once)
            if args.once:
                if args.json:
                    print(json.dumps(payload, ensure_ascii=False, indent=2))
                else:
                    print(f"Registry host: {payload['url']}")
                    print(f"Mirror: {payload['mirror']}")
                    print(f"Pakker: {payload['count']}")
                    print(f"Served bytes: {payload['served_bytes']}")
                    print(f"OK: {'ja' if payload['ok'] else 'nei'}")
                if not payload["ok"]:
                    sys.exit(1)
            else:
                server = payload.pop("server")
                if args.json:
                    print(json.dumps(payload, ensure_ascii=False, indent=2), flush=True)
                else:
                    print(f"Registry host: {payload['url']}", flush=True)
                    print(f"Mirror: {payload['mirror']}", flush=True)
                    print("Stop med Ctrl-C.", flush=True)
                try:
                    server.serve_forever()
                except KeyboardInterrupt:
                    pass
                finally:
                    server.server_close()

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
            if result is not None and os.environ.get("NORCODE_SUPPRESS_RETURN") not in {"1", "true", "TRUE", "yes", "YES"}:
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
            if result is not None and os.environ.get("NORCODE_SUPPRESS_RETURN") not in {"1", "true", "TRUE", "yes", "YES"}:
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
                use_ncb_cache=getattr(args, 'use_cache', False),
                write_ncb_cache=getattr(args, 'write_cache', False),
            )
            print(f"{payload['passed']}/{payload['total']} OK")
            for row in payload['results']:
                status = "OK" if row.get("ok") else "FEIL"
                detail = row.get("result") if row.get("ok") else row.get("error")
                print(f"- {status}: {row['file']}" + (f" -> {detail}" if detail is not None else ""))

        elif args.cmd == "selfhost-ncb-export":
            from compiler.selfhost_chain import export_selfhost_ncb
            out_path = export_selfhost_ncb(args.file, output=getattr(args, 'output', None))
            print(f"NCB: {out_path}")

        elif args.cmd == "selfhost-ncb-run":
            from compiler.selfhost_chain import run_from_ncb
            result = run_from_ncb(
                args.file,
                trace=getattr(args, 'trace', False),
                max_steps=getattr(args, 'max_steps', 5000000),
            )
            if result is not None and os.environ.get("NORCODE_SUPPRESS_RETURN") not in {"1", "true", "TRUE", "yes", "YES"}:
                print(f"Return: {result}")

        elif args.cmd == "selfhost-ncb-build-cache":
            from compiler.selfhost_chain import build_ncb_cache
            report = build_ncb_cache(args.files or None)
            print(f"Bygd: {len(report['built'])}, Hoppa over: {len(report['skipped'])}, Feil: {len(report['errors'])}")
            for item in report['built']:
                print(f"  + {item}")
            for err in report['errors']:
                print(f"  ! FEIL {err['file']}: {err['error']}")
            if not payload['ok']:
                sys.exit(1)

        elif args.cmd == "selfhost-compile-all":
            payload = compile_whole_norscode(
                WholeCompileOptions(
                    roots=tuple(args.roots or ("selfhost", "compiler", "std")),
                    output_dir=args.output_dir,
                    fail_fast=args.fail_fast,
                )
            )
            if args.json:
                print(json.dumps(payload, ensure_ascii=False, indent=2))
            else:
                print(f"Selfhost whole compile: {payload['passed']}/{payload['total']} OK")
                print(f"Manifest: {payload['manifest']}")
                for row in payload["results"]:
                    if row.get("ok"):
                        continue
                    print(f"- FEIL: {row['file']} -> {row.get('error')}")
            if not payload["ok"]:
                sys.exit(1)

        elif args.cmd == "diagnose":
            payload = run_project_diagnostics(path=args.path)
            if args.json:
                print(json.dumps(payload, ensure_ascii=False, indent=2))
            else:
                print(f"OK: {'ja' if payload.get('ok') else 'nei'}")
                print(f"Sti: {payload['root']}")
                print(f"Prosjektrot: {payload.get('project_root') or 'fant ikke'}")
                print(f"Konfig: {payload.get('config_path') or 'fant ikke'}")
                print(
                    f"Prosjekt: {payload.get('project_name') or '-'} "
                    f"entry={payload.get('project_entry') or '-'}"
                )
                print(
                    "Paths: "
                    f"source={payload['paths'].get('source') or '-'} "
                    f"stdlib={payload['paths'].get('stdlib') or '-'} "
                    f"build={payload['paths'].get('build') or '-'}"
                )
                print(f"Stdlib-roots: {', '.join(payload['stdlib_roots']) if payload['stdlib_roots'] else '-'}")
                print(f"std.web resolvable: {'ja' if payload['stdlib_resolves_web'] else 'nei'}")
                print(f"Dependencies: {payload['dependency_count']}")
                print(f"Tester: {payload['test_count']}")
                print(
                    "Git: "
                    f"branch={payload['git'].get('branch') or '-'} "
                    f"dirty={'ja' if payload['git'].get('dirty') else 'nei'} "
                    f"rev={payload['git'].get('revision') or '-'}"
                )
                if payload["tests"]:
                    print("Første tester:")
                    for test_path in payload["tests"]:
                        print(f"- {test_path}")

        elif args.cmd == "doctor":
            exit_code = dispatch_command(SimpleNamespace(**vars(args), command_module=MODULAR_COMMANDS["doctor"]))
            if exit_code != 0:
                sys.exit(exit_code)
            return

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

        else:
            parser.print_help()
            sys.exit(1)

    except subprocess.CalledProcessError as e:
        if getattr(e, "stdout", None):
            print("STDOUT:")
            print(e.stdout, end="" if str(e.stdout).endswith("\n") else "\n")
        if getattr(e, "stderr", None):
            print("STDERR:")
            print(e.stderr, end="" if str(e.stderr).endswith("\n") else "\n")
        print(f"Feil: {_format_cli_exception(e)}")
        sys.exit(1)
    except Exception as e:
        print(f"Feil: {_format_cli_exception(e)}")
        sys.exit(1)


if __name__ == "__main__":
    main()
