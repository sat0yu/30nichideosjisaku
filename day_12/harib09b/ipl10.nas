; haribote-os
; TAB=4

CYLS    EQU    10    ; EQU: equal

    ORG    0x7c00    ; Set the origin at the beginning of the boot sector

; Commonly used declaration for a floppy disk formatted in FAT12
    JMP   entry
    DB    0x90
    DB    "HELLOIPL"       ; Boot sector name in 8-bytes
    DW    512              ; [Fixed] size of each sector
    DB    1                ; [Fixed] size of cluster
    DW    1                ; FAT starting point
    DB    2                ; [Fixed] number of FAT
    DW    224              ; Number of entries in the root directory
    DW    2880             ; [Fixed] size of the drive
    DB    0xf0             ; [Fixed] Media type
    DW    9                ; [Fixed] Length of FAT fields
    DW    18               ; [Fixed] Number of sectors in a track
    DW    2                ; [Fixed] Number of heads
    DD    0                ; Number of partitions
    DD    2880             ; Confirm the size of the drive
    DB    0,0,0x29         ; ???
    DD    0xffffffff       ; ??? (Serial number of the volume)
    DB    "HELLO-OS   "    ; Disk name in 11-bytes
    DB    "FAT12   "       ; Format name in 8-bytes
    TIMES   18    DB    0x00  ; RESB 18

; Main part
entry:
    MOV    AX,0        ; AX: accumulator
    MOV    SS,AX       ; SS: stack segment
    MOV    SP,0x7c00   ; SP: stack pointer
    MOV    DS,AX       ; DS: data segment
    MOV    ES,AX       ; DS: extra segment

    MOV    AX,0x0820
    MOV    ES,AX
    MOV    CH,0        ; specify the cylinder
    MOV    DH,0        ; specify the head
    MOV    CL,2        ; specify the sector

readloop:
    MOV    SI,0        ; retry counter

retry:
    MOV    AH,0x02     ; read 0x02 , write 0x03, verify 0x04 and seek 0x0c
    MOV    AL,1        ; 1st sector
    MOV    BX,0        ; specify the buffer address along with ES
    MOV    DL,0x00     ; drive numer (A: drive)
    INT    0x13        ; invoke disk BIOS
    JNC    next        ; JNC stands for "Jump if not carry"
    ADD    SI,1        ; count up the retry counter
    CMP    SI,5
    JAE    error       ; JAE stands for "Jump if above or equal"
    MOV    AH,0x00     ; set the seek mode
    MOV    DL,0x00     ; specify the reset mode (0x00)
    INT    0x13
    JMP    retry

next:
    MOV    AX,ES
    ADD    AX,0x0020   ; 0x0020 = 512 /* 1-sector */ / 16 /* Buffer-address = ES*16 + BX */
    MOV    ES,AX       ; Cannot write as ADD ES,0x0020
    ADD    CL,1        ; go to the next sector
    CMP    CL,18
    JBE    readloop    ; JBE stands for "Jump if below or equal"
    MOV    CL,1
    ADD    DH,1        ; flip the head
    CMP    DH,2
    JB     readloop    ; JB stands for "Jump if below"
    MOV    DH,0        ; turn over the head
    ADD    CH,1        ; go to the next cylinder
    CMP    CH,CYLS
    JB     readloop

    MOV    [0x0ff0],CH ; write the current cylinder index at 0x0ff0
    JMP    0xc200      ; Jump to haribote.sys; 0xc20 = 0x8000 + 0x4200

putloop:
    MOV    AL,[SI]     ; stands for "MOV AL,BYTE [SI]", meaning to load the byte at the address [SI]. The representation [X] refers to the stored data at the address X.
    ADD    SI,1        ; SI: source index
    CMP    AL,0        ; Declare the comparison
    JE     fin         ; Jump to the specified label if the both sides of the comparison have the same value (Equal)
    MOV    AH,0x0e     ; The preparation to output a character on the screen
    MOV    BX,15       ; Specify the color code
    INT    0x10        ; Invoke the video BIOS
    JMP    putloop

fin:
    HLT                ; Stop exhausting CPU resource
    JMP    fin         ; infinit loop

error:
    MOV    SI,msg

msg:
    DB    0x0a, 0x0a
    DB    "hello, world"
    DB    0x0a
    DB    0

    TIMES    0x1fe-($-$$)    DB    0x00

    DB 0x55, 0xaa
