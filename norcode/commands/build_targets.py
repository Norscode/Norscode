"""Build and native build command modules."""

from __future__ import annotations

import json

from compiler.native.pipeline import compile_source_to_native_elf, run_native_elf

from norcode.commands.base import CommandModule
from norcode.compiler_service import build_program


def register_build_arguments(parser) -> None:
    parser.add_argument("file")


def run_build(args) -> int:
    _source_path, c_path, exe_path, _alias_map, _analyzer = build_program(args.file)
    print(f"Generert C-fil: {c_path}")
    print("Kompilert med: clang")
    print(f"Kjørbar fil: {exe_path}")
    return 0


def register_native_build_arguments(parser) -> None:
    parser.add_argument("file")
    parser.add_argument("--output", "-o", help="Output ELF-fil (default: <file>.elf)")
    parser.add_argument("--json", action="store_true", help="Skriv native build-resultat som JSON")


def run_native_build(args) -> int:
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
    return 0


def register_native_run_arguments(parser) -> None:
    parser.add_argument("file")
    parser.add_argument("--output", "-o", help="Output ELF-fil (default: <file>.elf)")
    parser.add_argument("--json", action="store_true", help="Skriv native run-resultat som JSON")


def run_native_run(args) -> int:
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
        return 1
    return 0


BUILD_COMMAND = CommandModule(
    name="build",
    help="Generer C og bygg kjørbar fil",
    register_arguments=register_build_arguments,
    run=run_build,
)

NATIVE_BUILD_COMMAND = CommandModule(
    name="native-build",
    help="Bygg ekte Linux x86_64 ELF fra enkel Norscode-entry",
    register_arguments=register_native_build_arguments,
    run=run_native_build,
    bootstrap_only=True,
)

NATIVE_RUN_COMMAND = CommandModule(
    name="native-run",
    help="Bygg og kjør ekte Linux x86_64 ELF når host støtter det",
    register_arguments=register_native_run_arguments,
    run=run_native_run,
    bootstrap_only=True,
)
