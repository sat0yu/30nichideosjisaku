#include "./bootpack.h"

unsigned int memtest(unsigned int start, unsigned int end);
unsigned int memtest_sub(unsigned int start, unsigned int end);

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
    enable_mouse(&mdec);

    init_palette();
    init_screen(binfo->vram, binfo->scrnx, binfo->scrny);
    mx = (binfo->scrnx - 16) / 2; // put the cursor in the center/middle position
    my = (binfo->scrny - 28 - 16) / 2;
    init_mouse_cursor(mcursor, COL8_008484);
    putblock8_8(binfo->vram, binfo->scrnx, 16, 16, mx, my, mcursor, 16);
    sprintf(s, "(%d, %d)", mx, my);
    putfonts8_asc(binfo->vram, binfo->scrnx, 0, 0, COL8_FFFFFF, s);

    i = memtest(0x00400000, 0xbfffffff) / (1024 * 1024);
    sprintf(s, "memory %dMB", i);
    putfonts8_asc(binfo->vram, binfo->scrnx, 0, 32, COL8_FFFFFF, s);

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
                    boxfill8(binfo->vram, binfo->scrnx, COL8_000000, 32, 16, 32 * 15 + 8 - 1, 31);
                    putfonts8_asc(binfo->vram, binfo->scrnx, 32, 16, COL8_FFFFFF, s);
                    // move the mouse cursor
                    //   (1) erase the cursor
                    boxfill8(binfo->vram, binfo->scrnx, COL8_008484, mx, my, mx + 15, my + 15);
                    mx += mdec.x;
                    my += mdec.y;
                    //   (2) block the cursoe from going out of the window
                    if(mx < 0){
                        mx= 0;
                    }
                    if(my < 0){
                        my= 0;
                    }
                    if(mx > binfo->scrnx - 16){
                        mx= binfo->scrnx - 16;
                    }
                    if(my > binfo->scrny - 16){
                        my= binfo->scrny - 16;
                    }
                    //   (3) erase the previous position and rewrite the position and the cursor
                    boxfill8(binfo->vram, binfo->scrnx, COL8_008484, 0, 0, 79, 5);
                    sprintf(s, "(%d, %d)", mx, my);
                    putfonts8_asc(binfo->vram, binfo->scrnx, 0, 0, COL8_FFFFFF, s);
                    putblock8_8(binfo->vram, binfo->scrnx, 16, 16, mx, my, mcursor, 16);
                }
            }
        }
    }
}

#define EFLAGS_AC_BIT      0x00040000
#define CR0_CACHE_DISABLE  0x60000000

unsigned int memtest(unsigned int start, unsigned int end){
    char flg486 = 0;
    unsigned int eflg, cr0, i;

    // Check the CPU generation by loading the eflags again once updating it
    eflg = io_load_eflags();
    eflg |= EFLAGS_AC_BIT; // AC-bit = 1
    io_store_eflags(eflg);
    eflg = io_load_eflags();
    // AC-bit would return 0 when it is 386
    flg486 = ((eflg & EFLAGS_AC_BIT) != 0) ? 1 : flg486;
    // revert eflags as it was
    eflg &= ~EFLAGS_AC_BIT;
    io_store_eflags(eflg);

    // Disable caching
    if(flg486 != 0){
        cr0 = load_cr0();
        cr0 |= CR0_CACHE_DISABLE;
        store_cr0(cr0);
    }

    i = memtest_sub(start, end);

    // Enable caching
    if(flg486 != 0){
        cr0 = load_cr0();
        cr0 &= ~CR0_CACHE_DISABLE;
        store_cr0(cr0);
    }

    return i;
}

unsigned int memtest_sub(unsigned int start, unsigned int end){
    unsigned int i, *p, old, pat0 = 0xaa55aa55, pat1 = 0x55aa55aa;
    for(i = start; i <= end; i += 0x1000){ // 0x1000 = 4096 = 4KB
        p = (unsigned char *) (i + 0xffc); // 0x0ffc = 4092; will check the last four-bytes
        old = *p;
        *p = pat0;
        *p ^= 0xffffffff; // reverse the last four-bytes in the chunk
        if(*p != pat1){ // pat0 == pat1 ^ 0xffffffff
not_memoty:
            *p = old;
            break;
        }
        *p ^= 0xffffffff;
        if(*p != pat0){
            goto not_memoty;
        }
        *p = old;
    }
    return i;
}
