"""Build-bytecode command module.

This command produces portable `.ncb.json` bytecode artifacts without going
through the legacy C backend.
"""

from __future__ import annotations

from norcode.commands.base import CommandModule
from norcode.compiler_core import build_bytecode_file



def register_arguments(parser) -> None:
    parser.add_argument("file")
    parser.add_argument("-o", "--output", help="Output .ncb.json file")



def run(args) -> int:
    result = build_bytecode_file(args.file, output=args.output)
    print(f"Bytecode skrevet til: {result.output_path}")
    return 0


BUILD_BYTECODE_COMMAND = CommandModule(
    name="bytecode-build",
    help="Bygg .ncb.json bytecode-artifact",
    register_arguments=register_arguments,
    run=run,
)
