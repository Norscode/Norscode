from __future__ import annotations

from dataclasses import dataclass


@dataclass(slots=True)
class StackVariable:
    name: str
    offset: int


class NativeFunctionLowering:
    """
    Minimal x86_64 function lowering.

    Goal:
        Support:
            - stack frames
            - local variables
            - function calls
            - return handling

    ABI:
        System V AMD64 ABI
    """

    def __init__(self) -> None:
        self.code = bytearray()
        self.stack_size = 0
        self.locals: dict[str, StackVariable] = {}

    # ------------------------------------------------------------------
    # Function prologue / epilogue
    # ------------------------------------------------------------------

    def emit_prologue(self) -> None:
        """
        push rbp
        mov rbp, rsp
        """

        # push rbp
        self.code += b"\x55"

        # mov rbp, rsp
        self.code += b"\x48\x89\xe5"

    def emit_epilogue(self) -> None:
        """
        mov rsp, rbp
        pop rbp
        ret
        """

        # mov rsp, rbp
        self.code += b"\x48\x89\xec"

        # pop rbp
        self.code += b"\x5d"

        # ret
        self.code += b"\xc3"

    # ------------------------------------------------------------------
    # Stack allocation
    # ------------------------------------------------------------------

    def allocate_local(self, name: str, size: int = 8) -> StackVariable:
        self.stack_size += size

        variable = StackVariable(
            name=name,
            offset=-self.stack_size,
        )

        self.locals[name] = variable

        return variable

    def emit_stack_reserve(self) -> None:
        if self.stack_size <= 0:
            return

        # sub rsp, imm32
        self.code += b"\x48\x81\xec"
        self.code += int(self.stack_size).to_bytes(
            4,
            "little",
            signed=False,
        )

    # ------------------------------------------------------------------
    # Argument handling (System V ABI)
    # ------------------------------------------------------------------

    def emit_mov_rdi_imm32(self, value: int) -> None:
        self.code += b"\x48\xc7\xc7"
        self.code += int(value).to_bytes(4, "little", signed=True)

    def emit_mov_rsi_imm32(self, value: int) -> None:
        self.code += b"\x48\xc7\xc6"
        self.code += int(value).to_bytes(4, "little", signed=True)

    # ------------------------------------------------------------------
    # Return handling
    # ------------------------------------------------------------------

    def emit_mov_rax_imm32(self, value: int) -> None:
        self.code += b"\x48\xc7\xc0"
        self.code += int(value).to_bytes(4, "little", signed=True)

    # ------------------------------------------------------------------
    # Native call lowering
    # ------------------------------------------------------------------

    def emit_call_placeholder(self) -> int:
        """
        call rel32
        """

        self.code += b"\xe8"

        patch_offset = len(self.code)

        self.code += b"\x00\x00\x00\x00"

        return patch_offset

    def patch_call(self, patch_offset: int, target_offset: int) -> None:
        relative_offset = target_offset - (patch_offset + 4)

        self.code[patch_offset : patch_offset + 4] = (
            int(relative_offset).to_bytes(
                4,
                "little",
                signed=True,
            )
        )

    # ------------------------------------------------------------------
    # Example lowering
    # ------------------------------------------------------------------

    def lower_add_function(self) -> bytes:
        """
        Minimal example:

            add(a, b) -> a + b

        Arguments:
            rdi = a
            rsi = b

        Return:
            rax
        """

        self.emit_prologue()

        # mov rax, rdi
        self.code += b"\x48\x89\xf8"

        # add rax, rsi
        self.code += b"\x48\x01\xf0"

        self.emit_epilogue()

        return bytes(self.code)
