#include "./bootpack.h"

#define PIT_CTRL    0x0043
#define PIT_CNT0    0x0040

struct TIMERCTL timerctl;

#define TIMER_FLAGS_UNUSED   0 // un-used
#define TIMER_FLAGS_ALLOC    1 // ready to use
#define TIMER_FLAGS_USING    2 // in-use

void init_pit(void){
    int i;
    io_out8(PIT_CTRL, 0x34);
    io_out8(PIT_CNT0, 0x9c);
    io_out8(PIT_CNT0, 0x2e);
    timerctl.count = 0;
    timerctl.next = 0xffffffff;
    for(i=0; i < MAX_TIMER; i++){
        timerctl.timer[i].flags = TIMER_FLAGS_UNUSED;
    }
    return;
}

struct TIMER *timer_alloc(void){
    int i;
    for(i=0; i < MAX_TIMER; i++){
        if(timerctl.timer[i].flags == TIMER_FLAGS_UNUSED){
            timerctl.timer[i].flags = TIMER_FLAGS_ALLOC;
            return &timerctl.timer[i];
        }
    }
    return 0; // found no available timer
}

void timer_free(struct TIMER *timer){
    timer->flags = TIMER_FLAGS_UNUSED;
    return;
}

void timer_init(struct TIMER *timer, struct FIFO8 *fifo, unsigned char data){
    timer->fifo = fifo;
    timer->data = data;
    return;
}

void timer_settime(struct TIMER *timer, unsigned int timeout){
    timer->timeout = timeout + timerctl.count;
    timer->flags = TIMER_FLAGS_USING;
    if(timerctl.next > timer->timeout){
        timerctl.next = timer->timeout;
    }
    return;
}

void inthandler20(int *esp){
    int i;
    struct TIMER *t;
    io_out8(PIC0_OCW2, 0x60); // inform PIC that IRQ-00 has been received
    timerctl.count++;
    if(timerctl.next > timerctl.count){
        // no timer needs to be triggered
        return;
    }
    timerctl.next = 0xffffffff;
    for(i=0; i < MAX_TIMER; i++){
        t = &timerctl.timer[i];
        if(t->flags != TIMER_FLAGS_USING){
            continue;
        }
        if(t->timeout <= timerctl.count){
            t->flags = TIMER_FLAGS_ALLOC;
            fifo8_put(t->fifo, t->data);
        }else if (timerctl.next > t->timeout){
            // even if the timer is yet to be triggered, check whether the next timepoint should be updated
            timerctl.next = t->timeout;
        }
    }
    return;
}
