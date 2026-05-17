from __future__ import annotations

import struct
from pathlib import Path


ELF_MAGIC = b"\x7fELF"
ELFCLASS64 = 2
ELFDATA2LSB = 1
EV_CURRENT = 1
ELFOSABI_SYSV = 0

ET_EXEC = 2
EM_X86_64 = 62

PT_LOAD = 1

PF_X = 1
PF_W = 2
PF_R = 4

PAGE_SIZE = 0x1000
BASE_ADDRESS = 0x400000
TEXT_ALIGNMENT = 0x1000


class NativeELFWriter:
    """
    Minimal ELF64 executable writer for Linux x86_64.

    Goal:
        Generate standalone native executables without VM/runtime.
    """

    def __init__(self) -> None:
        self.text_section = bytearray()

    # ------------------------------------------------------------------
    # Machinecode emission
    # ------------------------------------------------------------------

    def emit_exit_0(self) -> None:
        """
        Linux x86_64:
            mov rax, 60
            mov rdi, 0
            syscall
        """

        # mov rax, 60
        self.text_section += b"\x48\xc7\xc0\x3c\x00\x00\x00"

        # mov rdi, 0
        self.text_section += b"\x48\xc7\xc7\x00\x00\x00\x00"

        # syscall
        self.text_section += b"\x0f\x05"

    # ------------------------------------------------------------------
    # ELF header writer
    # ------------------------------------------------------------------

    def build_elf_header(
        self,
        entry_address: int,
        program_header_offset: int,
        section_header_offset: int,
        program_header_count: int,
    ) -> bytes:
        ident = bytearray(16)
        ident[0:4] = ELF_MAGIC
        ident[4] = ELFCLASS64
        ident[5] = ELFDATA2LSB
        ident[6] = EV_CURRENT
        ident[7] = ELFOSABI_SYSV

        return struct.pack(
            "<16sHHIQQQIHHHHHH",
            bytes(ident),
            ET_EXEC,
            EM_X86_64,
            EV_CURRENT,
            entry_address,
            program_header_offset,
            section_header_offset,
            0,
            64,
            56,
            program_header_count,
            0,
            0,
            0,
        )

    # ------------------------------------------------------------------
    # Program header writer
    # ------------------------------------------------------------------

    def build_program_header(
        self,
        file_offset: int,
        virtual_address: int,
        segment_size: int,
    ) -> bytes:
        return struct.pack(
            "<IIQQQQQQ",
            PT_LOAD,
            PF_R | PF_X,
            file_offset,
            virtual_address,
            virtual_address,
            segment_size,
            segment_size,
            PAGE_SIZE,
        )

    # ------------------------------------------------------------------
    # Binary image assembly
    # ------------------------------------------------------------------

    def build_executable_image(self) -> bytes:
        elf_header_size = 64
        program_header_size = 56

        text_offset = TEXT_ALIGNMENT

        text_virtual_address = BASE_ADDRESS + text_offset

        entry_address = text_virtual_address

        program_header_offset = elf_header_size

        elf_header = self.build_elf_header(
            entry_address=entry_address,
            program_header_offset=program_header_offset,
            section_header_offset=0,
            program_header_count=1,
        )

        segment_size = len(self.text_section)

        program_header = self.build_program_header(
            file_offset=text_offset,
            virtual_address=text_virtual_address,
            segment_size=segment_size,
        )

        image = bytearray()

        image += elf_header
        image += program_header

        if len(image) < text_offset:
            image += b"\x00" * (text_offset - len(image))

        image += self.text_section

        return bytes(image)

    # ------------------------------------------------------------------
    # Executable file writing
    # ------------------------------------------------------------------

    def write_executable(self, output_path: str | Path) -> Path:
        output_path = Path(output_path)

        image = self.build_executable_image()

        output_path.write_bytes(image)

        output_path.chmod(0o755)

        return output_path


if __name__ == "__main__":
    writer = NativeELFWriter()
    writer.emit_exit_0()
    writer.write_executable("program")
