"""Generate AArch64 assembly text from the native pipeline IR.

Supports: la variables, while loops, if/else, integer arithmetic,
function parameters (up to 8 via x0-x7), function calls, and
skriv() string output.  Uses symbolic labels — no raw-byte offsets.

Register convention:
  x0        — accumulator (and first argument / return value)
  x1–x7     — additional argument registers (spilled after call)
  x1, x2    — scratch during expression evaluation
  x19–x23   — variable/parameter slots (callee-saved)
  x29/x30   — frame pointer / link register
"""
from __future__ import annotations

from compiler.ast_nodes import (
    BinOpNode,
    CallNode,
    NumberNode,
    StringNode,
    UnaryOpNode,
    VarAccessNode,
)

# ── Register allocation ──────────────────────────────────────────────────────
# Variable / parameter slots use callee-saved registers so they survive calls.
_VAR_REGS = ["x19", "x20", "x21", "x22", "x23", "x24", "x25", "x26"]
_MAX_VARS  = len(_VAR_REGS)

# AArch64 argument registers for outgoing calls
_ARG_REGS  = ["x0", "x1", "x2", "x3", "x4", "x5", "x6", "x7"]

# ── Condition codes ──────────────────────────────────────────────────────────
_INVERSE_CC: dict[str, str] = {
    "EQ": "ne", "NE": "eq",
    "LT": "ge", "LTE": "gt",
    "GT": "le", "GTE": "lt",
}
_DIRECT_CC: dict[str, str] = {
    "EQ": "eq", "NE": "ne",
    "LT": "lt", "LTE": "le",
    "GT": "gt", "GTE": "ge",
}


