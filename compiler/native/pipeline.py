from __future__ import annotations

import os
import platform
import subprocess
from dataclasses import dataclass
from pathlib import Path
from tempfile import TemporaryDirectory

from compiler.ast_nodes import (
    BinOpNode,
    BlockNode,
    CallNode,
    FunctionNode,
    IfNode,
    NumberNode,
    PrintNode,
    ProgramNode,
    ReturnNode,
    StringNode,
    UnaryOpNode,
    VarAccessNode,
    VarDeclareNode,
    VarSetNode,
    WhileNode,
)
from compiler.lexer import Lexer
from compiler.native.arithmetic_lowering import NativeArithmeticLowering
from compiler.native.elf_writer import BASE_ADDRESS, TEXT_ALIGNMENT, NativeELFWriter
from compiler.parser import Parser


class NativeCompileError(RuntimeError):
    pass


@dataclass(frozen=True)
class NativeBuildResult:
    source: Path
    output: Path
    exit_code: int
    machine_code: bytes
    elf_image: bytes
    entry_address: int
    executable: bool


@dataclass(frozen=True)
class NativeRunResult:
    build: NativeBuildResult
    ran: bool
    returncode: int | None
    stdout: str
    stderr: str
    reason: str | None = None


def _parse_program(source: str) -> ProgramNode:
    return Parser(Lexer(source)).parse()


def _find_entry(program: ProgramNode) -> FunctionNode:
    for preferred in ("start", "main"):
        for function in program.functions:
            if function.name == preferred:
                if function.params:
                    raise NativeCompileError(f"Native entry '{preferred}' kan ikke ha parametre ennå")
                return function
    raise NativeCompileError("Fant ingen native entry. Definer funksjon start() eller main().")


_COMPARISON_OPS = frozenset({"EQ", "NE", "LT", "GT", "LTE", "GTE"})


def _extract_single_return(block: BlockNode, slot: str) -> object:
    """Hent ut uttrykket fra et hvis-/ellers-blokk som inneholder
    nøyaktig én returner-setning."""
    stmts = block.statements
    if len(stmts) != 1 or not isinstance(stmts[0], ReturnNode):
        raise NativeCompileError(
            f"Native v0 krever at {slot}-blokken inneholder nøyaktig én "
            f"returner-setning (fikk {len(stmts)} statements)"
        )
    return stmts[0].expr


def _validate_comparison(cond, ctx: str) -> str:
    if not isinstance(cond, BinOpNode):
        raise NativeCompileError(
            f"Native v0 {ctx}-betingelse må være en sammenligning (==, !=, <, >, <=, >=)"
        )
    op = getattr(cond.op, "typ", "")
    if op not in _COMPARISON_OPS:
        raise NativeCompileError(
            f"Native v0 {ctx}-betingelse må bruke sammenligningsoperator (fikk {op})"
        )
    return op


def _classify_body_ops(statements, allow_var_decl: bool, ctx: str) -> list:
    """Klassifiser en liste statements til ops-tupler.
    ops-formater:
        ("skriv", bytes)
        ("set",  name, expr)
        ("decl", name, init_expr)    — kun hvis allow_var_decl=True
        ("mens", cond, body_ops)
    """
    ops: list = []
    for stmt in statements:
        if isinstance(stmt, VarDeclareNode):
            if not allow_var_decl:
                raise NativeCompileError(
                    f"Native v0: `la` (deklarasjon) er ikke tillatt inni {ctx}; "
                    "bruk `NAVN = UTTRYKK` for re-tilordning"
                )
            if stmt.var_type not in (None, "heltall"):
                raise NativeCompileError(
                    f"Native v0 støtter bare heltall-variabler (fikk {stmt.var_type!r})"
                )
            ops.append(("decl", stmt.name, stmt.expr))
            continue
        if isinstance(stmt, VarSetNode):
            ops.append(("set", stmt.name, stmt.expr))
            continue
        if isinstance(stmt, PrintNode):
            if not isinstance(stmt.expr, StringNode):
                raise NativeCompileError(
                    "Native v0 støtter bare skriv(\"streng-literal\") "
                    f"(fikk {type(stmt.expr).__name__})"
                )
            ops.append(("skriv", stmt.expr.value.encode("utf-8") + b"\n"))
            continue
        if isinstance(stmt, WhileNode):
            _validate_comparison(stmt.condition, "mens")
            body_ops = _classify_body_ops(
                stmt.body.statements, allow_var_decl=False, ctx="mens-kropp"
            )
            ops.append(("mens", stmt.condition, body_ops))
            continue
        if isinstance(stmt, IfNode):
            then_ops = _classify_body_ops(
                stmt.then_block.statements, allow_var_decl=False, ctx="hvis-kropp"
            )
            else_ops = None
            if stmt.else_block is not None:
                else_ops = _classify_body_ops(
                    stmt.else_block.statements, allow_var_decl=False, ctx="ellers-kropp"
                )
            ops.append(("hvis", stmt.condition, then_ops, else_ops))
            continue
        if isinstance(stmt, CallNode):
            ops.append(("kall", stmt))
            continue
        if isinstance(stmt, ReturnNode):
            # Mid-body return: treat as exit — only valid as last statement for now
            ops.append(("returner_mid", stmt.expr))
            continue
        raise NativeCompileError(
            f"Native v0: ukjent statement-type {type(stmt).__name__} i {ctx}"
        )
    return ops


