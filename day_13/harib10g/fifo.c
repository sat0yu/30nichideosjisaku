#include "bootpack.h"

#define FLAGS_OVERRUN   0x0001

void fifo32_init(struct FIFO32 *fifo, int size, int *buf){
    fifo->size  = size;
    fifo->buf   = buf;
    fifo->free  = size;
    fifo->flags = 0;
    fifo->p     = 0; // write head
    fifo->q     = 0; // read head
    return;
}

int fifo32_put(struct FIFO32 *fifo, int data){
    if(fifo->free == 0){
        // exceed the capacity
        fifo->flags |= FLAGS_OVERRUN;
        return -1;
    }
    fifo->buf[fifo->p++] = data;
    if(fifo->p == fifo->size){
        fifo->p = 0;
    }
    fifo->free--;
    return 0;
}

int fifo32_get(struct FIFO32 *fifo){
    int data;
    if(fifo->free == fifo->size){
        // when the buffer is empty
        return -1;
    }
    data = fifo->buf[fifo->q++];
    if(fifo->q == fifo->size){
        fifo->q = 0;
    }
    fifo->free++;
    return data;
}

int fifo32_status(struct FIFO32 *fifo){
    return fifo->size - fifo->free;
}