OUTPUT_FORMAT("binary");

SECTIONS
{
    .head 0x0 : {
        LONG(64 * 1024)       /*  0 : stack + .data + heap */
        LONG(0x69726148)      /*  4 : signature "Hari" */
        LONG(0)               /*  8 : mmarea */
        LONG(0x310000)        /* 12 : initial value for stack and the destination to which .data is sent */
        LONG(SIZEOF(.data))   /* 16 : .data */
        LONG(LOADADDR(.data)) /* 20 : address of the initial .data value */
        LONG(0xE9000000)      /* 24 : 0xE9000000 */
        LONG(HariMain - 0x20) /* 28 : entry address */
        LONG(0)               /* 32 : heap address starting point */
    }

    .text : { *(.text) }

    .data 0x310000 : AT ( ADDR(.text) + SIZEOF(.text) ) {
        *(.data)
        *(.rodata*)
        *(.bss)
    }

    /DISCARD/ : { *(.eh_frame) }

}
