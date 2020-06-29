#include "./bootpack.h"

struct MOUSE_DEC {
    unsigned char buf[3], phase;
    int x, y, btn;
};

extern struct FIFO8 keyfifo, mousefifo;
void enable_mouse(struct MOUSE_DEC *mdec);
int mouse_decode(struct MOUSE_DEC *mdec, unsigned char dat);
void init_keyboard(void);

void HariMain(void) {
    struct BOOTINFO *binfo = (struct BOOTINFO *) ADR_BOOTINFO;
    char s[40], mcursor[256], keybuf[32], mousebuf[128];
    int mx, my, i;
    struct MOUSE_DEC mdec;

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

    enable_mouse(&mdec);

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
                if(mouse_decode(&mdec, i) != 0){
                    sprintf(s, "[lcr %d %d]", mdec.x, mdec.y);
                    if((mdec.btn & 0x01) != 0){
                        s[1] = 'L';
                    }
                    if((mdec.btn & 0x02) != 0){
                        s[3] = 'R';
                    }
                    if((mdec.btn & 0x04) != 0){
                        s[2] = 'C';
                    }
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

void enable_mouse(struct MOUSE_DEC *mdec){
    wait_KBC_sendready();
    io_out8(PORT_KEYCMD, KEYCMD_WRITE_MODE);
    wait_KBC_sendready();
    io_out8(PORT_KEYDAT, KBC_MODE);
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