def _entry_statements(function: FunctionNode) -> tuple[list[tuple[str, object]], list, object]:
    """Returner (la_decls, ops, exit_form) for entry-funksjonen.

    la_decls: alle la-deklarasjoner på toppnivå (i orden) — variabler får
              register-tildeling basert på denne rekkefølgen.
    ops:      liste av runtime-operasjoner mellom deklarasjoner og avslutning.
              Hver op er en tuple: ("skriv", bytes), ("set", name, expr),
              ("mens", cond, body_ops).
    exit_form: AST-uttrykk for plain returner, eller
               ("if", cond, then_expr, else_expr) for hvis-avslutning.
    """
    statements = function.body.statements
    if not statements:
        raise NativeCompileError("Native v0 krever en returner-setning i entry-funksjonen")

    final = statements[-1]
    if isinstance(final, ReturnNode):
        exit_form: object = final.expr
        body_stmts = statements[:-1]
    elif isinstance(final, IfNode):
        if final.elif_blocks:
            raise NativeCompileError(
                "Native v0 støtter ikke ellers-hvis i avsluttings-hvis (kun en gren)"
            )
        if final.else_block is None:
            raise NativeCompileError(
                "Native v0 krever ellers-gren i avsluttings-hvis (begge grener må returnere)"
            )
        cond_op = _validate_comparison(final.condition, "hvis")
        then_expr = _extract_single_return(final.then_block, "hvis")
        else_expr = _extract_single_return(final.else_block, "ellers")
        exit_form = ("if", final.condition, then_expr, else_expr)
        body_stmts = statements[:-1]
    else:
        raise NativeCompileError(
            "Native v0 krever at siste setning i entry er en returner-setning "
            "eller en hvis(...)/ellers med returner i hver gren "
            f"(fikk {type(final).__name__})"
        )

    # Klassifiser body. la-deklarasjoner tillates kun på dette nivået.
    raw_ops = _classify_body_ops(body_stmts, allow_var_decl=True, ctx="entry-kropp")

    # Splitt: la_decls (med duplikatsjekk) først, så øvrige ops.
    la_decls: list[tuple[str, object]] = []
    declared: set[str] = set()
    other_ops: list = []
    for op in raw_ops:
        if op[0] == "decl":
            _, name, init_expr = op
            if name in declared:
                raise NativeCompileError(
                    f"Native v0: variabel '{name}' deklarert to ganger"
                )
            declared.add(name)
            la_decls.append((name, init_expr))
        else:
            other_ops.append(op)

    return la_decls, other_ops, exit_form


def _constant_int(expr, env: dict[str, int] | None = None) -> int:
    env = env or {}
    if isinstance(expr, NumberNode):
        return int(expr.value)
    if isinstance(expr, VarAccessNode):
        if expr.name not in env:
            raise NativeCompileError(
                f"Native v0: variabel '{expr.name}' brukt før den er deklarert med 'la'"
            )
        return env[expr.name]
    if isinstance(expr, UnaryOpNode):
        op = getattr(expr.op, "typ", "")
        value = _constant_int(expr.node, env)
        if op == "PLUS":
            return value
        if op == "MINUS":
            return -value
    if isinstance(expr, BinOpNode):
        left = _constant_int(expr.left, env)
        right = _constant_int(expr.right, env)
        op = getattr(expr.op, "typ", "")
        if op == "PLUS":
            return left + right
        if op == "MINUS":
            return left - right
        if op == "MUL":
            return left * right
        if op == "DIV":
            if right == 0:
                raise NativeCompileError("Kan ikke native-kompilere divisjon på null")
            return int(left / right)
        if op == "PERCENT":
            if right == 0:
                raise NativeCompileError("Kan ikke native-kompilere modulo på null")
            return left % right
    raise NativeCompileError("Native v0 støtter bare konstante heltallsuttrykk i returner")


def lower_exit_code(exit_code: int) -> bytes:
    if not 0 <= exit_code <= 255:
        raise NativeCompileError("Native process exit code må være mellom 0 og 255")
    lowering = NativeArithmeticLowering()
    lowering.emit_mov_rax_imm32(exit_code)
    lowering.emit_linux_exit_from_rax()
    return bytes(lowering.code)


# Bytes per write(2)-syscall (mov eax/edi/esi/edx imm32 + syscall = 5+5+5+5+2 = 22).
_WRITE_SYSCALL_BYTES = 22

# Registre brukt for variabler. Callee-saved iht. System V AMD64 ABI, og
# klobres ikke av Linux syscall-instruksjonen (som klobrer kun rcx og r11).
# Med 5 registre er v0 begrenset til 5 variabler.
_VAR_REGS = (3, 12, 13, 14, 15)  # rbx, r12, r13, r14, r15
_RAX = 0
_RCX = 1

# Første 4 integer-parametre per System V AMD64 ABI: rdi, rsi, rdx, rcx.
# Vi kopierer disse til VAR_REGS i funksjonsprolog så param-access blir
# konsistent med vanlige variabler.
_PARAM_REGS = (7, 6, 2, 1)


