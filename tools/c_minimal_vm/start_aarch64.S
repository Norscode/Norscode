/* Freestanding entry (Linux aarch64). */
    .text
    .globl _start
_start:
    mov x29, xzr
    bl norcode_compiler_main
    mov x8, #93
    svc #0