class _AsmBuilder:
    _label_counter = 0

    def __init__(self) -> None:
        self._lines: list[str] = []
        self._strings: list[bytes] = []

    def _label(self) -> str:
        _AsmBuilder._label_counter += 1
        return f".L{_AsmBuilder._label_counter}"

    def op(self, instr: str) -> None:
        self._lines.append(f"    {instr}")

    def lbl(self, name: str) -> None:
        self._lines.append(f"{name}:")

    def add_string(self, data: bytes) -> str:
        idx = len(self._strings)
        self._strings.append(data)
        return f"_Lmsg_{id(self)}_{idx}"

    # ── Load immediate ───────────────────────────────────────────────────────

    def load_imm(self, reg: str, value: int) -> None:
        v = int(value) & 0xFFFF_FFFF_FFFF_FFFF
        lo = v & 0xFFFF
        self.op(f"movz {reg}, #{lo}")
        for shift in (16, 32, 48):
            part = (v >> shift) & 0xFFFF
            if part:
                self.op(f"movk {reg}, #{part}, lsl #{shift}")

    # ── Expression → x0 ─────────────────────────────────────────────────────

    def emit_expr(self, node, var_slots: dict[str, str]) -> None:
        if isinstance(node, NumberNode):
            self.load_imm("x0", int(node.value))
            return

        if isinstance(node, StringNode):
            # String literals: pass as a pointer to a cstring label
            lbl = self.add_string((node.value + "\n").encode("utf-8"))
            self.op(f"adrp x0, {lbl}@PAGE")
            self.op(f"add x0, x0, {lbl}@PAGEOFF")
            return

        if isinstance(node, VarAccessNode):
            reg = var_slots.get(node.name)
            if reg is None:
                raise ValueError(f"Ukjent variabel: {node.name}")
            if reg != "x0":
                self.op(f"mov x0, {reg}")
            return

        if isinstance(node, CallNode):
            # Push all live variable registers (callee-saved; we save them anyway
            # in the prologue, but nested calls in expressions need the saved values)
            args = node.args if hasattr(node, "args") else []
            # Evaluate arguments left-to-right, stage in temp saves
            staged: list[str] = []
            for i, arg in enumerate(args[:8]):
                self.emit_expr(arg, var_slots)
                self.op("str x0, [sp, #-16]!")
                staged.append(f"[sp+{(len(staged))*16}]")
            # Pop args into argument registers in reverse
            for i in range(len(staged) - 1, -1, -1):
                self.op(f"ldr {_ARG_REGS[i]}, [sp], #16")
            callee = node.name if hasattr(node, "name") else str(node)
            self.op(f"bl _{callee}")
            # Result is in x0
            return

        if isinstance(node, UnaryOpNode):
            op = getattr(node.op, "typ", str(node.op))
            self.emit_expr(node.node, var_slots)
            if op == "MINUS":
                self.op("neg x0, x0")
            elif op == "IKKE":
                # logisk ikke: 0 → 1, ikke-null → 0
                self.op("cmp x0, #0")
                self.op("cset x0, eq")
            else:
                raise ValueError(f"Ukjent unær operator: {op}")
            return

        if isinstance(node, BinOpNode):
            op = getattr(node.op, "typ", "")
            self.emit_expr(node.left, var_slots)
            self.op("str x0, [sp, #-16]!")    # push left
            self.emit_expr(node.right, var_slots)
            self.op("mov x1, x0")              # right → x1
            self.op("ldr x0, [sp], #16")       # pop left → x0
            if op == "PLUS":
                self.op("add x0, x0, x1")
            elif op == "MINUS":
                self.op("sub x0, x0, x1")
            elif op == "MUL":
                self.op("mul x0, x0, x1")
            elif op == "DIV":
                self.op("sdiv x0, x0, x1")
            elif op == "PERCENT":
                self.op("sdiv x2, x0, x1")
                self.op("mul x2, x2, x1")
                self.op("sub x0, x0, x2")
            elif op in _DIRECT_CC:
                self.op("cmp x0, x1")
                true_lbl = self._label()
                end_lbl  = self._label()
                self.op(f"b.{_DIRECT_CC[op]} {true_lbl}")
                self.op("movz x0, #0")
                self.op(f"b {end_lbl}")
                self.lbl(true_lbl)
                self.op("movz x0, #1")
                self.lbl(end_lbl)
            else:
                raise ValueError(f"Ukjent binæroperator: {op}")
            return

        raise ValueError(f"Ukjent uttrykksnode: {type(node).__name__}")

    # ── Comparison: emit cmp, return condition string ────────────────────────

    def emit_cmp(self, cond, var_slots: dict[str, str]) -> str:
        op = getattr(getattr(cond, "op", None), "typ", "")
        self.emit_expr(cond.left, var_slots)
        self.op("str x0, [sp, #-16]!")
        self.emit_expr(cond.right, var_slots)
        self.op("mov x1, x0")
        self.op("ldr x0, [sp], #16")
        self.op("cmp x0, x1")
        return _DIRECT_CC.get(op, "ne")

    # ── Statements ───────────────────────────────────────────────────────────

    # Set by generate_aarch64_asm to the label helper functions jump to on early return
    _epilogue_label: str | None = None

    def emit_ops(self, ops: list, var_slots: dict[str, str]) -> None:
        for op in ops:
            kind = op[0]
            if kind == "skriv":
                msg_bytes = op[1]
                lbl = self.add_string(msg_bytes)
                self.op("movz x0, #1")
                self.op(f"adrp x1, {lbl}@PAGE")
                self.op(f"add x1, x1, {lbl}@PAGEOFF")
                self.load_imm("x2", len(msg_bytes))
                self.op("mov x16, #4")
                self.op("svc #0x80")

            elif kind in ("set", "decl"):
                _, name, expr = op
                self.emit_expr(expr, var_slots)
                reg = var_slots[name]
                if reg != "x0":
                    self.op(f"mov {reg}, x0")

            elif kind == "mens":
                _, cond, body_ops = op
                loop_top = self._label()
                loop_end = self._label()
                self.lbl(loop_top)
                op_typ = getattr(getattr(cond, "op", None), "typ", "")
                self.emit_cmp(cond, var_slots)
                inverse = _INVERSE_CC.get(op_typ, "ne")
                self.op(f"b.{inverse} {loop_end}")
                self.emit_ops(body_ops, var_slots)
                self.op(f"b {loop_top}")
                self.lbl(loop_end)

            elif kind == "hvis":
                # ("hvis", cond, then_ops, else_ops_or_None)
                _, cond, then_ops, else_ops = op
                else_lbl = self._label()
                end_lbl  = self._label()
                op_typ = getattr(getattr(cond, "op", None), "typ", "")
                self.emit_cmp(cond, var_slots)
                inverse = _INVERSE_CC.get(op_typ, "ne")
                self.op(f"b.{inverse} {else_lbl}")
                self.emit_ops(then_ops, var_slots)
                self.op(f"b {end_lbl}")
                self.lbl(else_lbl)
                if else_ops:
                    self.emit_ops(else_ops, var_slots)
                self.lbl(end_lbl)

            elif kind == "skriv_expr":
                # Evaluer uttrykket → x0, kall deretter _nc_print_int
                _, expr = op
                self.emit_expr(expr, var_slots)
                self.op("bl _nc_print_int")

            elif kind == "skriv_tekst_var":
                # Tekst-variabel: last pekeren fra register → x0, kall _nc_skriv_tekst
                _, name = op
                reg = var_slots.get(name)
                if reg is None:
                    raise ValueError(f"Ukjent tekst-variabel: {name}")
                if reg != "x0":
                    self.op(f"mov x0, {reg}")
                self.op("bl _nc_skriv_tekst")

            elif kind == "kall":
                _, call_node = op
                self.emit_expr(call_node, var_slots)  # result discarded

            elif kind == "returner_mid":
                # Early return inside a block (e.g. inside if).
                # Put value in x0, then jump to the epilogue.
                _, expr = op
                self.emit_expr(expr, var_slots)
                if self._epilogue_label:
                    self.op(f"b {self._epilogue_label}")
                # If no epilogue label (entry/main path), the caller must handle this

            else:
                raise ValueError(f"Ukjent op: {kind!r}")

    # ── Assembly output ──────────────────────────────────────────────────────

    def prologue(self, used_vars: int) -> list[str]:
        lines = [
            "    stp x29, x30, [sp, #-16]!",
            "    mov x29, sp",
        ]
        regs = _VAR_REGS[:used_vars]
        for i in range(0, len(regs), 2):
            pair = regs[i:i+2]
            if len(pair) == 2:
                lines.append(f"    stp {pair[0]}, {pair[1]}, [sp, #-16]!")
            else:
                lines.append(f"    str {pair[0]}, [sp, #-16]!")
        return lines

    def epilogue(self, used_vars: int) -> list[str]:
        lines = []
        regs = _VAR_REGS[:used_vars]
        for i in range(len(regs) - 1, -1, -2):
            pair = regs[max(0, i-1):i+1]
            if len(pair) == 2:
                lines.append(f"    ldp {pair[0]}, {pair[1]}, [sp], #16")
            else:
                lines.append(f"    ldr {pair[0]}, [sp], #16")
        lines.append("    ldp x29, x30, [sp], #16")
        return lines

    def data_section(self) -> list[str]:
        if not self._strings:
            return []
        lines = ["", ".section __TEXT,__cstring,cstring_literals"]
        for i, (lbl_id, raw) in enumerate(
            zip([self.add_string(b"") for _ in range(0)], self._strings)
        ):
            pass
        # Re-enumerate using the actual labels stored
        seen = {}
        for raw in self._strings:
            key = id(raw)
            if key not in seen:
                seen[key] = raw
        # Use the labels as recorded in add_string
        label_base = f"_Lmsg_{id(self)}_"
        for i, raw in enumerate(self._strings):
            lbl = f"{label_base}{i}"
            escaped = "".join(
                f"\\{b:03o}" if (b < 0x20 or b > 0x7E) else chr(b)
                for b in raw
            )
            lines.append(f"{lbl}:")
            lines.append(f'    .asciz "{escaped}"')  # null-avsluttet → hindrer linker-merging
        return lines

    def text_lines(self) -> list[str]:
        return self._lines


