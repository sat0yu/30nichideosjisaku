; haribote-os
; TAB=4

; Declare constant around BOOT_INFO
LEDS    EQU    0x0ff1
VMODE   EQU    0x0ff2 ; n-bit color
SCRNX   EQU    0x0ff4 ; screen x
SCRNY   EQU    0x0ff6 ; screen y
VRAM    EQU    0x0ff8 ; the address of the starting point of graphic buffer

    ORG    0xc200    ; Set the origin at the 0x4200-th byte from 0x8000

    MOV    AL,0x13    ; VGA graphics, 320x200x8bit color
    MOV    AH,0x00
    INT    0x10       ; Set the screen mode
    MOV    BYTE [VMODE],8    ; memoize
    MOV    WORD [SCRNX],320
    MOV    WORD [SCRNY],200
    MOV    DWORD [SCRNY],0x000a0000

; Get the LED state from keyboard BIOS
    MOV    AH,0x02
    INT    0x16    ; keyboard BIOS
    MOV    [LEDS],AL

fin:
    HLT
    JMP    fin
