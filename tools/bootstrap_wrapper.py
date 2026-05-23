#!/usr/bin/env python3
"""
norcode-bootstrap-compile — Python VM bootstrap compiler
=========================================================
Drop-in Python-erstatter for den C-kompilerte bootstrap-binaryen.
Handterer same env-var-protokoll som C VM-et, men bruker Python VM.

Protokoll:
  NORCODE_BOOTSTRAP_CLI=1  → CLI-modus (selfcheck, test, identity …)
  NORCODE_BOOTSTRAP_VM=1   → Kompilator-modus (run/build via selfhost-chain)
  (ingen)                  → Native-modus (ikkje tilgjengeleg utan C VM)

Generert av tools/build-bootstrap-binary.sh — ingen C-kompilator trengst.
"""
from __future__ import annotations

import os
import sys
from pathlib import Path

# ─── Finn prosjekt-rotet frå plasseringa av dette scriptet ──────────────────
# Scriptet ligg i dist/, prosjektrotet er ein nivå opp.
ROOT = Path(__file__).resolve().parent.parent
if str(ROOT) not in sys.path:
    sys.path.insert(0, str(ROOT))

# ─── Hovudfunksjon ───────────────────────────────────────────────────────────

def main() -> int:
    if os.environ.get("NORCODE_BOOTSTRAP_CLI"):
        return _cli_mode()
    if os.environ.get("NORCODE_BOOTSTRAP_VM"):
        return _compiler_mode()
    return _native_mode()


# ─── CLI-modus: selfcheck, test, identity, release, help ─────────────────────

def _cli_mode() -> int:
    argc = int(os.environ.get("NORCODE_ARGC", "1"))
    args = [os.environ.get(f"NORCODE_ARG{i}", "") for i in range(max(argc, 1))]
    cmd = args[0] if args else "help"

    if cmd == "selfcheck":
        return _selfcheck()

    if cmd == "test":
        return _selfcheck()  # selfcheck er den primære testen

    if cmd in ("identity", "release", "release-targets"):
        # Deleger til selfcheck for no — utvid etter kvart
        return _selfcheck()

    if cmd in ("help", "--help", "-h"):
        _print_help()
        return 0

    print(f"norcode: ukjent CLI-kommando: {cmd}", file=sys.stderr)
    return 1


def _selfcheck() -> int:
    """Køyr selfhost-chain smoke-test på standard testfiler.

    Ekskluderer testar med eksterne pakkeavhengigheiter (std.tekst, butikk o.l.)
    som ikkje finst i denne distribusjonen.
    """
    try:
        from compiler.selfhost_chain import check_chain
        # Berre testar utan eksterne pakke-avhengigheiter
        files = [
            str(ROOT / "tests" / name)
            for name in [
                "test_if.no",
                "test_math.no",
                "test_assert.no",
                "test_for.no",
                "test_while.no",
                "test_elif.no",
                "test_selfhost_ifexpr_v21.no",
                "test_selfhost_indexset_v22.no",
                "test_empty_string_list.no",
            ]
        ]
        result = check_chain(files=files)
        passed = result.get("passed", 0)
        total = result.get("total", 0)
        ok = result.get("ok", False)
        if ok:
            print(f"selfcheck: {passed}/{total} OK")
            return 0
        else:
            for row in result.get("results", []):
                if not row.get("ok"):
                    fname = Path(row.get("file", "?")).name
                    err = row.get("error", "?")
                    print(f"  FEIL: {fname}: {err}", file=sys.stderr)
            print(f"selfcheck: {passed}/{total} OK — FEILA", file=sys.stderr)
            return 1
    except Exception as exc:
        print(f"norcode: selfcheck feila: {exc}", file=sys.stderr)
        return 1


