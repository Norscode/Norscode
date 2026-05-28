"""Generate AArch64 assembly text from the native pipeline IR.

Takes the same (la_decls, ops, exit_form) representation produced by
_entry_statements() in pipeline.py and emits clean macOS AArch64 assembly
that clang can assemble directly.  Uses symbolic labels — no raw-byte
offset arithmetic needed.

Register allocation:
  x0         – accumulator  (result of every expression)
  x1         – secondary scratch
  x2         – tertiary scratch (divisor / divisor for mod)
  x19–x23    – variable slots (callee-saved; up to 5 variables)
  x29 / x30  – frame pointer / link register (saved in prologue)
"""
from __future__ import annotations

from compiler.ast_nodes import (
    BinOpNode,
    CallNode,
    NumberNode,
    StringNode,
    VarAccessNode,
)

# ── Register names for variable slots ───────────────────────────────────────
_VAR_REGS = ["x19", "x20", "x21", "x22", "x23"]
_MAX_VARS  = len(_VAR_REGS)

# Condition code for the inverse of each comparison (used for loop exit)
_INVERSE_CC: dict[str, str] = {
    "EQ": "ne", "NE": "eq",
    "LT": "ge", "LTE": "gt",
    "GT": "le", "GTE": "lt",
}

# Condition code for direct use (used for conditional exit)
_DIRECT_CC: dict[str, str] = {
    "EQ": "eq", "NE": "ne",
    "LT": "lt", "LTE": "le",
    "GT": "gt", "GTE": "ge",
}


class _AsmBuilder:
    def __init__(self) -> None:
        self._text: list[str] = []
        self._strings: list[bytes] = []
        self._label_count = 0

    def _label(self) -> str:
        self._label_count += 1
        return f".L{self._label_count}"

    def op(self, instr: str) -> None:
        self._text.append(f"    {instr}")

    def lbl(self, name: str) -> None:
        self._text.append(f"{name}:")

    def add_string(self, data: bytes) -> str:
        idx = len(self._strings)
        self._strings.append(data)
        return f"_Lmsg_{idx}"

    # ── Utilities ────────────────────────────────────────────────────────────

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
        if isinstance(node, VarAccessNode):
            reg = var_slots.get(node.name)
            if reg is None:
                raise ValueError(f"Ukjent variabel: {node.name}")
            if reg != "x0":
                self.op(f"mov x0, {reg}")
            return
        if isinstance(node, BinOpNode):
            op = getattr(node.op, "typ", "")
            # Emit left → x0, save in x1, emit right → x0, compute
            self.emit_expr(node.left, var_slots)
            self.op("str x0, [sp, #-16]!")   # push left
            self.emit_expr(node.right, var_slots)
            self.op("mov x1, x0")             # right → x1
            self.op("ldr x0, [sp], #16")      # pop left → x0
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
                self.op(f"cmp x0, x1")
                end_lbl  = self._label()
                true_lbl = self._label()
                self.op(f"b.{_DIRECT_CC[op]} {true_lbl}")
                self.op("mov x0, #0")
                self.op(f"b {end_lbl}")
                self.lbl(true_lbl)
                self.op("mov x0, #1")
                self.lbl(end_lbl)
            else:
                raise ValueError(f"Ukjent binæroperator: {op}")
            return
        if isinstance(node, CallNode):
            # Simple call: push args, call, result in x0
            # Only handles integer args for now
            arg_regs = ["x0", "x1", "x2", "x3", "x4", "x5", "x6", "x7"]
            for i, arg in enumerate(node.args):
                self.emit_expr(arg, var_slots)
                if i < len(arg_regs) - 1:
                    self.op(f"mov {arg_regs[i]}, x0")
                else:
                    self.op(f"str x0, [sp, #-16]!")
            callee = node.callee.name if hasattr(node.callee, "name") else str(node.callee)
            self.op(f"bl _{callee}")
            return
        raise ValueError(f"Ukjent uttrykksnode: {type(node).__name__}")

    # ── Condition evaluation → flags ─────────────────────────────────────────

    def emit_cmp(self, cond, var_slots: dict[str, str]) -> str:
        """Emit the comparison and return the AArch64 condition code string."""
        op = getattr(cond.op, "typ", "")
        self.emit_expr(cond.left, var_slots)
        self.op("str x0, [sp, #-16]!")
        self.emit_expr(cond.right, var_slots)
        self.op("mov x1, x0")
        self.op("ldr x0, [sp], #16")
        self.op("cmp x0, x1")
        return _DIRECT_CC.get(op, "ne")

    # ── Statements ───────────────────────────────────────────────────────────

    def emit_ops(self, ops: list, var_slots: dict[str, str]) -> None:
        for op in ops:
            kind = op[0]
            if kind == "skriv":
                msg_bytes = op[1]
                lbl = self.add_string(msg_bytes)
                self.op("mov x0, #1")
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
                cc = self.emit_cmp(cond, var_slots)
                inverse = _INVERSE_CC.get(_get_op_typ(cond), "ne")
                self.op(f"b.{inverse} {loop_end}")
                self.emit_ops(body_ops, var_slots)
                self.op(f"b {loop_top}")
                self.lbl(loop_end)
            else:
                raise ValueError(f"Ukjent op: {kind}")

    # ── Full assembly output ─────────────────────────────────────────────────

    def text_section(self) -> list[str]:
        return self._text

    def data_section(self) -> list[str]:
        lines = []
        if self._strings:
            lines.append("")
            lines.append(".section __TEXT,__cstring,cstring_literals")
            for i, raw in enumerate(self._strings):
                lines.append(f"_Lmsg_{i}:")
                escaped = "".join(
                    f"\\{b:03o}" if (b < 0x20 or b > 0x7E) else chr(b)
                    for b in raw
                )
                lines.append(f'    .ascii "{escaped}"')
        return lines


