from __future__ import annotations


class NativeComparisonLowering:
    """
    Minimal x86_64 comparison lowering.

    Goal:
        Lower comparisons directly to native machinecode and
        normalize CPU flags into integer boolean values (0/1).
    """

    def __init__(self) -> None:
        self.code = bytearray()

    # ---------------------------------------------------------------
    # Register loading
    # ---------------------------------------------------------------

    def emit_mov_rax_imm32(self, value: int) -> None:
        self.code += b"\x48\xc7\xc0"
        self.code += int(value).to_bytes(4, "little", signed=True)

    def emit_mov_rbx_imm32(self, value: int) -> None:
        self.code += b"\x48\xc7\xc3"
        self.code += int(value).to_bytes(4, "little", signed=True)

    # ---------------------------------------------------------------
    # Compare / test emission
    # ---------------------------------------------------------------

    def emit_cmp_rax_rbx(self) -> None:
        """
        cmp rax, rbx
        """

        self.code += b"\x48\x39\xd8"

    def emit_test_rax_rax(self) -> None:
        """
        test rax, rax
        """

        self.code += b"\x48\x85\xc0"

    # ---------------------------------------------------------------
    # Boolean result lowering
    # ---------------------------------------------------------------

    def emit_zero_rax(self) -> None:
        """
        xor rax, rax
        """

        self.code += b"\x48\x31\xc0"

    def emit_sete_al(self) -> None:
        self.code += b"\x0f\x94\xc0"

    def emit_setne_al(self) -> None:
        self.code += b"\x0f\x95\xc0"

    def emit_setl_al(self) -> None:
        self.code += b"\x0f\x9c\xc0"

    def emit_setg_al(self) -> None:
        self.code += b"\x0f\x9f\xc0"

    def emit_setle_al(self) -> None:
        self.code += b"\x0f\x9e\xc0"

    def emit_setge_al(self) -> None:
        self.code += b"\x0f\x9d\xc0"

    # ---------------------------------------------------------------
    # Comparison lowering
    # ---------------------------------------------------------------

    def lower_eq(self, left: int, right: int) -> bytes:
        self.emit_mov_rax_imm32(left)
        self.emit_mov_rbx_imm32(right)
        self.emit_cmp_rax_rbx()
        self.emit_zero_rax()
        self.emit_sete_al()
        return bytes(self.code)

    def lower_ne(self, left: int, right: int) -> bytes:
        self.emit_mov_rax_imm32(left)
        self.emit_mov_rbx_imm32(right)
        self.emit_cmp_rax_rbx()
        self.emit_zero_rax()
        self.emit_setne_al()
        return bytes(self.code)

    def lower_lt(self, left: int, right: int) -> bytes:
        self.emit_mov_rax_imm32(left)
        self.emit_mov_rbx_imm32(right)
        self.emit_cmp_rax_rbx()
        self.emit_zero_rax()
        self.emit_setl_al()
        return bytes(self.code)

    def lower_gt(self, left: int, right: int) -> bytes:
        self.emit_mov_rax_imm32(left)
        self.emit_mov_rbx_imm32(right)
        self.emit_cmp_rax_rbx()
        self.emit_zero_rax()
        self.emit_setg_al()
        return bytes(self.code)

    def lower_le(self, left: int, right: int) -> bytes:
        self.emit_mov_rax_imm32(left)
        self.emit_mov_rbx_imm32(right)
        self.emit_cmp_rax_rbx()
        self.emit_zero_rax()
        self.emit_setle_al()
        return bytes(self.code)

    def lower_ge(self, left: int, right: int) -> bytes:
        self.emit_mov_rax_imm32(left)
        self.emit_mov_rbx_imm32(right)
        self.emit_cmp_rax_rbx()
        self.emit_zero_rax()
        self.emit_setge_al()
        return bytes(self.code)
