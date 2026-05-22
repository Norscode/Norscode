/* Freestanding entry (Linux x86_64). */
    .text
    .globl _start
    .type _start, @function
_start:
    xor %rbp, %rbp
    call norcode_compiler_main
    mov %eax, %edi
    mov $60, %rax
    syscall
    .size _start, .-_start