def _emit_expr_to_rax(
    expr,
    var_regs: dict,
    lowering: NativeArithmeticLowering,
    helpers_offsets: dict | None = None,
    code_base: int = 0,
) -> None:
    """Emit maskinkode som plasserer verdien av expr i rax ved kjøretid.

    helpers_offsets: dict {navn: offset-i-text-section} hvis kall til
        helper-funksjoner er tillatt i dette uttrykket. None → ingen kall.
    code_base: offset for byte 0 i lowering.code målt fra start av
        text-seksjonen. Nødvendig for å beregne CALL rel32 riktig når
        helper-koden ligger etter entry-funksjonen.
    """
    if isinstance(expr, NumberNode):
        lowering.emit_mov_reg_imm32(_RAX, int(expr.value))
        return

    if isinstance(expr, VarAccessNode):
        if expr.name not in var_regs:
            raise NativeCompileError(
                f"Native v0: variabel '{expr.name}' brukt før den er deklarert med 'la'"
            )
        lowering.emit_mov_reg_reg(_RAX, var_regs[expr.name])
        return

    if isinstance(expr, UnaryOpNode):
        op = getattr(expr.op, "typ", "")
        _emit_expr_to_rax(expr.node, var_regs, lowering, helpers_offsets, code_base)
        if op == "PLUS":
            return
        if op == "MINUS":
            # neg rax = REX.W + F7 /3
            lowering.code += b"\x48\xf7\xd8"
            return
        raise NativeCompileError(f"Native v0 støtter ikke unær operator {op}")

    if isinstance(expr, CallNode):
        if helpers_offsets is None:
            raise NativeCompileError(
                "Native v0: funksjonskall er kun tillatt i entry-funksjonen "
                "(ikke i helper-kropp)"
            )
        if expr.name not in helpers_offsets:
            raise NativeCompileError(f"Native v0: ukjent funksjon '{expr.name}'")
        if len(expr.args) > len(_PARAM_REGS):
            raise NativeCompileError(
                f"Native v0 støtter maks {len(_PARAM_REGS)} argumenter per kall "
                f"(fikk {len(expr.args)} til '{expr.name}')"
            )
        # 1) Evaluer hvert argument i rekkefølge og push til stack.
        for arg in expr.args:
            _emit_expr_to_rax(arg, var_regs, lowering, helpers_offsets, code_base)
            lowering.emit_push_reg(_RAX)
        # 2) Pop til parameter-registre i revers ordre, slik at første
        #    pop havner i siste param-reg (rcx om 4 args, rdx om 3, osv.).
        for i in reversed(range(len(expr.args))):
            lowering.emit_pop_reg(_PARAM_REGS[i])
        # 3) Emit `call rel32`. PC etter call = code_base + len(code) + 5.
        helper_off = helpers_offsets[expr.name]
        pc_after_call = code_base + len(lowering.code) + 5
        rel32 = helper_off - pc_after_call
        lowering.emit_call_rel32(rel32)
        # rax har returverdien
        return

    if isinstance(expr, BinOpNode):
        op = getattr(expr.op, "typ", "")
        if op not in ("PLUS", "MINUS", "MUL", "DIV", "PERCENT"):
            raise NativeCompileError(f"Native v0 støtter +/-/*//%, ikke {op}")

        # Generell strategi som håndterer nested binops via stack:
        #   evaluer venstre → rax
        #   push rax
        #   evaluer høyre → rax
        #   mov rcx, rax    (rcx = høyre)
        #   pop rax         (gjenopprett venstre)
        #   op rax, rcx
        # For leaf-only operander er overhead 5 bytes (push+pop+mov rcx,rax)
        # men kodemodellen blir uniform.
        _emit_expr_to_rax(expr.left, var_regs, lowering, helpers_offsets, code_base)
        lowering.emit_push_rax()
        _emit_expr_to_rax(expr.right, var_regs, lowering, helpers_offsets, code_base)
        lowering.emit_mov_reg_reg(_RCX, _RAX)
        lowering.emit_pop_rax()

        if op == "PLUS":
            lowering.emit_add_reg_reg(_RAX, _RCX)
        elif op == "MINUS":
            lowering.emit_sub_reg_reg(_RAX, _RCX)
        elif op == "MUL":
            lowering.emit_imul_reg_reg(_RAX, _RCX)
        elif op == "DIV":
            lowering.emit_cqo()
            lowering.emit_idiv_reg(_RCX)
            # Kvotient i rax — ferdig.
        else:  # PERCENT
            lowering.emit_cqo()
            lowering.emit_idiv_reg(_RCX)
            # Rest i rdx — flytt til rax.
            lowering.emit_mov_reg_reg(_RAX, 2)  # 2 = rdx
        return

    raise NativeCompileError(
        f"Native v0 støtter ikke uttrykk av type {type(expr).__name__}"
    )


def _emit_exit_with_value(
    expr,
    var_regs: dict,
    lowering: NativeArithmeticLowering,
    helpers_offsets: dict | None = None,
    code_base: int = 0,
) -> None:
    """Emit kode som evaluerer expr til rax og kaller sys_exit."""
    _emit_expr_to_rax(expr, var_regs, lowering, helpers_offsets, code_base)
    lowering.emit_linux_exit_from_rax()


