; haribote-os
; TAB=4

    ORG    0xc200    ; Set the origin at the 0x4200-th byte from 0x8000

    MOV    AL,0x13    ; VGA graphics, 320x200x8bit color
    MOV    AH,0x00
    INT    0x10       ; Set the screen mode
fin:
    HLT
    JMP    fin
