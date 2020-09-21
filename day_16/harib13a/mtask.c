#include "bootpack.h"

struct TASKCTL *taskctl;
struct TIMER *task_timer;

struct TASK *task_init(struct MEMMAN *memman){
    int i;
    struct TASK *task;
    struct SEGMENT_DESCRIPTOR *gdt = (struct SEGMENT_DESCRIPTOR *) ADR_GDT;
    taskctl = (struct TASKCTL *) memman_alloc_4k(memman, sizeof (struct TASKCTL));
    for(i = 0; i < MAX_TASKS; i++){
        taskctl->tasks0[i].flags = 0;
        taskctl->tasks0[i].sel = (TASK_GDT0 + i) * 8;
        set_segmdesc(gdt + TASK_GDT0 + i, 103, (int) &taskctl->tasks0[i].tss, AR_TSS32);
    }
    task = task_alloc();
    task->flags = 2; // mark as working(active)
    taskctl->running = 1;
    taskctl->now = 0; // meaning that the current active task is placed at 0
    taskctl->tasks[0] = task;
    load_tr(task->sel);
    task_timer = timer_alloc();
    timer_settime(task_timer, 2);
    return task;
}

struct TASK *task_alloc(){
    int i;
    struct TASK *task;
    for(i = 0; i < MAX_TASKS; i++){
        if(taskctl->tasks0[i].flags != 0){
            continue;
        }
        task = &taskctl->tasks0[i];
        task->flags = 1; // mark as used
        task->tss.eflags = 0x00000202; /* IF = 1; */
        task->tss.eax = 0;
        task->tss.ecx = 0;
        task->tss.edx = 0;
        task->tss.ebx = 0;
        task->tss.ebp = 0;
        task->tss.esi = 0;
        task->tss.edi = 0;
        task->tss.es = 0;
        task->tss.ds = 0;
        task->tss.fs = 0;
        task->tss.gs = 0;
        task->tss.ldtr = 0;
        task->tss.iomap = 0x40000000;
        return task;
    }
    return 0; // there's no room to allocate a new task
}

void task_run(struct TASK *task){
    task->flags = 2; // mark as working(active)
    taskctl->tasks[taskctl->running] = task;
    taskctl->running++;
    return;
}

void task_switch(void){
    timer_settime(task_timer, 2);
    if(taskctl->running < 2){
        return;
    }
    taskctl->now++; // switch to the next task
    if(taskctl->now == taskctl->running){
        // if reaching at the end, start over from the head of the list
        taskctl->now = 0;
    }
    farjump(0, taskctl->tasks[taskctl->now]->sel);
    return;
}