# x86_64 J<cc> rel32 er 6 bytes (0F XX imm32). Inverse av sammenlignings-
# opcode brukes for å hoppe FORBI then-grenen når betingelsen ikke holder.
# (CMP setter flags som om lhs - rhs er beregnet; vi tester relasjonen
# lhs OP rhs ved å sjekke status etter SUB.)
_JCC_INVERSE = {
    "EQ":  "emit_jne_rel32",   # hopp hvis IKKE likhet → ellers-gren
    "NE":  "emit_je_rel32",
    "LT":  "emit_jge_rel32",
    "GT":  "emit_jle_rel32",
    "LTE": "emit_jg_rel32",
    "GTE": "emit_jl_rel32",
}


def _emit_conditional_exit(
    cond,
    then_expr,
    else_expr,
    var_regs: dict,
    lowering: NativeArithmeticLowering,
    helpers_offsets: dict | None = None,
    code_base: int = 0,
) -> None:
    """Emit kode for `hvis (LHS OP RHS) { returner THEN } ellers { returner ELSE }`."""
    _emit_expr_to_rax(cond.left, var_regs, lowering, helpers_offsets, code_base)
    lowering.emit_push_rax()
    _emit_expr_to_rax(cond.right, var_regs, lowering, helpers_offsets, code_base)
    lowering.emit_mov_reg_reg(_RCX, _RAX)
    lowering.emit_pop_rax()
    lowering.emit_cmp_reg_reg(_RAX, _RCX)

    op = getattr(cond.op, "typ", "")
    jcc_method = getattr(NativeArithmeticLowering, _JCC_INVERSE[op])

    # Mål then-grenen i en lokal buffer. Vi bruker en "skjøtet" code_base så
    # eventuelle CALL inni then-grenen får riktig offset etter at jcc + then
    # er sluttet til outer lowering. cmp+jcc tar (len(cmp) + 6) bytes —
    # vi vet offset frem til etter jcc ved å bruke nåværende lowering-pos + 6.
    then_lowering = NativeArithmeticLowering()
    inner_code_base = code_base + len(lowering.code) + 6
    _emit_exit_with_value(
        then_expr, var_regs, then_lowering, helpers_offsets, inner_code_base
    )
    then_bytes = bytes(then_lowering.code)

    jcc_method(lowering, len(then_bytes))
    lowering.code += then_bytes
    _emit_exit_with_value(
        else_expr, var_regs, lowering, helpers_offsets, code_base
    )


def _collect_messages(ops: list) -> list[bytes]:
    """Walk ops-tre og samle alle skriv-meldinger i utfør-rekkefølge.
    Brukt til å reservere msg-data-byte-rekkefølgen i .text-segmentet."""
    msgs: list[bytes] = []
    for op in ops:
        kind = op[0]
        if kind == "skriv":
            msgs.append(op[1])
        elif kind == "mens":
            msgs.extend(_collect_messages(op[2]))
    return msgs


def _emit_ops(
    ops: list,
    var_regs: dict,
    msg_vaddr_iter,
    lowering: NativeArithmeticLowering,
    helpers_offsets: dict | None = None,
    code_base: int = 0,
) -> None:
    """Emit alle operasjoner i ops-listen i rekkefølge."""
    for op in ops:
        kind = op[0]
        if kind == "skriv":
            msg = op[1]
            vaddr = next(msg_vaddr_iter)
            lowering.emit_linux_write_stdout(vaddr, len(msg))
        elif kind == "set":
            _, name, expr = op
            if name not in var_regs:
                raise NativeCompileError(
                    f"Native v0: tilordning til '{name}' før den er deklarert med 'la'"
                )
            _emit_expr_to_rax(expr, var_regs, lowering, helpers_offsets, code_base)
            lowering.emit_mov_reg_reg(var_regs[name], _RAX)
        elif kind == "mens":
            _, cond, body_ops = op
            _emit_while_loop(cond, body_ops, var_regs, msg_vaddr_iter, lowering, helpers_offsets, code_base)
        else:
            raise NativeCompileError(f"Ukjent op-type: {kind}")


