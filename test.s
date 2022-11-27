.equ io_mem, 0xf8_00    ; address of IO memory region
.equ ndisp_base, io_mem + 8
.equ ndisp_d7, ndisp_base + 7   ; address of the last digit display register

; reset vector
.section text/vectors
.short start        ; 0x02 0x00

.section text/start
start:
    nop             ; 0x00
    ldi 1           ; 0x01 0x01
    addi 3          ; 0x50 0x03
    sta ndisp_d7    ; 0x03 0x0f 0xf8
    jmp halt

.section    text/halt   
halt:
    jmp halt      ; 0x04 0xfd 0xff
