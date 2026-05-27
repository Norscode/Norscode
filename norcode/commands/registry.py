"""Static command registry for the modular CLI.

This registry is the shared source of truth for command metadata and parser
registration.  Each command can own:

- argument registration
- execution handler
- documentation metadata
- selfhost/runtime compatibility metadata
"""

from __future__ import annotations

from norcode.commands.ast_export_validate import AST_EXPORT_VALIDATE_COMMAND
from norcode.commands.bootstrap_runtime import (
    BOOTSTRAP_COMPILER_VERIFY_COMMAND,
    REGISTRY_HOST_COMMAND,
    SELFHOST_CHAIN_RUN_COMMAND,
    SELFHOST_COMPILE_ALL_COMMAND,
)
from norcode.commands.build_targets import BUILD_COMMAND, NATIVE_BUILD_COMMAND, NATIVE_RUN_COMMAND
from norcode.commands.inspection import DEBUG_COMMAND, DISASM_COMMAND, IR_DISASM_COMMAND, UPDATE_SNAPSHOTS_COMMAND
from norcode.commands.ast_validate import AST_VALIDATE_COMMAND
from norcode.commands.base import CommandModule
from norcode.commands.build_bytecode import BUILD_BYTECODE_COMMAND
from norcode.commands.doctor import DOCTOR_COMMAND
from norcode.commands.bytecode_run import BYTECODE_RUN_COMMAND
from norcode.commands.check import CHECK_COMMAND
from norcode.commands.diagnose import DIAGNOSE_COMMAND
from norcode.commands.lexer_parity_check import LEXER_PARITY_CHECK_COMMAND
from norcode.commands.lexer_parity_fixture import LEXER_PARITY_FIXTURE_COMMAND
from norcode.commands.lexer_parity_suite import LEXER_PARITY_SUITE_COMMAND
from norcode.commands.parser_parity_fixture import PARSER_PARITY_FIXTURE_COMMAND
from norcode.commands.lint import LINT_COMMAND
from norcode.commands.release import RELEASE_COMMAND
from norcode.commands.run import RUN_COMMAND
from norcode.commands.runtime_call import RUNTIME_CALL_COMMAND
from norcode.commands.format import FORMAT_COMMAND
from norcode.commands.bench import BENCH_COMMAND
from norcode.commands.ci import CI_COMMAND
from norcode.commands.fuzz import FUZZ_COMMAND
from norcode.commands.migrate_names import MIGRATE_NAMES_COMMAND
from norcode.commands.package_registry_commands import (
    ADD_COMMAND,
    LOCK_COMMAND,
    REGISTRY_MIRROR_COMMAND,
    REGISTRY_SIGN_COMMAND,
    REGISTRY_SYNC_COMMAND,
    UPDATE_COMMAND,
)
from norcode.commands.meta_commands import COMMANDS_COMMAND, REPL_COMMAND
from norcode.commands.selfhost_artifacts import (
    AST_EXPORT_COMMAND,
    SELFHOST_AST_EXPORT_COMMAND,
    SELFHOST_CHAIN_CHECK_COMMAND,
    SELFHOST_CHAIN_EXPORT_COMMAND,
    SELFHOST_NCB_BUILD_CACHE_COMMAND,
    SELFHOST_NCB_EXPORT_COMMAND,
    SELFHOST_NCB_RUN_COMMAND,
)
from norcode.commands.scaffold_api import SCAFFOLD_API_COMMAND
from norcode.commands.smoke import SMOKE_COMMAND
from norcode.commands.serve_e2e import SERVE_E2E_COMMAND
from norcode.commands.stress import STRESS_COMMAND
from norcode.commands.security import SECURITY_COMMAND
from norcode.commands.selfhost_lexer_compile_check import SELFHOST_LEXER_COMPILE_CHECK_COMMAND
from norcode.commands.selfhost_lexer_list_smoke import SELFHOST_LEXER_LIST_SMOKE_COMMAND
from norcode.commands.selfhost_lexer_parity import SELFHOST_LEXER_PARITY_COMMAND
from norcode.commands.selfhost_lexer_run import SELFHOST_LEXER_RUN_COMMAND
from norcode.commands.selfhost_lexer_status import SELFHOST_LEXER_STATUS_COMMAND
from norcode.commands.selfhost_lexer_suite import SELFHOST_LEXER_SUITE_COMMAND
from norcode.commands.selfhost_lexer_token_smoke import SELFHOST_LEXER_TOKEN_SMOKE_COMMAND
from norcode.commands.selfhost_bootstrap_gate import SELFHOST_BOOTSTRAP_GATE_COMMAND
from norcode.commands.selfhost_parity import (
    SELFHOST_PARITY_COMMAND,
    SELFHOST_PARITY_CONSISTENCY_COMMAND,
    SELFHOST_PARITY_GATE_COMMAND,
    SELFHOST_PARITY_PROGRESS_COMMAND,
)
from norcode.commands.selfhost_parity_fixtures import (
    SYNC_SELFHOST_PARITY_M2_COMMAND,
    UPDATE_SELFHOST_PARITY_FIXTURES_COMMAND,
)
from norcode.commands.selfhost_parser_suite import SELFHOST_PARSER_SUITE_COMMAND
from norcode.commands.selfhost_bytecode_suite import SELFHOST_BYTECODE_SUITE_COMMAND
from norcode.commands.selfhost_bootstrap_build import SELFHOST_BOOTSTRAP_BUILD_COMMAND
from norcode.commands.selfhost_bootstrap_check import SELFHOST_BOOTSTRAP_CHECK_COMMAND
from norcode.commands.selfhost_semantic_suite import SELFHOST_SEMANTIC_SUITE_COMMAND
from norcode.commands.selfhost_stdlib_suite import SELFHOST_STDLIB_SUITE_COMMAND
from norcode.commands.serve import SERVE_COMMAND
from norcode.commands.ui_render import UI_RENDER_COMMAND
from norcode.commands.test import TEST_COMMAND
from norcode.commands.token_validate import TOKEN_VALIDATE_COMMAND


