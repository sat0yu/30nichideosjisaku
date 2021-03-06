#include "./bootpack.h"

extern struct FIFO8 keyfifo, mousefifo;
void enable_mouse(void);
void init_keyboard(void);

void HariMain(void) {
    struct BOOTINFO *binfo = (struct BOOTINFO *) ADR_BOOTINFO;
    char s[40], mcursor[256], keybuf[32], mousebuf[128];
    int mx, my, i;
    unsigned char mouse_dbuf[3], mouse_phase;

    init_gdtidt();
    init_pic();
    io_sti(); // Remove interruption restriction once IDT and PIC are initialized

    fifo8_init(&keyfifo, 32, keybuf);
    fifo8_init(&mousefifo, 128, mousebuf);
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
    mouse_phase = 0;

    for(;;){
        io_cli();
        if(fifo8_status(&keyfifo) + fifo8_status(&mousefifo) == 0){
            // there's no receiving data to process
            // NOTE: In order to perform STI and HLT operations as atomic,
            // use io_stihlt() in place of calling io_sti() and then io_hlt() one after the other.
            io_stihlt();
        }else{
            if(fifo8_status(&keyfifo) != 0){
                i = fifo8_get(&keyfifo);
                io_sti();
                sprintf(s, "%x", i);
                boxfill8(binfo->vram, binfo->scrnx, COL8_000000, 0, 0, 32 * 8 - 1, 15);
                putfonts8_asc(binfo->vram, binfo->scrnx, 0, 0, COL8_FFFFFF, s);
            }else if(fifo8_status(&mousefifo) != 0){
                i = fifo8_get(&mousefifo);
                io_sti();
                if(mouse_phase == 0){
                    // waiting 0xfa which means the moues gets ready.
                    // the mouse sends its status in a three-bytes chunk one after another
                    if(i == 0xfa){
                        mouse_phase = 1;
                    }
                }else if(mouse_phase == 1){
                    mouse_dbuf[0] = i;
                    mouse_phase = 2;
                }else if(mouse_phase == 2){
                    mouse_dbuf[1] = i;
                    mouse_phase = 3;
                }else if(mouse_phase == 3){
                    mouse_dbuf[2] = i;
                    mouse_phase = 1;
                    // show the received data once all three bytes in the current chunk are loaded,
                    sprintf(s, "%x %x %x", mouse_dbuf[0], mouse_dbuf[1], mouse_dbuf[2]);
                    boxfill8(binfo->vram, binfo->scrnx, COL8_000000, 0, 0, 32 * 8 - 1, 15);
                    putfonts8_asc(binfo->vram, binfo->scrnx, 0, 0, COL8_FFFFFF, s);
                }

            }
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