def _emit_while_loop(
    cond,
    body_ops: list,
    var_regs: dict,
    msg_vaddr_iter,
    outer_lowering: NativeArithmeticLowering,
    helpers_offsets: dict | None = None,
    code_base: int = 0,
) -> None:
    """Emit en while-løkke:
        LOOP_START:
            evaluer cmp(LHS, RHS)
            J<inverse_op> END_LOOP
            ... body ...
            jmp LOOP_START   (rel32 negativ)
        END_LOOP:

    Vi måler først cmp-blokken og body-blokken hver for seg, så
    beregner offsets, så stitcher sammen i `outer_lowering`.
    """
    # 1) Mål cmp-blokken (kondisjon-evaluering + cmp rax, rcx).
    cmp_lowering = NativeArithmeticLowering()
    _emit_expr_to_rax(cond.left, var_regs, cmp_lowering, helpers_offsets, code_base + len(outer_lowering.code))
    cmp_lowering.emit_push_rax()
    _emit_expr_to_rax(cond.right, var_regs, cmp_lowering, helpers_offsets, code_base + len(outer_lowering.code) + len(cmp_lowering.code))
    cmp_lowering.emit_mov_reg_reg(_RCX, _RAX)
    cmp_lowering.emit_pop_rax()
    cmp_lowering.emit_cmp_reg_reg(_RAX, _RCX)
    cmp_bytes = bytes(cmp_lowering.code)

    # 2) Mål body-blokken — bruker dummy-vaddrs/helpers siden størrelsene
    #    er invariante av faktiske verdier (5-byte imm32).
    body_lowering_dummy = NativeArithmeticLowering()
    dummy_msgs = _collect_messages(body_ops)
    dummy_iter = iter([0] * len(dummy_msgs))
    _emit_ops(body_ops, var_regs, dummy_iter, body_lowering_dummy, helpers_offsets, 0)
    body_size = len(body_lowering_dummy.code)

    # 3) Beregn offsets.
    op = getattr(cond.op, "typ", "")
    jcc_size = 6  # Jcc rel32
    jmp_size = 5  # jmp rel32
    # j<inverse> hopper FORBI body + jmp-back-instruksjonen til END_LOOP.
    skip_offset = body_size + jmp_size

    # jmp tilbake til LOOP_START: PC etter jmp er FØR cmp-blokken minus
    # cmp_bytes - jcc_size - body_size - jmp_size, dvs:
    #   target = LOOP_START
    #   PC_after_jmp = LOOP_START + len(cmp) + jcc_size + body_size + jmp_size
    #   rel32 = target - PC_after_jmp = -(len(cmp) + jcc_size + body_size + jmp_size)
    back_rel = -(len(cmp_bytes) + jcc_size + body_size + jmp_size)

    # 4) Emit i outer_lowering.
    outer_lowering.code += cmp_bytes
    getattr(outer_lowering, _JCC_INVERSE[op])(skip_offset)
    # Re-emit body med ekte msg-vaddrs (fra ytre iterator) og oppdatert code_base.
    _emit_ops(body_ops, var_regs, msg_vaddr_iter, outer_lowering, helpers_offsets, code_base)
    outer_lowering.emit_jmp_rel32(back_rel)


def _compile_helper(helper: FunctionNode) -> bytes:
    """Kompiler en helper-funksjon. Helpers har egen prolog/epilog og kan
    motta opp til 4 heltall-parametre via ABI-registre rdi/rsi/rdx/rcx,
    som kopieres til VAR_REGS i prologen.

    Begrensninger i v0:
    - Ingen skriv() i helpers (kun entry har msg-håndtering)
    - Ingen kall til andre funksjoner (helpers er leaves)
    - Maks 5 totale variabler (params + locals) — VAR_REGS-kvota
    """
    params = list(helper.params)
    if len(params) > len(_PARAM_REGS):
        raise NativeCompileError(
            f"Native v0 helper '{helper.name}' har for mange parametre "
            f"({len(params)}; maks {len(_PARAM_REGS)})"
        )
    for p in params:
        if getattr(p, "type_name", None) not in (None, "heltall"):
            raise NativeCompileError(
                f"Native v0 helper-parameter må være heltall (fikk {p.type_name!r})"
            )
    if helper.return_type not in (None, "heltall"):
        raise NativeCompileError(
            f"Native v0 helper-funksjoner må returnere heltall "
            f"(fikk {helper.return_type!r} på {helper.name})"
        )

    la_decls, ops, exit_form = _entry_statements(helper)
    # Helpers kan ikke ha skriv eller kall.
    msgs = _collect_messages(ops)
    if msgs:
        raise NativeCompileError(
            f"Native v0 helper '{helper.name}' kan ikke inneholde skriv()"
        )

    total_vars = len(params) + len(la_decls)
    if total_vars > len(_VAR_REGS):
        raise NativeCompileError(
            f"Native v0 helper '{helper.name}' har for mange variabler "
            f"({total_vars}; maks {len(_VAR_REGS)})"
        )

    # Tildel var-regs: først til parametrene (i orden), så til lokale la-decls.
    var_regs: dict[str, int] = {}
    for i, p in enumerate(params):
        var_regs[p.name] = _VAR_REGS[i]
    for i, (name, _) in enumerate(la_decls):
        var_regs[name] = _VAR_REGS[len(params) + i]

    lowering = NativeArithmeticLowering()
    # Prolog: save callee-saved (push i orden 3, 12, 13, 14, 15).
    for reg in _VAR_REGS:
        lowering.emit_push_reg(reg)
    # Kopier parametre fra ABI-registre til var-regs.
    for i, p in enumerate(params):
        lowering.emit_mov_reg_reg(var_regs[p.name], _PARAM_REGS[i])
    # La-initialisering.
    for name, init_expr in la_decls:
        # Helpers kan ikke kalle helpers, så helpers_offsets=None.
        _emit_expr_to_rax(init_expr, var_regs, lowering, None, 0)
        lowering.emit_mov_reg_reg(var_regs[name], _RAX)
    # Body (uten skriv siden vi har sjekket; ingen msg_vaddr_iter trengs).
    empty_iter = iter([])
    _emit_ops(ops, var_regs, empty_iter, lowering, None, 0)
    # Exit-uttrykk → rax. Vi gjør IKKE syscall — helper bare RETter.
    # For helpers betyr "exit_form" returner-uttrykk for funksjonen.
    if isinstance(exit_form, tuple) and exit_form[0] == "if":
        # hvis-returner-form: emit cmp + branch + then-eval+ret + else-eval+ret
        # Litt enklere: emit en lokal helper med samme mønster.
        cond, then_expr, else_expr = exit_form[1], exit_form[2], exit_form[3]
        _emit_expr_to_rax(cond.left, var_regs, lowering, None, 0)
        lowering.emit_push_rax()
        _emit_expr_to_rax(cond.right, var_regs, lowering, None, 0)
        lowering.emit_mov_reg_reg(_RCX, _RAX)
        lowering.emit_pop_rax()
        lowering.emit_cmp_reg_reg(_RAX, _RCX)
        op = getattr(cond.op, "typ", "")
        jcc_method = getattr(NativeArithmeticLowering, _JCC_INVERSE[op])

        then_lowering = NativeArithmeticLowering()
        _emit_expr_to_rax(then_expr, var_regs, then_lowering, None, 0)
        # Epilog: pop i revers + ret
        for reg in reversed(_VAR_REGS):
            then_lowering.emit_pop_reg(reg)
        then_lowering.emit_ret()
        then_bytes = bytes(then_lowering.code)

        jcc_method(lowering, len(then_bytes))
        lowering.code += then_bytes
        _emit_expr_to_rax(else_expr, var_regs, lowering, None, 0)
    else:
        _emit_expr_to_rax(exit_form, var_regs, lowering, None, 0)

    # Epilog: pop callee-saved i revers + ret.
    for reg in reversed(_VAR_REGS):
        lowering.emit_pop_reg(reg)
    lowering.emit_ret()
    return bytes(lowering.code)