COMMANDS: tuple[CommandModule, ...] = (
    RUN_COMMAND,
    CHECK_COMMAND,
    TEST_COMMAND,
    DEBUG_COMMAND,
    DISASM_COMMAND,
    IR_DISASM_COMMAND,
    UPDATE_SNAPSHOTS_COMMAND,
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
    SELFHOST_PARSER_SUITE_COMMAND,
    SELFHOST_BYTECODE_SUITE_COMMAND,
    SELFHOST_BOOTSTRAP_BUILD_COMMAND,
    SELFHOST_BOOTSTRAP_CHECK_COMMAND,
    SELFHOST_SEMANTIC_SUITE_COMMAND,
    SELFHOST_STDLIB_SUITE_COMMAND,
    SELFHOST_AST_EXPORT_COMMAND,
    AST_EXPORT_COMMAND,
    SELFHOST_CHAIN_EXPORT_COMMAND,
    SELFHOST_CHAIN_CHECK_COMMAND,
    SELFHOST_NCB_EXPORT_COMMAND,
    SELFHOST_NCB_RUN_COMMAND,
    SELFHOST_NCB_BUILD_CACHE_COMMAND,
    FORMAT_COMMAND,
    LINT_COMMAND,
    SCAFFOLD_API_COMMAND,
    FUZZ_COMMAND,
    RELEASE_COMMAND,
    SMOKE_COMMAND,
    BENCH_COMMAND,
    SERVE_E2E_COMMAND,
    STRESS_COMMAND,
    SECURITY_COMMAND,
    DOCTOR_COMMAND,
    DIAGNOSE_COMMAND,
    MIGRATE_NAMES_COMMAND,
    ADD_COMMAND,
    LOCK_COMMAND,
    UPDATE_COMMAND,
    REGISTRY_SIGN_COMMAND,
    REGISTRY_SYNC_COMMAND,
    REGISTRY_MIRROR_COMMAND,
    UI_RENDER_COMMAND,
    REPL_COMMAND,
    BUILD_COMMAND,
    NATIVE_BUILD_COMMAND,
    NATIVE_RUN_COMMAND,
    BOOTSTRAP_COMPILER_VERIFY_COMMAND,
    REGISTRY_HOST_COMMAND,
    CI_COMMAND,
    SELFHOST_BOOTSTRAP_GATE_COMMAND,
    UPDATE_SELFHOST_PARITY_FIXTURES_COMMAND,
    SYNC_SELFHOST_PARITY_M2_COMMAND,
    SELFHOST_PARITY_COMMAND,
    SELFHOST_PARITY_PROGRESS_COMMAND,
    SELFHOST_PARITY_GATE_COMMAND,
    SELFHOST_PARITY_CONSISTENCY_COMMAND,
    SELFHOST_CHAIN_RUN_COMMAND,
    SELFHOST_COMPILE_ALL_COMMAND,
    SERVE_COMMAND,
    COMMANDS_COMMAND,
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
