#include "bootpack.h"

#define SHEET_USE    1

void sheet_shift_elements(struct SHTCTL *ctl, int begin, int end);
void sheet_unshift_elements(struct SHTCTL *ctl, int begin, int end);

struct SHTCTL *shtctl_init(struct MEMMAN *memman, unsigned char *vram, int xsize, int ysize){
    struct SHTCTL *ctl;
    int i;
    ctl = (struct SHTCTL *) memman_alloc_4k(memman, sizeof (struct SHTCTL));
    if (ctl == 0){
        goto err;
    }
    ctl->vram = vram;
    ctl->xsize = xsize;
    ctl->ysize = ysize;
    ctl->top = -1; // there's no sheet in the beginning
    for(i=0; i<MAX_SHEETS; i++){
        ctl->sheets0[i].flags = 0; // mark as "unused"
    }
err:
    return ctl;
}

struct SHEET *sheet_alloc(struct SHTCTL *ctl){
    struct SHEET *sht;
    int i;
    for(i=0; i<MAX_SHEETS; i++){
        if(ctl->sheets0[i].flags != 0){
            continue;
        }
        sht = &ctl->sheets0[i];
        sht->flags = SHEET_USE; // mark as "used"
        sht->height = -1; // invisible
        return sht;
    }
    return 0; // if there's no available sheet left
}

void sheet_setbuf(struct SHEET *sht, unsigned char *buf, int xsize, int ysize, int col_inv){
    sht->buf = buf;
    sht->bxsize = xsize;
    sht->bysize = ysize;
    sht->col_inv = col_inv;
    return;
}

void sheet_refreshsub(struct SHTCTL *ctl, int vx0, int vy0, int vx1, int vy1){
    int h, bx, by, vx, vy, bx0, by0, bx1, by1;
    unsigned char *buf, c, *vram = ctl->vram;
    struct SHEET *sht;
    // handle coordinates outside the region
    if(vx0 < 0){ vx0 = 0; }
    if(vy0 < 0){ vy0 = 0; }
    if(vx1 > ctl->xsize){ vx1 = ctl->xsize; }
    if(vy1 > ctl->ysize){ vy1 = ctl->ysize; }
    // fill pixels in side the region as looking up the sheets from bottom to top
    for(h=0; h <= ctl->top; h++){
        sht = ctl->sheets[h];
        buf = sht->buf;
        // calculate bx0, ..., by1 from vx0, ..., vy1
        bx0 = vx0 - sht->vx0 < 0           ? 0           : vx0 - sht->vx0;
        by0 = vy0 - sht->vy0 < 0           ? 0           : vy0 - sht->vy0;
        bx1 = vx1 - sht->vx0 > sht->bxsize ? sht->bxsize : vx1 - sht->vx0;
        by1 = vy1 - sht->vy0 > sht->bysize ? sht->bysize : vy1 - sht->vy0;
        for(by=by0; by < by1; by++){
            vy = sht->vy0 + by;
            for(bx=bx0; bx < bx1; bx++){
                vx = sht->vx0 + bx;
                c = buf[by * sht->bxsize + bx];
                if(c != sht->col_inv){
                    vram[vy * ctl->xsize + vx] = c;
                }
            }
        }
    }
    return;
}