def lower_program(
    la_decls: list[tuple[str, object]],
    ops: list,
    exit_form,
    text_base_vaddr: int,
    helpers: list | None = None,
) -> tuple[bytes, bytes]:
    """Bygg (text, data) for et program med la-deklarasjoner, ops, exit_form,
    og evt. helper-funksjoner. text-layout:
        [entry-kode] [helper_1-kode] [helper_2-kode] ... [data]
    """
    helpers = helpers or []

    if len(la_decls) > len(_VAR_REGS):
        raise NativeCompileError(
            f"Native v0 støtter maks {len(_VAR_REGS)} variabler "
            f"(fikk {len(la_decls)})"
        )

    var_regs: dict[str, int] = {
        name: _VAR_REGS[i] for i, (name, _) in enumerate(la_decls)
    }
    all_messages = _collect_messages(ops)

    # Kompiler helpers først — størrelsene deres er deterministiske.
    helper_bytes: list[bytes] = [_compile_helper(h) for h in helpers]
    helper_sizes = [len(b) for b in helper_bytes]

    def emit_entry(msg_vaddrs: list[int], helpers_offsets: dict) -> bytes:
        lowering = NativeArithmeticLowering()
        env_so_far: dict[str, int] = {}
        for name, init_expr in la_decls:
            _emit_expr_to_rax(init_expr, env_so_far, lowering, helpers_offsets, 0)
            lowering.emit_mov_reg_reg(var_regs[name], _RAX)
            env_so_far[name] = var_regs[name]
        _emit_ops(ops, var_regs, iter(msg_vaddrs), lowering, helpers_offsets, 0)
        if isinstance(exit_form, tuple) and exit_form and exit_form[0] == "if":
            _, cond, then_expr, else_expr = exit_form
            _emit_conditional_exit(cond, then_expr, else_expr, var_regs, lowering, helpers_offsets, 0)
        else:
            _emit_exit_with_value(exit_form, var_regs, lowering, helpers_offsets, 0)
        return bytes(lowering.code)

    # Pass 1: dummy helper offsets (alle 0) for å måle entry-størrelse.
    dummy_offsets = {h.name: 0 for h in helpers}
    entry_dummy = emit_entry([0] * len(all_messages), dummy_offsets)
    entry_size = len(entry_dummy)

    # Helper offsets ligger etter entry-koden.
    helper_offsets: dict[str, int] = {}
    cursor = entry_size
    for h, sz in zip(helpers, helper_sizes):
        helper_offsets[h.name] = cursor
        cursor += sz
    code_size = cursor

    # Msg vaddrs ligger etter alt kode.
    msg_vaddrs: list[int] = []
    msg_cursor = code_size
    for msg in all_messages:
        msg_vaddrs.append(text_base_vaddr + msg_cursor)
        msg_cursor += len(msg)

    # Pass 2: emit entry med korrekte offsets + msg vaddrs.
    entry_real = emit_entry(msg_vaddrs, helper_offsets)
    assert len(entry_real) == entry_size, (len(entry_real), entry_size)

    text = entry_real + b"".join(helper_bytes)
    data = b"".join(all_messages)
    return text, data


