from __future__ import annotations

from dataclasses import dataclass


@dataclass
class NativeRegister:
    name: str
    code: int


RAX = NativeRegister("rax", 0)
RDI = NativeRegister("rdi", 7)


class NativeArithmeticLowering:
    """
    Minimal native arithmetic lowering layer.

    Goal:
        Lower simple integer arithmetic directly to x86_64 machinecode.

    Current scope:
        - integer immediates
        - addition
        - subtraction
        - return lowering
    """

    def __init__(self) -> None:
        self.code = bytearray()

    # ------------------------------------------------------------------
    # Register helpers
    # ------------------------------------------------------------------

    def emit_mov_rax_imm32(self, value: int) -> None:
        """
        mov rax, imm32
        """

        self.code += b"\x48\xc7\xc0"
        self.code += int(value).to_bytes(4, "little", signed=True)

    def emit_add_rax_imm32(self, value: int) -> None:
        """
        add rax, imm32
        """

        self.code += b"\x48\x05"
        self.code += int(value).to_bytes(4, "little", signed=True)

    def emit_sub_rax_imm32(self, value: int) -> None:
        """
        sub rax, imm32
        """

        self.code += b"\x48\x2d"
        self.code += int(value).to_bytes(4, "little", signed=True)

    # ------------------------------------------------------------------
    # Arithmetic lowering
    # ------------------------------------------------------------------

    def lower_addition(self, left: int, right: int) -> None:
        self.emit_mov_rax_imm32(left)
        self.emit_add_rax_imm32(right)

    def lower_subtraction(self, left: int, right: int) -> None:
        self.emit_mov_rax_imm32(left)
        self.emit_sub_rax_imm32(right)

    # ------------------------------------------------------------------
    # Generelle register-til-register-helpere (REX.W)
    # ------------------------------------------------------------------
    # Registre kodes med 4-bit indeks (0=rax, 1=rcx, 2=rdx, 3=rbx,
    # 4=rsp, 5=rbp, 6=rsi, 7=rdi, 8..15 = r8..r15). Når indeks >= 8 settes
    # REX.R/REX.B for å forlenge feltene fra 3 til 4 bit.

    @staticmethod
    def _modrm_rr(dst_reg: int, src_reg: int) -> int:
        """ModR/M byte for to GP-registre (mod=11)."""
        return 0xC0 | ((src_reg & 7) << 3) | (dst_reg & 7)

    def emit_mov_reg_imm32(self, reg: int, value: int) -> None:
        """mov regN, imm32 (sign-extended til 64 bit)."""
        rex = 0x48 | (0x01 if reg >= 8 else 0)
        self.code.append(rex)
        self.code.append(0xC7)
        self.code.append(0xC0 | (reg & 7))
        self.code += int(value).to_bytes(4, "little", signed=True)

    def emit_mov_reg_reg(self, dst: int, src: int) -> None:
        """mov dst64, src64."""
        rex = 0x48 | (0x04 if src >= 8 else 0) | (0x01 if dst >= 8 else 0)
        self.code.append(rex)
        self.code.append(0x89)
        self.code.append(self._modrm_rr(dst, src))

    def emit_add_reg_reg(self, dst: int, src: int) -> None:
        """add dst64, src64."""
        rex = 0x48 | (0x04 if src >= 8 else 0) | (0x01 if dst >= 8 else 0)
        self.code.append(rex)
        self.code.append(0x01)
        self.code.append(self._modrm_rr(dst, src))

    def emit_sub_reg_reg(self, dst: int, src: int) -> None:
        """sub dst64, src64."""
        rex = 0x48 | (0x04 if src >= 8 else 0) | (0x01 if dst >= 8 else 0)
        self.code.append(rex)
        self.code.append(0x29)
        self.code.append(self._modrm_rr(dst, src))

    def emit_imul_reg_reg(self, dst: int, src: int) -> None:
        """imul dst64, src64 — dst i /r-feltet, src i r/m-feltet."""
        rex = 0x48 | (0x04 if dst >= 8 else 0) | (0x01 if src >= 8 else 0)
        self.code.append(rex)
        self.code.append(0x0F)
        self.code.append(0xAF)
        self.code.append(0xC0 | ((dst & 7) << 3) | (src & 7))

    def emit_cqo(self) -> None:
        """cqo: sign-extend rax inn i rdx:rax — kreves før idiv."""
        self.code += b"\x48\x99"

    def emit_idiv_reg(self, divisor: int) -> None:
        """idiv divisor64: rdx:rax / divisor, kvotient i rax, rest i rdx."""
        rex = 0x48 | (0x01 if divisor >= 8 else 0)
        self.code.append(rex)
        self.code.append(0xF7)
        self.code.append(0xF8 | (divisor & 7))

    def emit_push_rax(self) -> None:
        """push rax (1 byte)."""
        self.code.append(0x50)

    def emit_pop_rax(self) -> None:
        """pop rax (1 byte)."""
        self.code.append(0x58)

    def emit_cmp_reg_reg(self, lhs: int, rhs: int) -> None:
        """cmp lhs64, rhs64 (= SUB lhs - rhs uten å lagre, setter flags)."""
        rex = 0x48 | (0x04 if rhs >= 8 else 0) | (0x01 if lhs >= 8 else 0)
        self.code.append(rex)
        self.code.append(0x39)
        self.code.append(self._modrm_rr(lhs, rhs))

    # ------------------------------------------------------------------
    # Betingede / ubetingede hopp (rel32, relativt til byte ETTER hoppet)
    # ------------------------------------------------------------------

    def _emit_jcc_rel32(self, opcode: int, rel32: int) -> None:
        """Felles helper for J<cc> rel32 (Jcc-instruksjoner er 6 bytes:
        0F <opcode> <imm32-LE>)."""
        if not -(2**31) <= rel32 < 2**31:
            raise ValueError(f"Jcc rel32 {rel32} ute av rekkevidde")
        self.code.append(0x0F)
        self.code.append(opcode)
        self.code += int(rel32).to_bytes(4, "little", signed=True)

    def emit_je_rel32(self, rel32: int) -> None:
        """je rel32 — hopp hvis ZF=1 (likhet)."""
        self._emit_jcc_rel32(0x84, rel32)

    def emit_jne_rel32(self, rel32: int) -> None:
        """jne rel32 — hopp hvis ZF=0."""
        self._emit_jcc_rel32(0x85, rel32)

    def emit_jl_rel32(self, rel32: int) -> None:
        """jl rel32 — hopp hvis fortegnet mindre (SF≠OF)."""
        self._emit_jcc_rel32(0x8C, rel32)

    def emit_jge_rel32(self, rel32: int) -> None:
        """jge rel32 — hopp hvis fortegnet større eller lik (SF=OF)."""
        self._emit_jcc_rel32(0x8D, rel32)

    def emit_jle_rel32(self, rel32: int) -> None:
        """jle rel32 — hopp hvis fortegnet mindre eller lik."""
        self._emit_jcc_rel32(0x8E, rel32)

    def emit_jg_rel32(self, rel32: int) -> None:
        """jg rel32 — hopp hvis fortegnet større."""
        self._emit_jcc_rel32(0x8F, rel32)

    def emit_jmp_rel32(self, rel32: int) -> None:
        """jmp rel32 — ubetinget hopp (5 bytes: E9 imm32)."""
        if not -(2**31) <= rel32 < 2**31:
            raise ValueError(f"jmp rel32 {rel32} ute av rekkevidde")
        self.code.append(0xE9)
        self.code += int(rel32).to_bytes(4, "little", signed=True)

    # ------------------------------------------------------------------
    # CALL / RET + generisk PUSH/POP reg
    # ------------------------------------------------------------------

    def emit_call_rel32(self, rel32: int) -> None:
        """call rel32 — PC-relativt funksjonskall (5 bytes: E8 imm32)."""
        if not -(2**31) <= rel32 < 2**31:
            raise ValueError(f"call rel32 {rel32} ute av rekkevidde")
        self.code.append(0xE8)
        self.code += int(rel32).to_bytes(4, "little", signed=True)

    def emit_ret(self) -> None:
        """ret (1 byte): C3."""
        self.code.append(0xC3)

    def emit_push_reg(self, reg: int) -> None:
        """push reg64 — 1 byte for 0-7, 2 bytes for r8-r15 (REX.B + 5x)."""
        if reg >= 8:
            self.code.append(0x41)
        self.code.append(0x50 | (reg & 7))

    def emit_pop_reg(self, reg: int) -> None:
        """pop reg64 — 1 byte for 0-7, 2 bytes for r8-r15 (REX.B + 5x)."""
        if reg >= 8:
            self.code.append(0x41)
        self.code.append(0x58 | (reg & 7))

    # ------------------------------------------------------------------
    # Linux exit lowering
    # ------------------------------------------------------------------

    def emit_linux_exit_from_rax(self) -> None:
        """
        mov rdi, rax
        mov rax, 60
        syscall
        """

        # mov rdi, rax
        self.code += b"\x48\x89\xc7"

        # mov rax, 60
        self.code += b"\x48\xc7\xc0\x3c\x00\x00\x00"

        # syscall
        self.code += b"\x0f\x05"

    # ------------------------------------------------------------------
    # Linux write(2) lowering
    # ------------------------------------------------------------------

    def emit_linux_write_stdout(self, msg_address: int, length: int) -> None:
        """
        write(1, msg_address, length) via syscall.

        x86_64 Linux:
            mov eax, 1        ; sys_write
            mov edi, 1        ; fd = stdout
            mov esi, msg      ; buf (32-bit imm zero-extends to rsi)
            mov edx, length   ; count
            syscall

        Forutsetter at msg_address passer i 32 bit (typisk 0x401000-noe på ELF-en vår).
        """
        if not 0 <= msg_address < 2**32:
            raise ValueError(f"msg_address {msg_address:x} passer ikke i 32 bit")
        if length < 0:
            raise ValueError("length må være ikke-negativ")

        # mov eax, 1
        self.code += b"\xb8\x01\x00\x00\x00"

        # mov edi, 1
        self.code += b"\xbf\x01\x00\x00\x00"

        # mov esi, msg_address
        self.code += b"\xbe"
        self.code += int(msg_address).to_bytes(4, "little", signed=False)

        # mov edx, length
        self.code += b"\xba"
        self.code += int(length).to_bytes(4, "little", signed=False)

        # syscall
        self.code += b"\x0f\x05"

    # ------------------------------------------------------------------
    # Public API
    # ------------------------------------------------------------------

    def lower_return_addition(self, left: int, right: int) -> bytes:
        self.lower_addition(left, right)
        self.emit_linux_exit_from_rax()
        return bytes(self.code)

    def lower_return_subtraction(self, left: int, right: int) -> bytes:
        self.lower_subtraction(left, right)
        self.emit_linux_exit_from_rax()
        return bytes(self.code)
