#include "./bootpack.h"

extern struct FIFO8 keyfifo;
void enable_mouse(void);
void init_keyboard(void);

void HariMain(void) {
    struct BOOTINFO *binfo = (struct BOOTINFO *) ADR_BOOTINFO;
    char s[40], mcursor[256], keybuf[32];
    int mx, my, i;

    init_gdtidt();
    init_pic();
    io_sti(); // Remove interruption restriction once IDT and PIC are initialized

    fifo8_init(&keyfifo, 32, keybuf);
    io_out8(PIC0_IMR, 0xf9); // allow PIC1 and keyboard to interrupt
    io_out8(PIC1_IMR, 0xef); // allow mouse to interrupt

    init_keyboard();

    init_palette();
    init_screen(binfo->vram, binfo->scrnx, binfo->scrny);
    mx = (binfo->scrnx - 16) / 2; // put the cursor in the center/middle position
    my = (binfo->scrny - 28 - 16) / 2;
    init_mouse_cursor(mcursor, COL8_008484);
    putblock8_8(binfo->vram, binfo->scrnx, 16, 16, mx, my, mcursor, 16);
    sprintf(s, "scrnx: %d", binfo->scrnx);
    putfonts8_asc(binfo->vram, binfo->scrnx, 16, 64, COL8_FFFFFF, s);

    enable_mouse();

    for(;;){
        io_cli();
        if(fifo8_status(&keyfifo) == 0){
            // there's no receiving data to process
            // NOTE: In order to perform STI and HLT operations as atomic,
            // use io_stihlt() in place of calling io_sti() and then io_hlt() one after the other.
            io_stihlt();
        }else{
            i = fifo8_get(&keyfifo);
            io_sti();
            sprintf(s, "%x", i);
            boxfill8(binfo->vram, binfo->scrnx, COL8_000000, 0, 0, 32 * 8 - 1, 15);
            putfonts8_asc(binfo->vram, binfo->scrnx, 0, 0, COL8_FFFFFF, s);
        }
    }
}

#define PORT_KEYDAT            0x0060
#define PORT_KEYSTA            0x0064
#define PORT_KEYCMD            0x0064
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

void enable_mouse(void){
    wait_KBC_sendready();
    io_out8(PORT_KEYCMD, KEYCMD_WRITE_MODE);
    wait_KBC_sendready();
    io_out8(PORT_KEYDAT, KBC_MODE);
    return;
}

#define KEYCMD_SENDTO_MOUDE    0xd4
#define MOUSECMD_ENABLE        0xf4

// The circit related to Mouse is included in the Keyboard's
void init_keyboard(void){
    wait_KBC_sendready();
    io_out8(PORT_KEYCMD, KEYCMD_SENDTO_MOUDE);
    wait_KBC_sendready();
    io_out8(PORT_KEYDAT, MOUSECMD_ENABLE);
    return;
}
