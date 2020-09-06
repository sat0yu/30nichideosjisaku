#include "./bootpack.h"

struct FIFO32 *mousefifo;
int moucedata0;

// Interruption handler for PS/2 Mouse
void inthandler2c(int *esp){
    int data;
    // PIC0 covers IRQ00-07, PIC1 does IRQ08-15
    io_out8(PIC1_OCW2, 0x60 + 0x04);
    // Notify PIC1 to have accepted IRQ-02
    io_out8(PIC0_OCW2, 0x60 + 0x02);
    data = io_in8(PORT_KEYDAT);
    fifo32_put(mousefifo, data + moucedata0);
    return;
}

#define KEYCMD_SENDTO_MOUDE    0xd4
#define MOUSECMD_ENABLE        0xf4

void enable_mouse(struct FIFO32 *fifo, int data0, struct MOUSE_DEC *mdec){
    mousefifo = fifo;
    moucedata0 = data0;
    wait_KBC_sendready();
    io_out8(PORT_KEYCMD, KEYCMD_SENDTO_MOUDE);
    wait_KBC_sendready();
    io_out8(PORT_KEYDAT, MOUSECMD_ENABLE);
    mdec->phase = 0; // waiting ACK (0xfa) response
    return;
}

int mouse_decode(struct MOUSE_DEC *mdec, unsigned char dat){
    if (mdec->phase == 0) {
        if (dat == 0xfa) {
            mdec->phase = 1;
        }
        return 0;
    }
    if (mdec->phase == 1) {
        // check if the first receiving byte has an appropriate value range
        // if not somehow, the phase remains 1 and continue to waiting the next first byte
        if((dat & 0xc8) == 0x08){
            mdec->buf[0] = dat;
            mdec->phase = 2;
        }
        return 0;
    }
    if (mdec->phase == 2) {
        mdec->buf[1] = dat;
        mdec->phase = 3;
        return 0;
    }
    if (mdec->phase == 3) {
        mdec->buf[2] = dat;
        mdec->phase = 1;
        // extract the low-order three bit as masking the byte with 0x07
        mdec->btn = mdec->buf[0] & 0x07;
        mdec->x = mdec->buf[1];
        mdec->y = mdec->buf[2];
        // set the high-order bits 0/1 according to the first byte
        if((mdec->buf[0] & 0x10) != 0){
            mdec->x |= 0xffffff00;
        }
        if((mdec->buf[0] & 0x20) != 0){
            mdec->y |= 0xffffff00;
        }
        // y-axis needs to be turn over to display the mouce icon
        mdec->y = - mdec->y;
        return 1;
    }
    // won't reach this line
    return -1;
}