def nc_skriv_tekst_helper_asm() -> str:
    """Returner AArch64-assembly for _nc_skriv_tekst (macOS/AArch64).

    Konvensjon: x0 = peker til null-avsluttet streng (inkl. \\n).
    Beregner lengde med inline strlen, skriver til stdout.
    Bevarer alle callee-saved registre; ødelegger kun x0–x5, x16.
    Stack-ramme: 16 bytes (bare x29/x30).
    """
    return """\
_nc_skriv_tekst:
    stp x29, x30, [sp, #-16]!
    mov x29, sp
    mov x1, x0
    mov x2, #0
.Lst_loop:
    ldrb w3, [x1, x2]
    cbz w3, .Lst_done
    add x2, x2, #1
    b .Lst_loop
.Lst_done:
    mov x0, #1
    mov x16, #4
    svc #0x80
    ldp x29, x30, [sp], #16
    ret
"""


def _get_op_typ(cond) -> str:
    return getattr(getattr(cond, "op", None), "typ", "")


# ── Runtime-hjelper: skriv heltall til stdout ────────────────────────────────

def nc_print_int_helper_asm() -> str:
    """Returner AArch64-assembly for _nc_print_int (macOS/AArch64).

    Konvensjon: x0 = heltall å skrive (signed 64-bit).
    Skriver desimalrepresentasjonen etterfulgt av linjeskift til stdout.
    Bevarer alle callee-saved registre; ødelegger kun x0–x8, x16.
    Stack-ramme: 48 bytes — x29/x30 (16 b) + siffer-buffer (32 b).
    """
    return """\
_nc_print_int:
    stp x29, x30, [sp, #-48]!
    mov x29, sp
    // buffer [x29+16 .. x29+47], bygg baklengs fra x29+48
    add x4, x29, #48

    cbz x0, .Lpi_zero

    mov x8, #0
    cmp x0, xzr
    b.ge .Lpi_loop_start
    mov x8, #1
    neg x0, x0

.Lpi_loop_start:
    mov x3, #10
.Lpi_loop:
    udiv x5, x0, x3
    msub x6, x5, x3, x0
    add x6, x6, #48
    sub x4, x4, #1
    strb w6, [x4]
    mov x0, x5
    cbnz x0, .Lpi_loop
    cbz x8, .Lpi_write
    sub x4, x4, #1
    mov x5, #45
    strb w5, [x4]
    b .Lpi_write

.Lpi_zero:
    sub x4, x4, #1
    mov x5, #48
    strb w5, [x4]

.Lpi_write:
    add x5, x29, #48
    sub x2, x5, x4
    mov x0, #1
    mov x1, x4
    mov x16, #4
    svc #0x80
    mov x5, #10
    strb w5, [x29, #16]
    mov x0, #1
    add x1, x29, #16
    mov x2, #1
    mov x16, #4
    svc #0x80
    ldp x29, x30, [sp], #48
    ret
"""


