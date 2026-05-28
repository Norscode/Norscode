"""AArch64 instruction encoding for the native Norscode compiler.

Covers the minimal instruction set needed by the native bootstrap pipeline:
integer arithmetic, comparisons, branches, system calls, and register moves.
All functions return bytes in little-endian order.
"""
from __future__ import annotations

# ── Register aliases ──────────────────────────────────────────────────────────
X0  = 0   # first argument / return value
X1  = 1
X16 = 16  # macOS syscall number register
XZR = 31  # zero register (reads as 0, writes discarded)
SP  = 31  # stack pointer (context-dependent)

# macOS AArch64 syscall numbers
SYSCALL_EXIT  = 1
SYSCALL_WRITE = 4


# ── Data movement ─────────────────────────────────────────────────────────────

def movz(Rd: int, imm16: int, shift: int = 0) -> bytes:
    """MOVZ Rd, #imm16, LSL #shift  (zero-extend; shift in {0,16,32,48})."""
    hw = shift // 16
    instr = 0xD2800000 | (hw << 21) | (imm16 << 5) | Rd
    return instr.to_bytes(4, "little")


def movk(Rd: int, imm16: int, shift: int = 0) -> bytes:
    """MOVK Rd, #imm16, LSL #shift  (keep other bits)."""
    hw = shift // 16
    instr = 0xF2800000 | (hw << 21) | (imm16 << 5) | Rd
    return instr.to_bytes(4, "little")


def mov_reg(Rd: int, Rm: int) -> bytes:
    """MOV Rd, Rm  (alias for ORR Rd, XZR, Rm)."""
    instr = 0xAA0003E0 | (Rm << 16) | Rd
    return instr.to_bytes(4, "little")


def load_imm64(Rd: int, value: int) -> bytes:
    """Load a 64-bit immediate into Rd using MOVZ + up to 3 MOVK instructions."""
    value &= 0xFFFF_FFFF_FFFF_FFFF
    parts = [(value >> (i * 16)) & 0xFFFF for i in range(4)]
    code = movz(Rd, parts[0], 0)
    for i in range(1, 4):
        if parts[i]:
            code += movk(Rd, parts[i], i * 16)
    return code


# ── Arithmetic ────────────────────────────────────────────────────────────────

def add_imm(Rd: int, Rn: int, imm12: int) -> bytes:
    """ADD Rd, Rn, #imm12."""
    instr = 0x91000000 | (imm12 << 10) | (Rn << 5) | Rd
    return instr.to_bytes(4, "little")


def sub_imm(Rd: int, Rn: int, imm12: int) -> bytes:
    """SUB Rd, Rn, #imm12."""
    instr = 0xD1000000 | (imm12 << 10) | (Rn << 5) | Rd
    return instr.to_bytes(4, "little")


def add_reg(Rd: int, Rn: int, Rm: int) -> bytes:
    """ADD Rd, Rn, Rm."""
    instr = 0x8B000000 | (Rm << 16) | (Rn << 5) | Rd
    return instr.to_bytes(4, "little")


def sub_reg(Rd: int, Rn: int, Rm: int) -> bytes:
    """SUB Rd, Rn, Rm."""
    instr = 0xCB000000 | (Rm << 16) | (Rn << 5) | Rd
    return instr.to_bytes(4, "little")


def mul_reg(Rd: int, Rn: int, Rm: int) -> bytes:
    """MUL Rd, Rn, Rm  (alias for MADD Rd, Rn, Rm, XZR)."""
    instr = 0x9B007C00 | (Rm << 16) | (Rn << 5) | Rd
    return instr.to_bytes(4, "little")


def sdiv_reg(Rd: int, Rn: int, Rm: int) -> bytes:
    """SDIV Rd, Rn, Rm."""
    instr = 0x9AC00C00 | (Rm << 16) | (Rn << 5) | Rd
    return instr.to_bytes(4, "little")


# ── Comparison and branches ───────────────────────────────────────────────────

def cmp_reg(Rn: int, Rm: int) -> bytes:
    """CMP Rn, Rm  (alias for SUBS XZR, Rn, Rm)."""
    instr = 0xEB000000 | (Rm << 16) | (Rn << 5) | XZR
    return instr.to_bytes(4, "little")


def cmp_imm(Rn: int, imm12: int) -> bytes:
    """CMP Rn, #imm12."""
    instr = 0xF100001F | (imm12 << 10) | (Rn << 5)
    return instr.to_bytes(4, "little")


# Condition codes for b_cond
_COND = {"EQ": 0x0, "NE": 0x1, "LT": 0xB, "LE": 0xD, "GT": 0xC, "GE": 0xA}


def b_cond(cond: str, offset_bytes: int) -> bytes:
    """B.<cond> label  (offset_bytes is signed byte offset from this instruction)."""
    imm19 = (offset_bytes >> 2) & 0x7FFFF
    instr = 0x54000000 | (imm19 << 5) | _COND[cond]
    return instr.to_bytes(4, "little")


def b_unconditional(offset_bytes: int) -> bytes:
    """B label  (signed byte offset)."""
    imm26 = (offset_bytes >> 2) & 0x3FFFFFF
    instr = 0x14000000 | imm26
    return instr.to_bytes(4, "little")


# ── Stack and memory ──────────────────────────────────────────────────────────

def stp_pre(Rt1: int, Rt2: int, Rn: int, imm7_bytes: int) -> bytes:
    """STP Rt1, Rt2, [Rn, #imm7]!  (pre-index; imm7_bytes must be multiple of 8)."""
    simm7 = (imm7_bytes // 8) & 0x7F
    instr = 0xA9800000 | (simm7 << 15) | (Rt2 << 10) | (Rn << 5) | Rt1
    return instr.to_bytes(4, "little")


def ldp_post(Rt1: int, Rt2: int, Rn: int, imm7_bytes: int) -> bytes:
    """LDP Rt1, Rt2, [Rn], #imm7  (post-index)."""
    simm7 = (imm7_bytes // 8) & 0x7F
    instr = 0xA8C00000 | (simm7 << 15) | (Rt2 << 10) | (Rn << 5) | Rt1
    return instr.to_bytes(4, "little")


# ── System calls ──────────────────────────────────────────────────────────────

def svc(imm16: int = 0x80) -> bytes:
    """SVC #imm16  (macOS uses 0x80)."""
    instr = 0xD4000001 | (imm16 << 5)
    return instr.to_bytes(4, "little")


def syscall_exit(code: int) -> bytes:
    """Emit AArch64 machine code for exit(code) on macOS."""
    return load_imm64(X0, code) + load_imm64(X16, SYSCALL_EXIT) + svc()


# ── NOP ───────────────────────────────────────────────────────────────────────

def nop() -> bytes:
    return b"\x1f\x20\x03\xd5"
