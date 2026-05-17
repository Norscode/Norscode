from __future__ import annotations

from dataclasses import dataclass


@dataclass(slots=True)
class NativeLabel:
    name: str
    offset: int | None = None


@dataclass(slots=True)
class PendingJump:
    label: str
    patch_offset: int


class NativeControlFlowLowering:
    """
    Minimal x86_64 control-flow lowering.

    Goal:
        Lower comparisons and conditional branches directly
        into native machinecode.
    """

    def __init__(self) -> None:
        self.code = bytearray()
        self.labels: dict[str, NativeLabel] = {}
        self.pending_jumps: list[PendingJump] = []
        self.label_counter = 0

    # ------------------------------------------------------------------
    # Label generation
    # ------------------------------------------------------------------

    def create_label(self, prefix: str = "label") -> str:
        self.label_counter += 1
        name = f"{prefix}_{self.label_counter}"
        self.labels[name] = NativeLabel(name=name)
        return name

    def mark_label(self, label: str) -> None:
        self.labels[label].offset = len(self.code)

    # ------------------------------------------------------------------
    # Compare lowering
    # ------------------------------------------------------------------

    def emit_mov_rax_imm32(self, value: int) -> None:
        self.code += b"\x48\xc7\xc0"
        self.code += int(value).to_bytes(4, "little", signed=True)

    def emit_mov_rbx_imm32(self, value: int) -> None:
        self.code += b"\x48\xc7\xc3"
        self.code += int(value).to_bytes(4, "little", signed=True)

    def emit_cmp_rax_rbx(self) -> None:
        self.code += b"\x48\x39\xd8"

    # ------------------------------------------------------------------
    # Conditional jumps
    # ------------------------------------------------------------------

    def _emit_jump(self, opcode: bytes, label: str) -> None:
        self.code += opcode

        patch_offset = len(self.code)

        # Placeholder rel32
        self.code += b"\x00\x00\x00\x00"

        self.pending_jumps.append(
            PendingJump(
                label=label,
                patch_offset=patch_offset,
            )
        )

    def emit_je(self, label: str) -> None:
        self._emit_jump(b"\x0f\x84", label)

    def emit_jne(self, label: str) -> None:
        self._emit_jump(b"\x0f\x85", label)

    def emit_jl(self, label: str) -> None:
        self._emit_jump(b"\x0f\x8c", label)

    def emit_jg(self, label: str) -> None:
        self._emit_jump(b"\x0f\x8f", label)

    def emit_jle(self, label: str) -> None:
        self._emit_jump(b"\x0f\x8e", label)

    def emit_jge(self, label: str) -> None:
        self._emit_jump(b"\x0f\x8d", label)

    def emit_jmp(self, label: str) -> None:
        self.code += b"\xe9"

        patch_offset = len(self.code)

        self.code += b"\x00\x00\x00\x00"

        self.pending_jumps.append(
            PendingJump(
                label=label,
                patch_offset=patch_offset,
            )
        )

    # ------------------------------------------------------------------
    # Jump patching
    # ------------------------------------------------------------------

    def patch_jumps(self) -> None:
        for jump in self.pending_jumps:
            target = self.labels[jump.label]

            if target.offset is None:
                raise RuntimeError(f"Unresolved label: {jump.label}")

            relative_offset = target.offset - (jump.patch_offset + 4)

            self.code[jump.patch_offset : jump.patch_offset + 4] = (
                int(relative_offset).to_bytes(
                    4,
                    "little",
                    signed=True,
                )
            )

    # ------------------------------------------------------------------
    # Branch lowering
    # ------------------------------------------------------------------

    def lower_if_less_than(
        self,
        left: int,
        right: int,
        true_exit: int,
        false_exit: int,
    ) -> bytes:
        else_label = self.create_label("else")
        end_label = self.create_label("end")

        # compare
        self.emit_mov_rax_imm32(left)
        self.emit_mov_rbx_imm32(right)
        self.emit_cmp_rax_rbx()

        # if not less-than => else
        self.emit_jge(else_label)

        # true branch
        self.emit_mov_rax_imm32(true_exit)
        self.emit_jmp(end_label)

        # else branch
        self.mark_label(else_label)
        self.emit_mov_rax_imm32(false_exit)

        # end
        self.mark_label(end_label)

        self.patch_jumps()

        return bytes(self.code)