def _print_help() -> None:
    print("Norscode Bootstrap Compiler (Python VM)")
    print("Kommandoar:")
    print("  selfcheck                     Verifiser kompilator (selfhost-chain)")
    print("  test                          Alias for selfcheck")
    print("  identity                      Kompilatoridentitet")
    print("  release                       Release-gate")
    print("  release-targets               Bygg release-mål")
    print("  help                          Vis dette")


# ─── Kompilator-modus: run / build via selfhost-chain ────────────────────────

def _compiler_mode() -> int:
    cmd = os.environ.get("NORCODE_CMD", "build")
    src = os.environ.get("NORCODE_SRC", "")
    out = os.environ.get("NORCODE_OUT", "")
    arch = os.environ.get("NORCODE_ARCH", "aarch64")

    if not src:
        print("norcode: NORCODE_SRC manglar", file=sys.stderr)
        return 11
    if not out and cmd == "build":
        print("norcode: NORCODE_OUT manglar", file=sys.stderr)
        return 12

    try:
        if cmd == "run":
            return _run_source(src, out, arch)
        elif cmd == "build":
            return _build_source(src, out, arch)
        else:
            print(f"norcode: ukjent NORCODE_CMD: {cmd}", file=sys.stderr)
            return 13
    except Exception as exc:
        print(f"norcode: {exc}", file=sys.stderr)
        return 1


def _run_source(src: str, out: str, arch: str) -> int:
    """Kompiler og køyr eit .no-program."""
    from compiler.selfhost_chain import run_chain
    result = run_chain(src, max_steps=20_000_000)
    if result is not None:
        print(f"Return: {result}")
    return 0


def _build_source(src: str, out: str, arch: str) -> int:
    """Kompiler eit .no-program til binær (native ELF for x86_64, VM-bytecode elles)."""
    # Prøv den native Python-ELF-pipeline for Linux x86_64 / same-arch
    import platform
    is_linux_x86 = platform.system() == "Linux" and platform.machine() in {"x86_64", "AMD64"}
    is_macos_arm = platform.system() == "Darwin" and platform.machine() == "arm64"
    target_arch = arch.lower()

    if (is_linux_x86 and target_arch in ("x86_64", "amd64")) or \
       (is_macos_arm and target_arch == "aarch64"):
        try:
            from compiler.native.pipeline import compile_source_to_native_elf
            build = compile_source_to_native_elf(src, output_path=out or None)
            if build.executable:
                return build.exit_code if build.exit_code >= 0 else 0
            print("norcode: ELF ikkje kjørbar", file=sys.stderr)
            return 1
        except Exception as exc:
            print(f"norcode: native ELF feil: {exc}", file=sys.stderr)
            return 1

    # Fallback: selfhost-chain produserer NCB JSON (portabelt bytecode)
    from compiler.selfhost_chain import build_selfhost_ast_bundle
    from compiler.ast_bridge import program_from_data
    from norcode.bytecode_service import compile_program_to_bytecode
    import json

    _src_path, bundle = build_selfhost_ast_bundle(src)
    program, alias_map = program_from_data(bundle)
    bytecode = compile_program_to_bytecode(program, alias_map=alias_map)
    out_path = Path(out) if out else Path(src).with_suffix(".ncb.json")
    out_path.write_text(json.dumps(bytecode, ensure_ascii=False, indent=2), encoding="utf-8")
    print(f"norcode: NCB JSON → {out_path}")
    return 0


# ─── Native-modus: ikkje tilgjengeleg ────────────────────────────────────────

def _native_mode() -> int:
    # Native-modus krev C VM eller ekte native Norscode-kompilator.
    # Python VM-wrapparen støttar ikkje native-modus direkte.
    print(
        "norcode: native-modus ikkje tilgjengeleg i Python VM-wrapper.\n"
        "Bruk: NORCODE_BOOTSTRAP_VM=1 eller NORCODE_BOOTSTRAP_CLI=1",
        file=sys.stderr,
    )
    return 1


# ─── Entry-point ─────────────────────────────────────────────────────────────

if __name__ == "__main__":
    sys.exit(main())
