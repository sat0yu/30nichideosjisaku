; naskfunc
; TAB=4

section .text
    GLOBAL  io_hlt, io_cli, io_sti, io_stihlt
    GLOBAL  io_in8, io_in16, io_in32
    GLOBAL  io_out8, io_out16, io_out32
    GLOBAL  io_load_eflags, io_store_eflags
    GLOBAL  load_gdtr, load_idtr
    GLOBAL  load_cr0, store_cr0
    GLOBAL  asm_inthandler20, asm_inthandler21
    GLOBAL  asm_inthandler27, asm_inthandler2c
    GLOBAL  memtest_sub
    EXTERN  inthandler20, inthandler21
    EXTERN  inthandler27, inthandler2c

io_hlt:  ; void io_hlt(void)
    HLT
    RET

io_cli:  ; void io_cli(void)
    CLI  ; clear interrupt flag: prohibit interruption
    RET

io_sti:  ; void io_sti(void)
    STI  ; set interrupt flag: allow interruption
    RET

io_stihlt:  ; void io_stihlt(void)
    STI
    HLT
    RET

io_in8:  ; int io_in8(int port)
    MOV     EDX,[ESP+4]  ; port
    MOV     EAX,0
    IN      AL,DX
    RET

io_in16:  ; int io_in16(int port)
    MOV     EDX,[ESP+4]  ; port
    MOV     EAX,0
    IN      AX,DX
    RET

io_in32:  ; int io_in32(int port)
    MOV     EDX,[ESP+4]  ; port
    IN      EAX,DX
    RET

io_out8:  ; void io_out8(int port, int data);
    MOV    EDX,[ESP+4]  ; port
    MOV    AL,[ESP+8]  ; data
    OUT    DX,AL
    RET

io_out16:  ; void io_out16(int port, int data);
    MOV    EDX,[ESP+4]  ; port
    MOV    EAX,[ESP+8]  ; data
    OUT    DX,AX
    RET

io_out32:  ; void io_out32(int port, int data);
    MOV    EDX,[ESP+4]  ; port
    MOV    EAX,[ESP+8]  ; data
    OUT    DX,EAX
    RET

io_load_eflags:  ; int io_load_eflags(void)
    PUSHFD  ; Push eflags
    POP    EAX
    RET

io_store_eflags:  ; void io_store_eflags(int eflags)
    MOV    EAX,[ESP+4]
    PUSH   EAX
    POPFD  ; Pop eflags
    RET

load_gdtr:  ; void load_gdtr(int limit, int addr)
    MOV    AX,[ESP+4]  ; limit
    MOV    [ESP+6],AX
    LGDT   [ESP+6]
    RET

load_idtr:  ; void load_idtr(int limit, int addr)
    MOV    AX,[ESP+4]  ; limit
    MOV    [ESP+6],AX
    LIDT   [ESP+6]
    RET

load_cr0:  ; int load_cr0(void)
    MOV    EAX,CR0  ; The returning value is passed via EAX
    RET

store_cr0:  ; void store_cr0(int cr0)
    MOV    EAX,[ESP+4]
    MOV    CR0,EAX
    RET

asm_inthandler20:
    PUSH    ES      ; evacuate the current registers' state. PUSH EAX == ADD ESP,-4 && MOV [SS:ESP],EAX
    PUSH    DS
    PUSHAD
    MOV     EAX,ESP
    PUSH    EAX
    MOV     AX,SS   ; set the same value to SS, DS, ES (as following the C-lang convention)
    MOV     DS,AX
    MOV     ES,AX
    CALL    inthandler20
    POP     EAX     ; restore the registers' state. POP EAX == MOV EAX,[SS:ESP] && ADD ESP,4
    POPAD
    POP     DS
    POP     ES
    IRETD

asm_inthandler21:
    PUSH    ES
    PUSH    DS
    PUSHAD
    MOV     EAX,ESP
    PUSH    EAX
    MOV     AX,SS
    MOV     DS,AX
    MOV     ES,AX
    CALL    inthandler21
    POP     EAX
    POPAD
    POP     DS
    POP     ES
    IRETD

asm_inthandler27:
    PUSH    ES
    PUSH    DS
    PUSHAD
    MOV     EAX,ESP
    PUSH    EAX
    MOV     AX,SS
    MOV     DS,AX
    MOV     ES,AX
    CALL    inthandler27
    POP     EAX
    POPAD
    POP     DS
    POP     ES
    IRETD

asm_inthandler2c:
    PUSH    ES
    PUSH    DS
    PUSHAD
    MOV     EAX,ESP
    PUSH    EAX
    MOV     AX,SS
    MOV     DS,AX
    MOV     ES,AX
    CALL    inthandler2c
    POP     EAX
    POPAD
    POP     DS
    POP     ES
    IRETD

memtest_sub:  ; unsigned int memtest_sub(unsigned int start, unsigned int end)
    PUSH    EDI  ; preserve the values of EBX, ESI, EDI
    PUSH    ESI
    PUSH    EBX
    MOV     ESI,0xaa55aa55  ; pat0
    MOV     EDI,0x55aa55aa  ; pat1
    MOV     EAX,[ESP+12+4]  ; i = start
mts_loop:
    MOV     EBX,EAX     ; p = i + 0x0ffc (work with the following line)
    ADD     EBX,0x0ffc
    MOV     EDX,[EBX]   ; old = *p
    MOV     [EBX],ESI   ; *p = pat0
    XOR     DWORD [EBX],0xffffffff  ; *p ^= 0xffffffff
    CMP     EDI,[EBX]   ; goto mts_fin if *p != pat1
    JNE     mts_fin
    XOR     DWORD [EBX],0xffffffff  ; *p ^= 0xffffffff
    CMP     ESI,[EBX]   ; goto mts_fin if *p != pat0
    JNE     mts_fin
    MOV     [EBX],EDX   ; *p = old
    ADD     EAX,0x1000  ; i += 0x1000
    CMP     EAX,[ESP+12+8]  ; goto mts_loop if i <= end
    JBE     mts_loop
    POP     EBX  ; restore the values of EBX, ESI, EDI
    POP     ESI
    POP     EDI
    RET
mts_fin:
    MOV     [EBX],EDX   ; *p = old
    POP     EBX  ; restore the values of EBX, ESI, EDI
    POP     ESI
    POP     EDI
    RET
