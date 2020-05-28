; haribote-os
; TAB=4

    ORG    0xc200    ; Set the origin at the 0x4200-th byte from 0x8000
fin:
    HLT
    JMP    fin
