"""Bytecode-run command module.

This command is the first explicit bytecode-first execution path in the modular
CLI. It can run a source file, AST JSON file, or precompiled bytecode JSON file
through the bytecode/runtime service stack.
"""

from __future__ import annotations

from norcode.bytecode_service import compile_ast_file_to_bytecode, compile_source_file_to_bytecode, load_bytecode_file
from norcode.commands.base import CommandModule
from norcode.runtime_service import RuntimeOptions, run_compiled_bytecode



def register_arguments(parser) -> None:
    parser.add_argument("file")
    parser.add_argument("--bytecode", action="store_true", help="Tolker input som .ncb.json i stedet for .no")
    parser.add_argument("--ast", action="store_true", help="Tolker input som .nast.json i stedet for .no")
    parser.add_argument("--trace", action="store_true", help="Vis VM-sporing")
    parser.add_argument("--max-steps", type=int, default=5_000_000, help="Maks VM-steg før kjøring avbrytes")



def run(args) -> int:
    if args.bytecode:
        bytecode = load_bytecode_file(args.file)
    elif args.ast:
        bytecode = compile_ast_file_to_bytecode(args.file)
    else:
        bytecode = compile_source_file_to_bytecode(args.file)

    options = RuntimeOptions(trace=args.trace, max_steps=args.max_steps)
    result = run_compiled_bytecode(bytecode, options=options)
    if result is not None:
        print(result)
    return 0


BYTECODE_RUN_COMMAND = CommandModule(
    name="bytecode-run",
    help="Kjør bytecode-backenden",
    register_arguments=register_arguments,
    run=run,
)
