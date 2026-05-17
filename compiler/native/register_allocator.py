from __future__ import annotations

from dataclasses import dataclass


@dataclass(slots=True)
class PhysicalRegister:
    name: str
    code: int
    caller_saved: bool = True


RAX = PhysicalRegister("rax", 0)
RBX = PhysicalRegister("rbx", 3, caller_saved=False)
RCX = PhysicalRegister("rcx", 1)
RDX = PhysicalRegister("rdx", 2)
RSI = PhysicalRegister("rsi", 6)
RDI = PhysicalRegister("rdi", 7)
R8 = PhysicalRegister("r8", 8)
R9 = PhysicalRegister("r9", 9)


AVAILABLE_REGISTERS = [
    RAX,
    RBX,
    RCX,
    RDX,
    RSI,
    RDI,
    R8,
    R9,
]


@dataclass(slots=True)
class VirtualRegister:
    id: int
    physical: PhysicalRegister | None = None
    stack_slot: int | None = None


class RegisterAllocationError(RuntimeError):
    pass


class NativeRegisterAllocator:
    """
    Minimal native register allocator.

    Goal:
        Manage:
            - virtual registers
            - physical register ownership
            - spills
            - stack slots

    Current scope:
        - linear allocation
        - simple spills
        - reusable register pool
    """

    def __init__(self) -> None:
        self.next_virtual_id = 0
        self.free_registers = AVAILABLE_REGISTERS.copy()
        self.active: dict[int, VirtualRegister] = {}
        self.stack_size = 0

    # ------------------------------------------------------------------
    # Virtual registers
    # ------------------------------------------------------------------

    def create_virtual_register(self) -> VirtualRegister:
        self.next_virtual_id += 1

        virtual = VirtualRegister(id=self.next_virtual_id)

        self.active[virtual.id] = virtual

        return virtual

    # ------------------------------------------------------------------
    # Allocation
    # ------------------------------------------------------------------

    def allocate(self, virtual: VirtualRegister) -> PhysicalRegister:
        if virtual.physical is not None:
            return virtual.physical

        if self.free_registers:
            physical = self.free_registers.pop(0)
            virtual.physical = physical
            return physical

        self.spill_oldest_register()

        if not self.free_registers:
            raise RegisterAllocationError(
                "No registers available after spill"
            )

        physical = self.free_registers.pop(0)
        virtual.physical = physical

        return physical

    # ------------------------------------------------------------------
    # Release
    # ------------------------------------------------------------------

    def release(self, virtual: VirtualRegister) -> None:
        if virtual.physical is not None:
            self.free_registers.append(virtual.physical)
            virtual.physical = None

    # ------------------------------------------------------------------
    # Spill handling
    # ------------------------------------------------------------------

    def allocate_stack_slot(self, size: int = 8) -> int:
        self.stack_size += size
        return -self.stack_size

    def spill_oldest_register(self) -> None:
        for virtual in self.active.values():
            if virtual.physical is not None:
                virtual.stack_slot = self.allocate_stack_slot()

                spilled_register = virtual.physical

                virtual.physical = None

                self.free_registers.append(spilled_register)

                return

        raise RegisterAllocationError("No register available for spill")

    # ------------------------------------------------------------------
    # Debug helpers
    # ------------------------------------------------------------------

    def allocation_snapshot(self) -> dict[str, object]:
        return {
            "active": {
                str(v.id): {
                    "physical": (
                        v.physical.name if v.physical else None
                    ),
                    "stack_slot": v.stack_slot,
                }
                for v in self.active.values()
            },
            "free_registers": [r.name for r in self.free_registers],
            "stack_size": self.stack_size,
        }
