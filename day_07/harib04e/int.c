#include "bootpack.h"

// PIC (Programmable Interrupt Controller) accepts IRQ (Interrupt ReQuest) and then sends 2-bytes back.
// The 2-bytes forms "0xCD 0x??" and CPU interprets them as an INT-operation.
void init_pic(void) {
    // PIC0 represents the master PIC and PIC1 is for the slave PIC

    // IMR stands for Interrupt Mask Register
    io_out8(PIC0_IMR,  0xff); // deny all requests
    io_out8(PIC1_IMR,  0xff); // deny all requests

    // ICW stands for  Initial Control Word
    io_out8(PIC0_ICW1, 0x11); // ICW1 and ICW4 are related to the circit board and they always have the same value
    io_out8(PIC0_ICW2, 0x20); // IRQ0-IRQ7 are interprets as INT20-INT27
    io_out8(PIC0_ICW3, 1 << 2); // 00000100; signals from PIC1 goes into IRQ2
    io_out8(PIC0_ICW4, 0x01);

    io_out8(PIC1_ICW1, 0x11); // ICW1 and ICW4 are related to the circit board and they always have the same value
    io_out8(PIC1_ICW2, 0x28); // IRQ8-IRQ15 are interprets as INT28-INT2F
    io_out8(PIC1_ICW3, 2); // signals from PIC1 goes into IRQ2
    io_out8(PIC1_ICW4, 0x01);

    io_out8(PIC0_IMR,  0xfb); // 11111011; accept only signals from PIC1
    io_out8(PIC1_IMR,  0xff); // 11111111; deny all requests

    return;
}

#define PORT_KEYDAT    0x0060

struct FIFO8 keyfifo;

// Interruption handler for PS/2 Keyboard
void inthandler21(int *esp){
    unsigned char data;
    // Notify PIC to have accepted IRQ-01, otherwise, PIC does not resume monitoring interruption
    io_out8(PIC0_OCW2, 0x60 + 0x01);
    data = io_in8(PORT_KEYDAT);
    fifo8_put(&keyfifo, data);
    return;
}

// Interruption handler for PS/2 Mouse
void inthandler2c(int *esp){
    struct BOOTINFO *binfo = (struct BOOTINFO *) ADR_BOOTINFO;
    boxfill8(binfo->vram, binfo->scrnx, COL8_000000, 0, 0, 32 * 8 - 1, 15);
    putfonts8_asc(binfo->vram, binfo->scrnx, 0, 0, COL8_FFFFFF, "INT 2C (IRQ-12): PS/2 Mouse");
    for(;;){
        io_hlt();
    }
}

// Interruption handler for PIC0
// Some processors (e.g. Athlon64X2) emit this interruption once when initialization
void inthandler27(int *esp){
    io_out8(PIC0_OCW2, 0x67);  // Notify PIC that IRQ-07 has been accepted
    return;
}
