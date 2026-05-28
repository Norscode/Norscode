"""Mach-O 64-bit writer for macOS AArch64 (Apple Silicon) executables.

Produces a minimal LC_SEGMENT_64 + LC_MAIN Mach-O binary containing a
single __TEXT/__text section with caller-supplied AArch64 machine code.
"""
from __future__ import annotations

import struct
from pathlib import Path

# ── Mach-O constants ────────────────────────────────────────────────────────
MH_MAGIC_64    = 0xFEEDFACF
CPU_TYPE_ARM64 = 0x0100000C
CPU_SUBTYPE_ARM64_ALL = 0x00000000
MH_EXECUTE     = 0x00000002
MH_FLAGS       = 0x00200085  # NOUNDEFS | DYLDLINK | TWOLEVEL | PIE

LC_SEGMENT_64  = 0x00000019
LC_MAIN        = 0x80000028

PAGE_SIZE      = 0x4000       # 16 KB (Apple Silicon page)
TEXT_BASE      = 0x100000000  # standard 64-bit macOS base address


def _pad(data: bytes, alignment: int) -> bytes:
    r = len(data) % alignment
    return data + (b"\x00" * (alignment - r)) if r else data


def write_macho(code: bytes, output: Path) -> None:
    """Write *code* as the __text section of a minimal Mach-O AArch64 binary."""

    # ── Section header (80 bytes) ───────────────────────────────────────────
    # Layout: header(32) + LC_SEGMENT_64(72) + section(80) + LC_MAIN(24)
    HDR_SIZE   = 32
    SEG_SIZE   = 72    # LC_SEGMENT_64 without embedded section
    SECT_SIZE  = 80    # one Section64
    MAIN_SIZE  = 24
    LC_TOTAL   = SEG_SIZE + SECT_SIZE + MAIN_SIZE
    HEADER_TOTAL = HDR_SIZE + LC_TOTAL          # 208 bytes

    # Code starts after the header, padded to 4 bytes
    code_padded = _pad(code, 4)
    code_offset = HEADER_TOTAL
    total_file  = code_offset + len(code_padded)

    # Virtual address of the __text section
    text_vaddr  = TEXT_BASE + code_offset

    # ── Mach-O header (32 bytes — 64-bit adds a reserved word after flags) ────
    macho_hdr = struct.pack(
        "<IIIIIIII",
        MH_MAGIC_64,
        CPU_TYPE_ARM64,
        CPU_SUBTYPE_ARM64_ALL,
        MH_EXECUTE,
        2,            # ncmds
        LC_TOTAL,     # sizeofcmds
        MH_FLAGS,
        0,            # reserved (required for 64-bit Mach-O, absent in 32-bit)
    )

    # ── LC_SEGMENT_64 (72 bytes) ────────────────────────────────────────────
    seg_name  = b"__TEXT\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00"  # 16 bytes
    vmsize    = _align(total_file, PAGE_SIZE)
    seg_cmd = struct.pack(
        "<II16sQQQQIIII",
        LC_SEGMENT_64,
        SEG_SIZE + SECT_SIZE,   # cmdsize includes the embedded section
        seg_name,
        TEXT_BASE,              # vmaddr
        vmsize,                 # vmsize
        0,                      # fileoff
        total_file,             # filesize
        7,                      # maxprot  (rwx)
        5,                      # initprot (r-x)
        1,                      # nsects
        0,                      # flags
    )

    # ── Section64 (80 bytes) ────────────────────────────────────────────────
    sect_name = b"__text\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00"  # 16 bytes
    sect = struct.pack(
        "<16s16sQQIIIIIIII",
        sect_name,
        seg_name,
        text_vaddr,             # addr
        len(code_padded),       # size
        code_offset,            # offset in file
        2,                      # align (2^2 = 4-byte)
        0,                      # reloff
        0,                      # nreloc
        0x80000400,             # S_ATTR_PURE_INSTRUCTIONS | S_ATTR_SOME_INSTRUCTIONS
        0, 0, 0,                # reserved1/2/3
    )

    # ── LC_MAIN (24 bytes) ──────────────────────────────────────────────────
    lc_main = struct.pack(
        "<IIQI",
        LC_MAIN,
        MAIN_SIZE,
        code_offset,            # entryoff: byte offset into file of entry point
        0,                      # stacksize (0 = default)
    )

    image = macho_hdr + seg_cmd + sect + lc_main + code_padded
    output.write_bytes(image)
    output.chmod(0o755)


def _align(value: int, alignment: int) -> int:
    r = value % alignment
    return value if r == 0 else value + (alignment - r)