def generate_aarch64_asm(la_decls, ops, exit_form,
                         params: list | None = None,
                         func_label: str = "_main") -> str:
    """Translate (la_decls, ops, exit_form) to macOS AArch64 assembly.

    la_decls: liste av (name, type, init_expr) eller (name, init_expr) (bakoverkompatibelt).
    params: liste av parameter-navn — lastes fra x0..xN til variabelplasser.
    func_label: assembly-etikett for generert funksjon (standard: _main).
    """
    # Normaliser la_decls til (name, vtype, init_expr)
    _la_normalized: list[tuple[str, str, object]] = []
    for entry in la_decls:
        if len(entry) == 3:
            _la_normalized.append(entry)           # (name, vtype, expr)
        else:
            _la_normalized.append((entry[0], "heltall", entry[1]))  # bakoverkompatibelt

    all_names: list[str] = []
    if params:
        all_names.extend(params)
    for name, _vtype, _expr in _la_normalized:
        if name not in all_names:
            all_names.append(name)

    if len(all_names) > _MAX_VARS:
        raise ValueError(f"Maks {_MAX_VARS} lokale navngitte verdier (fikk {len(all_names)})")

    var_slots: dict[str, str] = {name: _VAR_REGS[i] for i, name in enumerate(all_names)}
    used_vars = len(all_names)

    b = _AsmBuilder()
    is_main = func_label == "_main"

    # Helper functions use an epilogue_label so early returns can jump there
    epilogue_lbl = b._label() if not is_main else None
    b._epilogue_label = epilogue_lbl

    # ── Prologue ─────────────────────────────────────────────────────────────
    prolog = b.prologue(used_vars)

    # ── Load parameters from argument registers ───────────────────────────────
    param_lines: list[str] = []
    for i, pname in enumerate((params or [])):
        reg = var_slots[pname]
        if _ARG_REGS[i] != reg:
            param_lines.append(f"    mov {reg}, {_ARG_REGS[i]}")

    # ── Initialise la variables ───────────────────────────────────────────────
    for name, _vtype, init_expr in _la_normalized:
        b.emit_expr(init_expr, var_slots)
        reg = var_slots[name]
        if reg != "x0":
            b.op(f"mov {reg}, x0")

    # ── Body ops ──────────────────────────────────────────────────────────────
    b.emit_ops(ops, var_slots)

    # ── Exit (final return value → x0) ───────────────────────────────────────
    if isinstance(exit_form, tuple) and exit_form and exit_form[0] == "if":
        _, cond, then_expr, else_expr = exit_form
        else_lbl = b._label()
        end_lbl  = b._label()
        op_typ = _get_op_typ(cond)
        b.emit_cmp(cond, var_slots)
        inverse = _INVERSE_CC.get(op_typ, "ne")
        b.op(f"b.{inverse} {else_lbl}")
        b.emit_expr(then_expr, var_slots)
        b.op(f"b {end_lbl}")
        b.lbl(else_lbl)
        b.emit_expr(else_expr, var_slots)
        b.lbl(end_lbl)
    else:
        b.emit_expr(exit_form, var_slots)

    # ── Epilogue label (target for early returns in helpers) ──────────────────
    if epilogue_lbl:
        b.lbl(epilogue_lbl)

    epilog = b.epilogue(used_vars)

    lines: list[str] = []
    if is_main:
        lines += [
            ".section __TEXT,__text,regular,pure_instructions",
            ".global _main",
        ]
    lines.append(f"{func_label}:")
    lines.extend(prolog)
    lines.extend(param_lines)
    lines.extend(b.text_lines())
    lines.extend(epilog)
    if is_main:
        lines += ["    mov x16, #1", "    svc #0x80"]
    else:
        lines += ["    ret"]
    lines.extend(b.data_section())
    return "\n".join(lines) + "\n"