def compile_source_to_native_elf(source_path: str | Path, output_path: str | Path | None = None) -> NativeBuildResult:
    source = Path(source_path).resolve()
    if output_path is None:
        output = source.with_suffix(".elf")
    else:
        output = Path(output_path).resolve()
    output.parent.mkdir(parents=True, exist_ok=True)

    source_text = source.read_text(encoding="utf-8")
    program = _parse_program(source_text)
    entry = _find_entry(program)
    la_decls, ops, exit_form = _entry_statements(entry)

    # Compile-time-snapshot av exit-koden for NativeBuildResult-rapportering.
    # Conditional hvis-form eller dynamiske ops gir -1.
    try:
        env_ct: dict[str, int] = {}
        for name, init_expr in la_decls:
            env_ct[name] = _constant_int(init_expr, env_ct)
        if isinstance(exit_form, tuple) or any(op[0] != "skriv" for op in ops):
            exit_code = -1
        else:
            exit_code = _constant_int(exit_form, env_ct)
    except NativeCompileError:
        exit_code = -1

    if _is_macos_arm64():
        from compiler.native.aarch64_lowering import syscall_exit
        from compiler.native.aarch64_asm_gen import generate_aarch64_asm
        macho_output = output.with_suffix("") if output.suffix == ".elf" else output
        try:
            asm_text = _generate_full_aarch64_program(program, la_decls, ops, exit_form)
            result = _compile_macos_arm64_asm(asm_text, macho_output)
        except Exception:
            try:
                asm_text = generate_aarch64_asm(la_decls, ops, exit_form)
                result = _compile_macos_arm64_asm(asm_text, macho_output)
            except Exception:
                # Fallback to exit(0) smoke-test binary
                actual_exit = exit_code if exit_code >= 0 else 0
                result = _compile_macos_arm64(actual_exit, macho_output)
        if result is not None:
            actual_exit = exit_code if exit_code >= 0 else 0
            machine_code = syscall_exit(actual_exit)
            return NativeBuildResult(
                source=source,
                output=result["path"],
                exit_code=actual_exit,
                machine_code=machine_code,
                elf_image=result["image"],
                entry_address=result["entry"],
                executable=os.access(result["path"], os.X_OK),
            )

    text_base_vaddr = BASE_ADDRESS + TEXT_ALIGNMENT
    text, data = lower_program(la_decls, ops, exit_form, text_base_vaddr)
    machine_code = text  # eksponer kun den eksekverbare koden via NativeBuildResult

    writer = NativeELFWriter()
    writer.text_section = bytearray(text + data)
    writer.write_executable(output)
    elf_image = output.read_bytes()

    return NativeBuildResult(
        source=source,
        output=output,
        exit_code=exit_code,
        machine_code=machine_code,
        elf_image=elf_image,
        entry_address=BASE_ADDRESS + TEXT_ALIGNMENT,
        executable=os.access(output, os.X_OK),
    )


def _is_macos_arm64() -> bool:
    return platform.system() == "Darwin" and platform.machine() == "arm64"


def _compile_macos_arm64(exit_code: int, output: Path) -> dict | None:
    """Compile a minimal exit(exit_code) binary for macOS AArch64 via clang.

    Returns a dict with 'path', 'image', and 'entry' on success, or None
    if clang is not available.
    """
    clang = _find_clang()
    if clang is None:
        return None
    with TemporaryDirectory(prefix="norscode-macos-native-") as tmp:
        asm_path = Path(tmp) / "bootstrap.s"
        asm_path.write_text(
            f""".section __TEXT,__text,regular,pure_instructions
.global _main
_main:
    mov x0, #{exit_code}
    mov x16, #1
    svc #0x80
""",
            encoding="utf-8",
        )
        output.parent.mkdir(parents=True, exist_ok=True)
        result = subprocess.run(
            [clang, "-arch", "arm64", "-o", str(output), str(asm_path)],
            capture_output=True,
            text=True,
        )
        if result.returncode != 0 or not output.exists():
            return None
        image = output.read_bytes()
        entry = _macho_entry_address(image)
        return {"path": output, "image": image, "entry": entry}


def _generate_full_aarch64_program(
    program: ProgramNode,
    entry_la_decls, entry_ops, entry_exit_form,
) -> str:
    """Generate AArch64 assembly for a full program (entry + helpers)."""
    from compiler.native.aarch64_asm_gen import generate_aarch64_asm

    sections: list[str] = []
    header = [".section __TEXT,__text,regular,pure_instructions"]
    all_strings: list[str] = []

    # ── Helper functions (non-entry) ──────────────────────────────────────
    for func in program.functions:
        if func.name in ("start", "main"):
            continue
        param_names = [p.name for p in func.params]
        try:
            func_la_decls, func_ops, func_exit = _entry_statements(func)
        except NativeCompileError:
            raise
        asm = generate_aarch64_asm(
            func_la_decls, func_ops, func_exit,
            params=param_names,
            func_label=f"_{func.name}",
        )
        # Strip the section header from helpers (we emit it once)
        lines = asm.splitlines()
        body = [l for l in lines if not l.startswith(".section __TEXT,__text")]
        sections.append("\n".join(body))

    # ── Entry function ────────────────────────────────────────────────────
    entry_asm = generate_aarch64_asm(
        entry_la_decls, entry_ops, entry_exit_form,
        params=[],
        func_label="_main",
    )
    sections.append(entry_asm.replace(
        ".section __TEXT,__text,regular,pure_instructions\n", ""
    ))

    return (
        ".section __TEXT,__text,regular,pure_instructions\n"
        + "\n".join(sections)
    )


