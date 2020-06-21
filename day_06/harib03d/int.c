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
