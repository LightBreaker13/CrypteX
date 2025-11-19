    .section .multiboot
    .align 8
mb2_start:
    .long 0xE85250D6
    .long 0
    .long mb2_end - mb2_start
    .long -(0xE85250D6 + 0 + (mb2_end - mb2_start))
    
    .short 5
    .short 0
    .long 20
    .long 1024
    .long 768
    .long 32
    
    .short 0
    .short 0
    .long 8
mb2_end:

    .section .text
    .global _start
    .type _start, @function

_start:
    cli
    mov $stack_top, %esp
    push %ebx
    call kernel_main
halt_loop:
    hlt
    jmp halt_loop

    .section .bss
    .align 16
    .space 32768
stack_top:

