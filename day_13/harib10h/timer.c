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
    timerctl.using = 0;
    for(i=0; i < MAX_TIMER; i++){
        timerctl.timers0[i].flags = TIMER_FLAGS_UNUSED;
    }
    return;
}

struct TIMER *timer_alloc(void){
    int i;
    for(i=0; i < MAX_TIMER; i++){
        if(timerctl.timers0[i].flags == TIMER_FLAGS_UNUSED){
            timerctl.timers0[i].flags = TIMER_FLAGS_ALLOC;
            return &timerctl.timers0[i];
        }
    }
    return 0; // found no available timer
}

void timer_free(struct TIMER *timer){
    timer->flags = TIMER_FLAGS_UNUSED;
    return;
}

void timer_init(struct TIMER *timer, struct FIFO32 *fifo, int data){
    timer->fifo = fifo;
    timer->data = data;
    return;
}

void timer_settime(struct TIMER *timer, unsigned int timeout){
    int e;
    // 's' is the previous character of 't' in the Roman alphabet
    struct TIMER *t, *s;
    timer->timeout = timeout + timerctl.count;
    timer->flags = TIMER_FLAGS_USING;
    e = io_load_eflags();
    io_cli();
    timerctl.using++;
    if(timerctl.using == 1){
        // there's just one timer working
        timerctl.t0 = timer;
        timer->next = 0; // mark as the last element in the linked list
        timerctl.next = timer->timeout;
        io_store_eflags(e);
        return;
    }
    t = timerctl.t0;
    // when putting the given timer at the head of the list
    if(timer->timeout <= t->timeout){
        timerctl.t0 = timer;
        timer->next = t;
        timerctl.next = timer->timeout;
        io_store_eflags(e);
        return;
    }
    // search the inserting position
    for(;;){
        s = t;
        t = t->next;
        if(t == 0){
            // reached at the tail of the list
            break;
        }
        if(timer->timeout <= t->timeout){
            // inserting the given timer between 's' and 't'
            s->next = timer;
            timer->next = t;
            io_store_eflags(e);
            return;
        }
    }
    // placing the given timer at the tail
    s->next = timer;
    timer->next = 0;
    io_store_eflags(e);
    return;
}

void inthandler20(int *esp){
    int i;
    struct TIMER *timer;
    io_out8(PIC0_OCW2, 0x60); // inform PIC that IRQ-00 has been received
    timerctl.count++;
    if(timerctl.next > timerctl.count){
        // no timer needs to be triggered
        return;
    }
    timer = timerctl.t0;
    for(i=0; i < timerctl.using; i++){
        if(timer->timeout > timerctl.count){
            // the timers in [(i+1)..MAX_TIMER] are NOT out of time yet
            break;
        }
        // the following three lines take effects on the timer that reached timeout
        timer->flags = TIMER_FLAGS_ALLOC;
        fifo32_put(timer->fifo, timer->data);
        timer = timer->next;
    }
    // the timers in [0..i] ran out of time
    timerctl.using -= i;
    // change the timer placed at the head
    timerctl.t0 = timer;
    if(timerctl.using > 0){
        timerctl.next = timerctl.t0->timeout;
    }else{
        timerctl.next = 0xffffffff;
    }
    return;
}
