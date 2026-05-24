"""Static command registry used during CLI migration.

Today the authoritative argparse implementation still lives in `main.py`.
This registry is the first step toward a modular command system where each
command can later own:

- argument registration
- execution handler
- documentation metadata
- selfhost/runtime compatibility metadata
"""

from __future__ import annotations

from norcode.commands.ast_export_validate import AST_EXPORT_VALIDATE_COMMAND
from norcode.commands.ast_validate import AST_VALIDATE_COMMAND
from norcode.commands.base import CommandModule
from norcode.commands.build_bytecode import BUILD_BYTECODE_COMMAND
from norcode.commands.bytecode_run import BYTECODE_RUN_COMMAND
from norcode.commands.check import CHECK_COMMAND
from norcode.commands.lexer_parity_check import LEXER_PARITY_CHECK_COMMAND
from norcode.commands.lexer_parity_fixture import LEXER_PARITY_FIXTURE_COMMAND
from norcode.commands.lexer_parity_suite import LEXER_PARITY_SUITE_COMMAND
from norcode.commands.parser_parity_fixture import PARSER_PARITY_FIXTURE_COMMAND
from norcode.commands.run import RUN_COMMAND
from norcode.commands.runtime_call import RUNTIME_CALL_COMMAND
from norcode.commands.selfhost_lexer_compile_check import SELFHOST_LEXER_COMPILE_CHECK_COMMAND
from norcode.commands.selfhost_lexer_list_smoke import SELFHOST_LEXER_LIST_SMOKE_COMMAND
from norcode.commands.selfhost_lexer_parity import SELFHOST_LEXER_PARITY_COMMAND
from norcode.commands.selfhost_lexer_run import SELFHOST_LEXER_RUN_COMMAND
from norcode.commands.selfhost_lexer_status import SELFHOST_LEXER_STATUS_COMMAND
from norcode.commands.selfhost_lexer_suite import SELFHOST_LEXER_SUITE_COMMAND
from norcode.commands.selfhost_lexer_token_smoke import SELFHOST_LEXER_TOKEN_SMOKE_COMMAND
from norcode.commands.ui_render import UI_RENDER_COMMAND
from norcode.commands.test import TEST_COMMAND
from norcode.commands.token_validate import TOKEN_VALIDATE_COMMAND


COMMANDS: tuple[CommandModule, ...] = (
    RUN_COMMAND,
    CHECK_COMMAND,
    TEST_COMMAND,
    BYTECODE_RUN_COMMAND,
    BUILD_BYTECODE_COMMAND,
    AST_VALIDATE_COMMAND,
    AST_EXPORT_VALIDATE_COMMAND,
    PARSER_PARITY_FIXTURE_COMMAND,
    LEXER_PARITY_FIXTURE_COMMAND,
    LEXER_PARITY_CHECK_COMMAND,
    LEXER_PARITY_SUITE_COMMAND,
    RUNTIME_CALL_COMMAND,
    SELFHOST_LEXER_STATUS_COMMAND,
    SELFHOST_LEXER_COMPILE_CHECK_COMMAND,
    SELFHOST_LEXER_TOKEN_SMOKE_COMMAND,
    SELFHOST_LEXER_LIST_SMOKE_COMMAND,
    SELFHOST_LEXER_RUN_COMMAND,
    SELFHOST_LEXER_PARITY_COMMAND,
    SELFHOST_LEXER_SUITE_COMMAND,
    TOKEN_VALIDATE_COMMAND,
    UI_RENDER_COMMAND,
    CommandModule(
        name="repl",
        help="Start en enkel interaktiv Norscode-REPL",
        register_arguments=lambda parser: None,
    ),
    CommandModule(
        name="build",
        help="Generer C og bygg kjørbar fil",
        register_arguments=lambda parser: parser.add_argument("file"),
        bootstrap_only=True,
    ),
    CommandModule(
        name="native-build",
        help="Bygg ekte Linux x86_64 ELF fra enkel Norscode-entry",
        register_arguments=lambda parser: (
            parser.add_argument("file"),
            parser.add_argument("--output", "-o"),
            parser.add_argument("--json", action="store_true"),
        ),
        bootstrap_only=True,
    ),
    CommandModule(
        name="native-run",
        help="Bygg og kjør ekte Linux x86_64 ELF når host støtter det",
        register_arguments=lambda parser: (
            parser.add_argument("file"),
            parser.add_argument("--output", "-o"),
            parser.add_argument("--json", action="store_true"),
        ),
        bootstrap_only=True,
    ),
    CommandModule(
        name="bootstrap-compiler-verify",
        help="Verifiser real bootstrap compiler native lane",
        register_arguments=lambda parser: parser.add_argument("--json", action="store_true"),
        bootstrap_only=True,
    ),
    CommandModule(
        name="selfhost-bootstrap-gate",
        help="Kjør whole selfhost compile + native bootstrap gate",
        register_arguments=lambda parser: (
            parser.add_argument("--output-dir", default="build/selfhost-bootstrap-gate"),
            parser.add_argument("--no-determinism", action="store_true"),
            parser.add_argument("--json", action="store_true"),
        ),
        bootstrap_only=True,
        experimental=True,
    ),
    CommandModule(
        name="registry-host",
        help="Host registry-speilfil over HTTP",
        register_arguments=lambda parser: (
            parser.add_argument("--host", default="127.0.0.1"),
            parser.add_argument("--port", type=int, default=8765),
            parser.add_argument("--mirror"),
            parser.add_argument("--once", action="store_true"),
            parser.add_argument("--json", action="store_true"),
        ),
        bootstrap_only=True,
    ),
    CommandModule(
        name="ci",
        help="Kjør lokal CI-sekvens (snapshot, parity, test)",
        register_arguments=lambda parser: (
            parser.add_argument("--json", action="store_true"),
            parser.add_argument("--check-names", action="store_true"),
            parser.add_argument("--parity-suite", choices=["m1", "m2", "all"], default="all"),
            parser.add_argument("--bootstrap-lane", action="store_true"),
            parser.add_argument("--bootstrap-output-dir", default="build/selfhost-bootstrap-gate"),
            parser.add_argument("--require-selfhost-ready", action="store_true"),
        ),
        bootstrap_only=True,
    ),
    CommandModule(
        name="selfhost-chain-run",
        help="Kjør full selfhost-kjede",
        register_arguments=lambda parser: parser.add_argument("file"),
        experimental=True,
    ),
    CommandModule(
        name="selfhost-compile-all",
        help="Kompiler hele Norscode-koden med selfhost compiler-broen",
        register_arguments=lambda parser: (
            parser.add_argument("--root", action="append", dest="roots"),
            parser.add_argument("--output-dir", default="build/selfhost-whole"),
            parser.add_argument("--fail-fast", action="store_true"),
            parser.add_argument("--json", action="store_true"),
        ),
        bootstrap_only=True,
        experimental=True,
    ),
    CommandModule(
        name="serve",
        help="Start en lokal webserver for en Norscode-app",
        register_arguments=lambda parser: parser.add_argument("file"),
    ),
)



def command_names() -> list[str]:
    return sorted(command.name for command in COMMANDS)



def command_overview() -> list[dict[str, object]]:
    return [
        {
            "name": command.name,
            "help": command.help,
            "bootstrap_only": command.bootstrap_only,
            "experimental": command.experimental,
        }
        for command in sorted(COMMANDS, key=lambda item: item.name)
    ]
