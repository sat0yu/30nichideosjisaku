#include "./bootpack.h"

void HariMain(void) {
    struct BOOTINFO *binfo = (struct BOOTINFO *) ADR_BOOTINFO;
    char s[40], mcursor[256];
    int mx, my;

    init_gdtidt();
    init_palette();
    init_screen(binfo->vram, binfo->scrnx, binfo->scrny);
    mx = (binfo->scrnx - 16) / 2; // put the cursor in the center/middle position
    my = (binfo->scrny - 28 - 16) / 2;
    init_mouse_cursor(mcursor, COL8_008484);
    putblock8_8(binfo->vram, binfo->scrnx, 16, 16, mx, my, mcursor, 16);
    sprintf(s, "scrnx: %d", binfo->scrnx);
    putfonts8_asc(binfo->vram, binfo->scrnx, 16, 64, COL8_FFFFFF, s);

    for(;;){
        io_hlt();
    }
}
