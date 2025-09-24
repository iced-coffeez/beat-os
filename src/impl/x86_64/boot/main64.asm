global long_mode_start
extern kernel_main

section .text
bits 64

section .text
global read_key

read_key:
    ; wait until the output buffer has data
.wait:
    in al, 0x64          ; read status register
    test al, 1           ; check bit 0 (output buffer status)
    jz .wait             ; if not set, loop until key available

    in al, 0x60          ; read scancode from data port
    ret



long_mode_start:
    ; load null into all data segment registers
    mov ax, 0
    mov ss, ax
    mov ds, ax
    mov es, ax
    mov fs, ax
    mov gs, ax
    
    call kernel_main