def _get_op_typ(cond) -> str:
    return getattr(getattr(cond, "op", None), "typ", "")


def generate_aarch64_asm(la_decls, ops, exit_form) -> str:
    """Translate (la_decls, ops, exit_form) to macOS AArch64 assembly source."""
    if len(la_decls) > _MAX_VARS:
        raise ValueError(f"Maks {_MAX_VARS} variabler (fikk {len(la_decls)})")

    var_slots: dict[str, str] = {
        name: _VAR_REGS[i] for i, (name, _) in enumerate(la_decls)
    }

    b = _AsmBuilder()

    # Prologue: save callee-saved registers + frame pointer
    used_vars = len(la_decls)
    if used_vars > 0:
        # Save frame + link register
        b.op("stp x29, x30, [sp, #-16]!")
        b.op("mov x29, sp")
        # Save variable registers
        save_pairs = []
        regs = _VAR_REGS[:used_vars]
        for i in range(0, len(regs), 2):
            pair = regs[i:i+2]
            if len(pair) == 2:
                b.op(f"stp {pair[0]}, {pair[1]}, [sp, #-16]!")
            else:
                b.op(f"str {pair[0]}, [sp, #-16]!")

    # Initialise variable registers
    for name, init_expr in la_decls:
        b.emit_expr(init_expr, var_slots)
        reg = var_slots[name]
        if reg != "x0":
            b.op(f"mov {reg}, x0")

    # Body operations
    b.emit_ops(ops, var_slots)

    # Exit
    if isinstance(exit_form, tuple) and exit_form and exit_form[0] == "if":
        _, cond, then_expr, else_expr = exit_form
        else_lbl = b._label()
        end_lbl  = b._label()
        cc = b.emit_cmp(cond, var_slots)
        inverse = _INVERSE_CC.get(_get_op_typ(cond), "ne")
        b.op(f"b.{inverse} {else_lbl}")
        b.emit_expr(then_expr, var_slots)
        b.op(f"b {end_lbl}")
        b.lbl(else_lbl)
        b.emit_expr(else_expr, var_slots)
        b.lbl(end_lbl)
    else:
        b.emit_expr(exit_form, var_slots)

    # Epilogue: restore registers and call exit(x0)
    if used_vars > 0:
        regs = _VAR_REGS[:used_vars]
        for i in range(len(regs) - 1, -1, -2):
            pair = regs[max(0, i-1):i+1]
            if len(pair) == 2:
                b.op(f"ldp {pair[0]}, {pair[1]}, [sp], #16")
            else:
                b.op(f"ldr {pair[0]}, [sp], #16")
        b.op("ldp x29, x30, [sp], #16")

    b.op("mov x16, #1")   # exit syscall
    b.op("svc #0x80")

    lines = [
        ".section __TEXT,__text,regular,pure_instructions",
        ".global _main",
        "_main:",
    ]
    lines.extend(b.text_section())
    lines.extend(b.data_section())

    return "\n".join(lines) + "\n"