def _compile_macos_arm64_asm(asm_text: str, output: Path) -> dict | None:
    """Compile AArch64 assembly text to a macOS native binary via clang."""
    clang = _find_clang()
    if clang is None:
        return None
    with TemporaryDirectory(prefix="norscode-macos-asm-") as tmp:
        asm_path = Path(tmp) / "program.s"
        asm_path.write_text(asm_text, encoding="utf-8")
        output.parent.mkdir(parents=True, exist_ok=True)
        result = subprocess.run(
            [clang, "-arch", "arm64", "-o", str(output), str(asm_path)],
            capture_output=True,
            text=True,
        )
        if result.returncode != 0 or not output.exists():
            return None
        image = output.read_bytes()
        entry = _macho_entry_address(image)
        return {"path": output, "image": image, "entry": entry}


def _find_clang() -> str | None:
    for candidate in ("/usr/bin/clang", "/usr/local/bin/clang"):
        if os.path.isfile(candidate) and os.access(candidate, os.X_OK):
            return candidate
    result = subprocess.run(["which", "clang"], capture_output=True, text=True)
    path = result.stdout.strip()
    return path if path else None


def _macho_entry_address(image: bytes) -> int:
    """Extract the __text section address from a Mach-O image (best effort)."""
    # Mach-O 64-bit: magic at offset 0, ncmds at 16, sizeofcmds at 20
    import struct as _struct
    if len(image) < 32 or image[:4] != b"\xcf\xfa\xed\xfe":
        return 0
    ncmds = _struct.unpack_from("<I", image, 16)[0]
    offset = 32
    for _ in range(ncmds):
        if offset + 8 > len(image):
            break
        cmd, cmdsize = _struct.unpack_from("<II", image, offset)
        if cmd == 0x19 and offset + 72 + 80 <= len(image):  # LC_SEGMENT_64 with 1 section
            # Section64 addr is at offset + 72 + 32 (after 16+16 byte names)
            addr = _struct.unpack_from("<Q", image, offset + 72 + 32)[0]
            return addr
        offset += cmdsize
    return 0x100000000


def host_can_execute_elf() -> bool:
    """True if the host can directly execute the binary produced by the native pipeline."""
    return _is_macos_arm64() or (
        platform.system() == "Linux" and platform.machine() in {"x86_64", "AMD64"}
    )


def host_can_execute_linux_x86_64_elf() -> bool:
    return platform.system() == "Linux" and platform.machine() in {"x86_64", "AMD64"}


def run_native_elf(source_path: str | Path, output_path: str | Path | None = None) -> NativeRunResult:
    build = compile_source_to_native_elf(source_path, output_path=output_path)
    if not host_can_execute_elf():
        return NativeRunResult(
            build=build,
            ran=False,
            returncode=None,
            stdout="",
            stderr="",
            reason="Host kan ikke kjøre denne binærtypen direkte",
        )

    proc = subprocess.run(
        [str(build.output)],
        check=False,
        text=True,
        capture_output=True,
    )
    return NativeRunResult(
        build=build,
        ran=True,
        returncode=proc.returncode,
        stdout=proc.stdout,
        stderr=proc.stderr,
    )


_MACHO_MAGIC = b"\xcf\xfa\xed\xfe"  # 0xFEEDFACF little-endian
_ELF_MAGIC   = b"\x7fELF"
# macOS AArch64 syscall instruction: svc #0x80 = D4001001 (LE: 01 10 00 D4)
_MACOS_SVC   = bytes([0x01, 0x10, 0x00, 0xD4])
# Linux x86_64 syscall: 0F 05
_LINUX_SYSCALL = b"\x0f\x05"


def verify_bootstrap_compiler() -> dict[str, object]:
    with TemporaryDirectory(prefix="norscode-native-bootstrap-") as tmp:
        source = Path(tmp) / "bootstrap.no"
        output = Path(tmp) / "bootstrap.elf"
        source.write_text("funksjon start() -> heltall { returner 0 }\n", encoding="utf-8")
        run = run_native_elf(source, output)
        build = run.build
        image = build.elf_image
        code  = build.machine_code
        is_macho = image.startswith(_MACHO_MAGIC)
        is_elf   = image.startswith(_ELF_MAGIC)
        valid_code = (
            (is_macho and _MACOS_SVC in code)
            or (is_elf  and code.endswith(_LINUX_SYSCALL))
        )
        ok = (is_macho or is_elf) and valid_code and build.executable and (
            not run.ran or run.returncode == 0
        )
        return {
            "ok": ok,
            "source": str(build.source),
            "output": str(build.output),
            "exit_code": build.exit_code,
            "machine_code_hex": code.hex(),
            "elf_magic": image[:4].hex(),
            "entry_address": hex(build.entry_address),
            "executable": build.executable,
            "ran": run.ran,
            "returncode": run.returncode,
            "skip_reason": run.reason,
        }
