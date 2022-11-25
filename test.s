.equ io_mem, 0xf800 ; address of IO memory region
.equ ndisp_base, io_mem + 8
.equ ndisp_d7, ndisp_base + 7   ; address of the last digit display register

; reset vector
.section vectors
.byte start & 0xff	; 0x02
.byte start >> 8	; 0x00

.section start
start:
    ldi 1           ; 0x01 0x01
    addi 3          ; 0x50 0x03
    sta ndisp_d7    ; 0x03 0x0f 0xf8

.section halt
halt:
    jmp halt      ; 0x04 0xfd 0xff
