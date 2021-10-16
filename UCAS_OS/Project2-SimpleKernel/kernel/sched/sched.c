#include <os/list.h>
#include <os/mm.h>
#include <os/lock.h>
#include <os/sched.h>
#include <os/time.h>
#include <os/irq.h>
#include <screen.h>
#include <stdio.h>
#include <assert.h>

pcb_t pcb[NUM_MAX_TASK];
const ptr_t pid0_stack = INIT_KERNEL_STACK + PAGE_SIZE;
pcb_t pid0_pcb = {
    .pid = 0,
    .kernel_sp = (ptr_t)pid0_stack,
    .user_sp = (ptr_t)pid0_stack,
    .preempt_count = 0
};

LIST_HEAD(ready_queue);

/* current running task PCB */
pcb_t * volatile current_running;

/* global process id */
pid_t process_id = 1;

void do_scheduler(void)
{
    // Modify the current_running pointer.
    if(!is_list_empty(&ready_queue)){
        pcb_t *next_running = container_of(dequeue(&ready_queue), pcb_t, list);
        pcb_t *tmp = current_running;

        // If not kernel process, add current_running to ready_queue
        if(current_running -> status == TASK_RUNNING && current_running -> pid != 0){ 
            enqueue(&ready_queue, &(current_running -> list));
            current_running -> status = TASK_READY;
        }
        next_running -> status = TASK_RUNNING;
        current_running = next_running;
        process_id = current_running -> pid;
        // restore the current_runnint's cursor_x and cursor_y
        vt100_move_cursor(current_running->cursor_x, current_running->cursor_y);
        screen_cursor_x = current_running->cursor_x;
        screen_cursor_y = current_running->cursor_y;
        switch_to(tmp, current_running);
    }
}

void do_sleep(uint32_t sleep_time)
{
    // TODO: sleep(seconds)
    // note: you can assume: 1 second = `timebase` ticks
    // 1. block the current_running
    current_running -> status = TASK_BLOCKED;
    // 2. create a timer which calls `do_unblock` when timeout
    create_timer(sleep_time * time_base);
    // 3. reschedule because the current_running is blocked.
    do_scheduler();
}

void do_block(list_node_t *pcb_node, list_head *queue)
{
    enqueue(queue, pcb_node);
    current_running -> status = TASK_BLOCKED;
    do_scheduler();
}

void do_unblock(list_node_t *pcb_node)
{
    // Unblock the `pcb` from the block queue
    pcb_t *task = container_of(pcb_node, pcb_t, list);
    delete_item(pcb_node);
    task -> status = TASK_READY;
    enqueue(&ready_queue, &(task -> list));
}
