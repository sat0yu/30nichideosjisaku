#include "./bootpack.h"

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

unsigned int memman_alloc_4k(struct MEMMAN *man, unsigned int size){
    unsigned int a;
    // ceil(size)
    // hint: when ceiling 1234 with hundreds place, it can be done with floor100(1234 + 99) = 1300
    // it is the same for 1234 with thousands place, it is done with floor1000(1234 + 999) = 2000
    size = (size + 0xfff) & 0xfffff000;
    a = memman_alloc(man, size);
    return a;
}

int memman_free_4k(struct MEMMAN *man, unsigned int addr, unsigned int size){
    unsigned int i;
    size = (size + 0xfff) & 0xfffff000;
    i = memman_free(man, addr, size);
    return i;
}
