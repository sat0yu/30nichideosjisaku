#include "./bootpack.h"

#define MEMMAN_FREES    4090 // sizeof(FREEINFO) * 4090 = 32720 (nearly equal to 32KB)
#define MEMMAN_ADDR     0x003c0000 // memory manager uses the region latter than 0x003c0000

struct FREEINFO {
    unsigned int addr, size;
};

struct MEMMAN {
    // frees: the length of 'free'
    // maxfrees: the max valus of 'frees'
    // lostsize: the sum of region sizes that failed to free
    // losts: # of failure to free a region
    int frees, maxfrees, lostsize, losts;
    struct FREEINFO free[MEMMAN_FREES];
};

unsigned int memtest(unsigned int start, unsigned int end);
void memman_init(struct MEMMAN *man);
unsigned int memman_total(struct MEMMAN *man);
unsigned int memman_alloc(struct MEMMAN *man, unsigned int size);
int memman_free(struct MEMMAN *man, unsigned int addr, unsigned int size);
void memman_shift_succeeding_elements(struct MEMMAN *man, unsigned int i);
void memman_unshift_succeeding_elements(struct MEMMAN *man, unsigned int i);

void HariMain(void) {
    struct BOOTINFO *binfo = (struct BOOTINFO *) ADR_BOOTINFO;
    char s[40], mcursor[256], keybuf[32], mousebuf[128];
    int mx, my, i;
    unsigned int memtotal;
    struct MOUSE_DEC mdec;
    struct MEMMAN *memman = (struct MEMMAN *) MEMMAN_ADDR;

    init_gdtidt();
    init_pic();
    io_sti(); // Remove interruption restriction once IDT and PIC are initialized

    fifo8_init(&keyfifo, 32, keybuf);
    fifo8_init(&mousefifo, 128, mousebuf);
    io_out8(PIC0_IMR, 0xf9); // allow PIC1 and keyboard to interrupt
    io_out8(PIC1_IMR, 0xef); // allow mouse to interrupt

    init_keyboard();
    enable_mouse(&mdec);
    memtotal = memtest(0x00400000, 0xbfffffff);
    memman_init(memman);
    memman_free(memman, 0x00001000, 0x009e000); // 0x00001000 - 0x0009efff
    memman_free(memman, 0x00400000, memtotal - 0x00400000);

    init_palette();
    init_screen(binfo->vram, binfo->scrnx, binfo->scrny);
    mx = (binfo->scrnx - 16) / 2; // put the cursor in the center/middle position
    my = (binfo->scrny - 28 - 16) / 2;
    init_mouse_cursor(mcursor, COL8_008484);
    putblock8_8(binfo->vram, binfo->scrnx, 16, 16, mx, my, mcursor, 16);
    sprintf(s, "(%d, %d)", mx, my);
    putfonts8_asc(binfo->vram, binfo->scrnx, 0, 0, COL8_FFFFFF, s);

    sprintf(s, "memory %dMB    free %dKB", memtotal / (1024 * 1024), memman_total(memman) / 1024);
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

void memman_init(struct MEMMAN *man){
    man->frees = 0;
    man->maxfrees = 0;
    man->lostsize = 0;
    man->losts = 0;
    return;
}

unsigned int memman_total(struct MEMMAN *man){
    unsigned int i, t = 0;
    for(i = 0; i < man->frees; i++){
        t += man->free[i].size;
    }
    return t;
}

unsigned int memman_alloc(struct MEMMAN *man, unsigned int size){
    unsigned int i, a;
    for(i = 0; i < man->frees; i++){
        if(man->free[i].size < size){
            continue;
        }
        // allocate the requested size of region from the head of the current free space
        a = man->free[i].addr;
        // move the free space's head position backward by the requested size
        man->free[i].addr += size;
        // shrink the free space by the requested size
        man->free[i].size -= size;
        if(man->free[i].size > 0){
            return a;
        }
        // if there's no space left, drop the free space entry
        memman_unshift_succeeding_elements(man, i);
    }
    // there's no space available
    return 0;
}

int memman_free(struct MEMMAN *man, unsigned int addr, unsigned int size){
    int i, j;
    // determin the position i which satisfies `free[i-1].addr < addr < free[i].addr`
    for(i = 0; i < man->frees && man->free[i].addr > addr; i++){}
    // the previous free space i-1 is able to join with the freeing space
    if(i > 0 && man->free[i-1].addr + man->free[i-1].size == addr){
        // join with the previous region
        man->free[i-1].size += size;
        // if there's a succeeding free space that can join with
        if(i < man->frees && addr + size == man->free[i].addr){
            man->free[i-1].size += man->free[i].size;
            memman_unshift_succeeding_elements(man, i);
        }
        return 0;
    }
    // the succeeding free space i is able to join with the freeing space
    if(i < man->frees && addr + size == man->free[i].addr){
        man->free[i].addr = addr;
        man->free[i].size += size;
        return 0;
    }
    // insert a new free space entry
    if(man->frees < MEMMAN_FREES){
        memman_shift_succeeding_elements(man, i);
        if(man->maxfrees < man->frees){
            man->maxfrees = man->frees;
        }
        man->free[i].addr = addr;
        man->free[i].size = size;
        return 0;
    }
    // failed to free
    man->losts++;
    man->lostsize += size;
    return -1;
}

void memman_shift_succeeding_elements(struct MEMMAN *man, unsigned int i){
    unsigned int j;
    for(j = man->frees; j > i; j--){
        man->free[j] = man->free[j-1];
    }
    man->frees++;
    return;
}

void memman_unshift_succeeding_elements(struct MEMMAN *man, unsigned int i){
    man->frees--;
    for(; i < man->frees; i++){
        man->free[i] = man->free[i+1];
    }
    return;
}
