#include "./bootpack.h"

struct FIFO32 *keyfifo;
int keydata0;

// Interruption handler for PS/2 Keyboard
void inthandler21(int *esp){
    int data;
    // Notify PIC to have accepted IRQ-01, otherwise, PIC does not resume monitoring interruption
    io_out8(PIC0_OCW2, 0x60 + 0x01);
    data = io_in8(PORT_KEYDAT);
    fifo32_put(keyfifo, data + keydata0);
    return;
}

#define PORT_KEYSTA            0x0064
#define KEYSTA_SEND_NOTREADY   0x02
#define KEYCMD_WRITE_MODE      0x60
#define KBC_MODE               0x47

void wait_KBC_sendready(void){
    // Once the connected keyboard gets ready,
    // it sends a data containing 1 at the second bit from the buttom to PORT_KEYSTA
    while(1){
        if((io_in8(PORT_KEYSTA) & KEYSTA_SEND_NOTREADY) == 0){
            break;
        }
    }
    return;
}

// The circit related to Mouse is included in the Keyboard's
void init_keyboard(struct FIFO32 *fifo, int data0){
    keyfifo = fifo;
    keydata0 = data0;
    wait_KBC_sendready();
    io_out8(PORT_KEYCMD, KEYCMD_WRITE_MODE);
    wait_KBC_sendready();
    io_out8(PORT_KEYDAT, KBC_MODE);
    return;
}
