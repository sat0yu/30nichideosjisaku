; haribote-os
; TAB=4

BOTPACK    EQU    0x00280000    ;
DSKCAC     EQU    0x00100000    ; disk cache
DSKCAC0    EQU    0x00008000    ; disk cache (real mode?)

; Declare constant around BOOT_INFO
CYLS    EQU    0x0ff0
LEDS    EQU    0x0ff1
VMODE   EQU    0x0ff2 ; n-bit color
SCRNX   EQU    0x0ff4 ; screen x
SCRNY   EQU    0x0ff6 ; screen y
VRAM    EQU    0x0ff8 ; the address of the starting point of graphic buffer

    ORG    0xc200    ; Set the origin at the 0x4200-th byte from 0x8000

; Specify the screen mode

    MOV    AL,0x13    ; VGA graphics, 320x200x8bit color
    MOV    AH,0x00
    INT    0x10       ; Set the screen mode
    MOV    BYTE [VMODE],8    ; memoize
    MOV    WORD [SCRNX],320
    MOV    WORD [SCRNY],200
    MOV    DWORD [VRAM],0x000a0000

; Get the LED state from keyboard BIOS

    MOV    AH,0x02
    INT    0x16    ; keyboard BIOS
    MOV    [LEDS],AL

; Guard PIC from any interuption
; According to the specificatin of AT-compatible machine,
; it may run into a problem when initializing PIC unless the protection
    MOV    AL,0xff
    OUT    0x21,AL
    NOP                ; There're a few machine types which do not work if repeating OUT-statement one after another
    OUT    0xa1,AL

    CLI                ; Prohibit interuption even from CPU

; Configure A20GATE up so for enabling memory accesses over 1MB

    CALL    waitkbdout
    MOV     AL,0xd1
    OUT     0x64,AL
    CALL    waitkbdout
    MOV     AL,0xdf        ; enable A20
    OUT     0x60,AL
    CALL    waitkbdout

; Go into Protection mode

    LGDT    [GDTR0]        ; Set a provisional GDT
    MOV     EAX,CR0
    AND     EAX,0x7fffffff ; set 0 at the 31st bit in order to prohibit paging
    OR      EAX,0x00000001 ; set 1 at the first bit for migrating to protection mode
    MOV     CR0,EAX
    JMP     pipelineflush

pipelineflush:
    MOV     AX,1*8         ; readable segment 32bits
    MOV     DS,AX
    MOV     ES,AX
    MOV     FS,AX
    MOV     GS,AX
    MOV     SS,AX

; copy bootpack.c

    MOV    ESI,bootpack
    MOV    EDI,BOTPACK
    MOV    ECX,512*1024/4
    CALL   memcpy

; copy disk data

; begin with the boot sector

    MOV    ESI,0x7c00
    MOV    ESI,DSKCAC
    MOV    ECX,512/4
    CALL   memcpy

; other all remaining parts

    MOV    ESI,DSKCAC0+512
    MOV    EDI,DSKCAC+512
    MOV    ECX,0
    MOV    CL,BYTE [CYLS]
    IMUL   ECX,512*18*2/4
    SUB    ECX,512/4
    CALL   memcpy

; Now that what asmhead is supposed to do are all done, then leave it to bootpack

    MOV    EBX,BOTPACK
    MOV    ECX,[EBX+16]
    ADD    ECX,3            ; ECX += 3
    SHR    ECX,2            ; ECX /= 4
    JZ     skip             ; there's nothing to copy
    MOV    ESI,[EBX+20]     ; copy soruce
    ADD    ESI,EBX
    MOV    EDI,[EBX+12]     ; copy destination
    CALL   memcpy

skip:
    MOV    ESP,[EBX+12]    ; stack initial value
    JMP    DWORD 2*8:0x0000001b

waitkbdout:
    IN     AL,0x64
    AND    AL,0x02
    JNZ    waitkbdout    ; Jump if not zero
    RET

memcpy:
    MOV    EAX,[ESI]
    ADD    ESI,4
    MOV    [EDI],EAX
    ADD    EDI,4
    SUB    ECX,1
    JNZ    memcpy    ; Jump if not zero
    RET

    ALIGNB  16, DB 0x00
GDT0:
    TIMES   8    DB    0x00    ; RESB 8
    DW      0xffff,0x0000,0x9200,0x00cf ; Readable segment 32bit
    DW      0xffff,0x0000,0x9a28,0x0047 ; Executable segment 32bit (for bootpack)

    DQ      0
GDTR0:
    DW      8*3-1
    DD      GDT0

    ALIGNB  16, DB 0x00
bootpack:
