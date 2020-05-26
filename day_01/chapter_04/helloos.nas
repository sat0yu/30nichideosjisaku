; hello-os
; TAB=4

; Commonly used declaration for a floppy disk formatted in FAT12
    DB    0xeb, 0x4e, 0x90
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
    ; RESB    18             ; ???
    TIMES   18    DB    0x00  ; Equevalent the line above in NASM

; Main part
    DB    0xb8, 0x00, 0x00, 0x8e, 0xd0, 0xbc, 0x00, 0x7c
    DB    0x8e, 0xd8, 0x8e, 0xc0, 0xbe, 0x74, 0x7c, 0x8a
    DB    0x04, 0x83, 0xc6, 0x01, 0x3c, 0x00, 0x74, 0x09
    DB    0xb4, 0x0e, 0xbb, 0x0f, 0x00, 0xcd, 0x10, 0xeb
    DB    0xee, 0xf4, 0xeb, 0xfd

; Message part
    DB    0x0a, 0x0a
    DB    "Hello, world"
    DB    0x0a
    DB    0

    ; RESB    0x1fe-$     ; Fill the succeeding parts with 0x00 until 0x001fe
    TIMES    0x1fe-($-$$)    DB    0x00  ; Equevalent the line above in NASM

    DB 0x55, 0xaa

; For irrlevant parts to the boot sector
    DB    0xf0, 0xff, 0xff, 0x00, 0x00, 0x00, 0x00, 0x00
    ; RESB    4600
    TIMES   4600    DB    0x00  ; Equevalent the line above in NASM
    DB    0xf0, 0xff, 0xff, 0x00, 0x00, 0x00, 0x00, 0x00
    ; RESB    1469432
    TIMES   1469432    DB    0x00  ; Equevalent the line above in NASM
