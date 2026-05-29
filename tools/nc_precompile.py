#!/usr/bin/env python3
"""
nc_precompile.py — Kompilér alle Norscode-kildefiler til bytekode en gang.

Produserer build/nc-precompiled/<modul>.ncb.json for hvert .no-fil.
Disse kan deretter kjøres med dist/nc-vm uten Python.

Bruk: python3 tools/nc_precompile.py [--roots selfhost std compiler tests]
"""
import sys, os, json, argparse
from pathlib import Path

sys.path.insert(0, str(Path(__file__).parent.parent))
os.chdir(Path(__file__).parent.parent)

from compiler.selfhost_whole_compile import WholeCompileOptions, compile_whole_norscode

def main():
    parser = argparse.ArgumentParser()
    parser.add_argument("--roots", nargs="*", default=["selfhost", "compiler", "std"])
    parser.add_argument("--output-dir", default="build/nc-precompiled")
    args = parser.parse_args()

    print(f"Kompilerer {args.roots} → {args.output_dir} ...")
    opts = WholeCompileOptions(
        roots=tuple(args.roots),
        output_dir=args.output_dir,
        fail_fast=False,
    )
    result = compile_whole_norscode(opts)
    print(f"✓ {result['passed']}/{result['total']} kompilert")
    if result['failed']:
        print(f"✗ {result['failed']} feilet")
    return 0 if result['ok'] else 1

if __name__ == "__main__":
    sys.exit(main())