void sheet_updown(struct SHTCTL *ctl, struct SHEET *sht, int height){
    int h, old = sht->height; // store the current height

    // adjust the given height with the upper/lower limit
    if(height > ctl->top + 1){
        height = ctl->top + 1;
    }
    if(height < -1){
        height = -1;
    }
    sht->height = height;

    // in the following part, rearranging sheets[] in the order
    // NOTE: the hiehgt(position) of the given sheet does not change yet in this line
    if(height < old){
        if(height < 0){ // height < 0 < old (meke the sheet invisible)
            if(old < ctl->top){
                // move the succeeding(covering) ones forward as burying the given sheet place; sheets[old]
                sheet_unshift_elements(ctl, old+1, ctl->top);
                // for(h = old; h < ctl->top; h++){
                //     ctl->sheets[h] = ctl->sheets[h+1];
                //     ctl->sheets[h]->height = h;
                // }
            }
            ctl->top--; // because an visible sheet hid
        } else { // 0 <= height < old
            // insert the given sheet (`ctl->sheets[height] = sht`) after pushing the succeeding sheets back
            sheet_shift_elements(ctl, height, old-1);
            // for(h = old; h > height; h--){
            //     ctl->sheets[h] = ctl->sheets[h-1];
            //     ctl->sheets[h]->height = h;
            // }
            ctl->sheets[height] = sht;
        }
        // draw windows again
        sheet_refreshsub(ctl, sht->vx0, sht->vy0, sht->vx0 + sht->bxsize, sht->vy0 + sht->bysize);
        return;
    }
    if(old < height){
        if(old < 0){ // old < 0 < height
            // insert the given sheet (`ctl->sheets[height] = sht`) after pulling the succeeding sheets up
            sheet_shift_elements(ctl, height, ctl->top);
            // for(h = ctl->top; h >= height; h--){
            //     ctl->sheets[h+1] = ctl->sheets[h];
            //     ctl->sheets[h+1]->height = h+1;
            // }
            ctl->sheets[height] = sht;
            ctl->top++; // because an visible sheet increased
        } else { // 0 < old < height
            // insert the given sheet (`ctl->sheets[height] = sht`) after pushing the succeeding sheets down
            sheet_unshift_elements(ctl, old+1, height);
            // for(h = old; h < height; h++){
            //     ctl->sheets[h] = ctl->sheets[h+1];
            //     ctl->sheets[h]->height = h;
            // }
            ctl->sheets[height] = sht;
        }
        // draw windows again
        sheet_refreshsub(ctl, sht->vx0, sht->vy0, sht->vx0 + sht->bxsize, sht->vy0 + sht->bysize);
        return;
    }

    // the height of the given sheet didn't changed
    return;
}

void sheet_refresh(struct SHTCTL *ctl, struct SHEET *sht, int bx0, int by0, int bx1, int by1){
    if(sht->height < 0){ // return immediately if the sheet is not visible
        return;
    }
    sheet_refreshsub(
        ctl,
        sht->vx0 + bx0,
        sht->vy0 + by0,
        sht->vx0 + bx1,
        sht->vy0 + by1
    );
    return;
}

void sheet_slide(struct SHTCTL *ctl, struct SHEET *sht, int vx0, int vy0){
    int old_vx0 = sht->vx0, old_vy0 = sht->vy0;
    sht->vx0 = vx0;
    sht->vy0 = vy0;
    if(sht->height < 0){ // return immediately if the sheet is not visible
        return;
    }
    sheet_refreshsub(ctl, old_vx0, old_vy0, old_vx0 + sht->bxsize, old_vy0 + sht->bysize);
    sheet_refreshsub(ctl,     vx0,     vy0,     vx0 + sht->bxsize,     vy0 + sht->bysize);
    return;
}

void sheet_free(struct SHTCTL *ctl, struct SHEET *sht){
    // hide the sheet for a start if it's not visible
    if(sht->height >= 0){
        sheet_updown(ctl, sht, -1);
    }
    sht->flags = 0; // mark as "unused"
    return;
}

void sheet_shift_elements(struct SHTCTL *ctl, int begin, int end){
    int i;
    for(i = end; i >= begin; i--){
        ctl->sheets[i+1] = ctl->sheets[i];
        ctl->sheets[i+1]->height = i+1;
    }
    return;
}

void sheet_unshift_elements(struct SHTCTL *ctl, int begin, int end){
    int i;
    for(i = begin; i <= end; i++){
        ctl->sheets[i-1] = ctl->sheets[i];
        ctl->sheets[i-1]->height = i-1;
    }
    return;
}
