"""Build command modules — Python-fri primærbane via bin/nc build."""
from __future__ import annotations
import os, subprocess
from norcode.commands.base import CommandModule


def _nc_vm():
    return os.path.join(os.path.dirname(__file__), "..", "..", "dist", "nc-vm")


def register_build(parser) -> None:
    parser.add_argument("file", help="Kildefil (.no)")
    parser.add_argument("output", help="Utdata (.elf)")
    parser.add_argument("--arch", default="aarch64")


def run_build(args) -> int:
    nc_vm = _nc_vm()
    if os.path.isfile(nc_vm) and os.access(nc_vm, os.X_OK):
        result = subprocess.run([nc_vm, "--nc-compile", args.file])
        return result.returncode
    print("nc build: nc-vm ikkje funne. Køyr: sh tools/bootstrap.sh")
    return 1


def _noop_register(parser) -> None:
    parser.add_argument("file", nargs="?")


def _noop_run(args) -> int:
    print("AVVIKLA: bruk bin/nc build i staden")
    return 2


BUILD_COMMAND = CommandModule(name="build", help="Kompiler til native binary", register_arguments=register_build, run=run_build)
NATIVE_BUILD_COMMAND = CommandModule(name="native-build", help="[avvikla]", register_arguments=_noop_register, run=_noop_run)
NATIVE_RUN_COMMAND = CommandModule(name="native-run", help="[avvikla]", register_arguments=_noop_register, run=_noop_run)
