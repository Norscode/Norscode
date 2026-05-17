from __future__ import annotations

from dataclasses import dataclass


@dataclass(slots=True)
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
