"""AArch64 assembly text generator for macOS.

Mirrors the interface of NativeArithmeticLowering but emits AArch64 assembly
source text that clang assembles into a Mach-O binary.

Register convention:
  x0        — accumulator (result register, like rax)
  x1        — secondary scratch (like rcx / rdx)
  x2        — tertiary scratch (for div)
  x19–x23   — variable registers (callee-saved, like rbx / r12–r15)
  x29 / fp  — frame pointer
  x30 / lr  — link register
  sp        — stack pointer

On macOS AArch64, syscalls use:
  x16 = syscall number, svc #0x80
  exit:  x16=1, x0=exit_code
  write: x16=4, x0=fd, x1=buf, x2=len
"""
from __future__ import annotations

import struct
from typing import ClassVar

# Variable slot → register name (callee-saved, so we don't need to save them
# around the calls in our minimal programs)
_VAR_REG_NAMES: list[str] = ["x19", "x20", "x21", "x22", "x23"]

# Register index → name mapping for the five variable slots + accumulator
_REG_NAME: dict[int, str] = {i: _VAR_REG_NAMES[i] for i in range(5)}
_REG_NAME[10] = "x0"   # accumulator slot (must not clash with 0–4)


class AArch64ProgramLowering:
    """Emits AArch64 assembly source compatible with clang on macOS.

    The caller builds a program by calling the same emit_* methods as
    NativeArithmeticLowering. After building, call `assembly()` to obtain
    the full assembly text, then pass it to clang.
    """

    _label_counter: ClassVar[int] = 0

    def __init__(self) -> None:
        self._lines: list[str] = []
        self._data_lines: list[str] = []
        self._string_counter = 0

    # ── Helpers ──────────────────────────────────────────────────────────────

    def _reg(self, slot: int) -> str:
        return _REG_NAME.get(slot, f"x{slot}")

    def _fresh_label(self, prefix: str = "L") -> str:
        AArch64ProgramLowering._label_counter += 1
        return f".{prefix}_{AArch64ProgramLowering._label_counter}"

    def _emit(self, line: str) -> None:
        self._lines.append(line)

    def _label(self, name: str) -> None:
        self._lines.append(f"{name}:")

    # ── Register moves ───────────────────────────────────────────────────────

    def emit_mov_reg_imm32(self, reg: int, value: int) -> None:
        r = self._reg(reg)
        v = int(value) & 0xFFFF_FFFF_FFFF_FFFF
        if 0 <= v <= 0xFFFF:
            self._emit(f"    movz {r}, #{v}")
        else:
            lo = v & 0xFFFF
            hi = (v >> 16) & 0xFFFF
            self._emit(f"    movz {r}, #{lo}")
            if hi:
                self._emit(f"    movk {r}, #{hi}, lsl #16")

    def emit_mov_rax_imm32(self, value: int) -> None:
        self.emit_mov_reg_imm32(10, value)

    def emit_mov_reg_reg(self, dst: int, src: int) -> None:
        d, s = self._reg(dst), self._reg(src)
        if d != s:
            self._emit(f"    mov {d}, {s}")

    # ── Arithmetic ───────────────────────────────────────────────────────────

    def emit_add_reg_reg(self, dst: int, src: int) -> None:
        d, s = self._reg(dst), self._reg(src)
        self._emit(f"    add {d}, {d}, {s}")

    def emit_sub_reg_reg(self, dst: int, src: int) -> None:
        d, s = self._reg(dst), self._reg(src)
        self._emit(f"    sub {d}, {d}, {s}")

    def emit_imul_reg_reg(self, dst: int, src: int) -> None:
        d, s = self._reg(dst), self._reg(src)
        self._emit(f"    mul {d}, {d}, {s}")

    def emit_cqo(self) -> None:
        pass  # Not needed on AArch64; sdiv handles sign extension

    def emit_idiv_reg(self, divisor: int) -> None:
        d = self._reg(divisor)
        acc = self._reg(10)
        self._emit(f"    sdiv {acc}, {acc}, {d}")

    def emit_add_rax_imm32(self, value: int) -> None:
        v = int(value)
        if 0 <= v <= 4095:
            self._emit(f"    add x0, x0, #{v}")
        else:
            self._emit(f"    movz x1, #{v & 0xFFFF}")
            if (v >> 16) & 0xFFFF:
                self._emit(f"    movk x1, #{(v >> 16) & 0xFFFF}, lsl #16")
            self._emit("    add x0, x0, x1")

    def emit_sub_rax_imm32(self, value: int) -> None:
        v = int(value)
        if 0 <= v <= 4095:
            self._emit(f"    sub x0, x0, #{v}")
        else:
            self._emit(f"    movz x1, #{v & 0xFFFF}")
            self._emit("    sub x0, x0, x1")

    # ── Stack (minimal — only used for intermediate values) ──────────────────

    def emit_push_rax(self) -> None:
        self._emit("    str x0, [sp, #-16]!")

    def emit_pop_rax(self) -> None:
        self._emit("    ldr x0, [sp], #16")

    def emit_push_reg(self, reg: int) -> None:
        self._emit(f"    str {self._reg(reg)}, [sp, #-16]!")

    def emit_pop_reg(self, reg: int) -> None:
        self._emit(f"    ldr {self._reg(reg)}, [sp], #16")

    # ── Comparisons ──────────────────────────────────────────────────────────

    def emit_cmp_reg_reg(self, lhs: int, rhs: int) -> None:
        l, r = self._reg(lhs), self._reg(rhs)
        self._emit(f"    cmp {l}, {r}")

    # ── Branches — patch-label approach ──────────────────────────────────────
    # The pipeline calls emit_jXX_rel32 and then later calls
    # patch_rel32_at to fill in the forward offset.  We instead use
    # symbolic labels: the first call reserves a label and emits the branch;
    # patch_rel32_at emits the target label.

    def _branch(self, cond: str) -> int:
        lbl = self._fresh_label("jmp")
        self._lines.append((cond, lbl))   # placeholder tuple
        return len(self._lines) - 1       # "address" = line index

    def _place_label(self, idx: int) -> None:
        entry = self._lines[idx]
        if isinstance(entry, tuple):
            cond, lbl = entry
            self._lines[idx] = f"    b.{cond} {lbl}"
            self._lines.append(f"{lbl}:")

    def emit_je_rel32(self, _rel: int) -> int:
        return self._branch("eq")

    def emit_jne_rel32(self, _rel: int) -> int:
        return self._branch("ne")

    def emit_jl_rel32(self, _rel: int) -> int:
        return self._branch("lt")

    def emit_jge_rel32(self, _rel: int) -> int:
        return self._branch("ge")

    def emit_jle_rel32(self, _rel: int) -> int:
        return self._branch("le")

    def emit_jg_rel32(self, _rel: int) -> int:
        return self._branch("gt")

    def emit_jmp_rel32(self, _rel: int) -> int:
        lbl = self._fresh_label("jmp")
        self._lines.append(("", lbl))  # unconditional
        return len(self._lines) - 1

    def _place_unconditional(self, idx: int) -> None:
        entry = self._lines[idx]
        if isinstance(entry, tuple):
            _, lbl = entry
            self._lines[idx] = f"    b {lbl}"
            self._lines.append(f"{lbl}:")

    def patch_rel32_at(self, idx: int) -> None:
        """Called by pipeline to resolve a forward branch."""
        entry = self._lines[idx]
        if isinstance(entry, tuple):
            cond, lbl = entry
            if cond:
                self._lines[idx] = f"    b.{cond} {lbl}"
            else:
                self._lines[idx] = f"    b {lbl}"
            self._lines.append(f"{lbl}:")

    # ── Calls ────────────────────────────────────────────────────────────────

    def emit_call_rel32(self, _rel: int) -> int:
        """Placeholder — real calls use emit_call_name."""
        return 0

    def emit_call_name(self, name: str) -> None:
        self._emit(f"    bl _{name}")

    def emit_ret(self) -> None:
        self._emit("    ret")

    # ── macOS syscalls ───────────────────────────────────────────────────────

    def emit_linux_exit_from_rax(self) -> None:
        """On macOS AArch64: exit(x0) via syscall #1."""
        self._emit("    mov x16, #1")
        self._emit("    svc #0x80")

    def emit_linux_write_stdout(self, msg_address: int, length: int) -> None:
        """On macOS AArch64: write(1, msg, len) via syscall #4.

        Note: msg_address is a virtual address placeholder; in the assembly
        output we use an Lstr_N data label instead.
        """
        # This is called by the pipeline with a vaddr placeholder.
        # We stash the (address, length) pair and emit an adrp+add sequence
        # at assembly time using a string label resolved at link time.
        lbl = f"_Lmsg_{self._string_counter}"
        self._string_counter += 1
        self._emit(f"    mov x0, #1")
        self._emit(f"    adrp x1, {lbl}@PAGE")
        self._emit(f"    add x1, x1, {lbl}@PAGEOFF")
        self._emit(f"    mov x2, #{length}")
        self._emit("    mov x16, #4")
        self._emit("    svc #0x80")
        # Store (label, length) for the data section
        self._data_lines.append((lbl, length))

    def add_string_data(self, label_idx: int, data: bytes) -> None:
        """Register the actual bytes for the label_idx-th string."""
        if label_idx < len(self._data_lines):
            lbl, _ = self._data_lines[label_idx]
            self._data_lines[label_idx] = (lbl, data)

    # ── Output ───────────────────────────────────────────────────────────────

    def assembly(self, strings: list[bytes], func_name: str = "start") -> str:
        out: list[str] = [
            ".section __TEXT,__text,regular,pure_instructions",
            ".global _main",
            "_main:",
            "    stp x29, x30, [sp, #-16]!",
            "    mov x29, sp",
        ]
        for line in self._lines:
            if isinstance(line, tuple):
                cond, lbl = line
                out.append(f"    b{'.' + cond if cond else ''} {lbl}")
            else:
                out.append(line)
        out.append("    ldp x29, x30, [sp, #16]")
        out.append("    ret")

        # String data section
        if strings:
            out.append("")
            out.append(".section __TEXT,__cstring,cstring_literals")
            for i, raw in enumerate(strings):
                lbl = f"_Lmsg_{i}"
                escaped = "".join(
                    f"\\{b:03o}" if b < 0x20 or b > 0x7E else chr(b)
                    for b in raw
                )
                out.append(f'{lbl}:')
                out.append(f'    .ascii "{escaped}"')

        return "\n".join(out) + "\n"
