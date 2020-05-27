; hello-os
; TAB=4

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
    TIMES   18    DB    0x00  ; Equevalent the line above in NASM

; Main part
entry:
    MOV    AX,0        ; AX: accumulator
    MOV    SS,AX       ; SS: stack segment
    MOV    SP,0x7c00   ; SP: stack pointer
    MOV    DS,AX       ; DS: data segment
    MOV    ES,AX       ; DS: extra segment

    MOV    SI,msg      ; substitute the memory address of the msg label

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

msg:
    DB    0x0a, 0x0a
    DB    "hello, world"
    DB    0x0a
    DB    0

    TIMES    0x1fe-($-$$)    DB    0x00

    DB 0x55, 0xaa
