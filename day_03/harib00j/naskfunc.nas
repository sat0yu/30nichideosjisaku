; naskfunc
; TAB=4

section .text
    GLOBAL  io_hlt  ; The function name which is declared in this file

io_hlt:  ; void io_hlt(void)
    HLT
    RET
