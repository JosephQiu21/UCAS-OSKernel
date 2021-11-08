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
    __asm__ __volatile__("csrr x0, sscratch\n");
    // Check sleep queue to wake up
    check_timer();
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

int do_kill(pid_t pid){
    pcb_t *killed_pcb = &pcb[pid - 1];
    if (pid == 1){
        prints("> [ERROR] You are not allowed to kill shell.\n");
        return 0;
    }
    if (killed_pcb -> pid == 0){
        prints("> [ERROR] Process does not exist.\n");
        return 0;
    }

    // Unblock tasks in its waiting list
    while (!is_list_empty(&killed_pcb -> wait_list)){
        // Find last pcb in the waiting list
        pcb_t *wait_pcb = container_of(killed_pcb -> wait_list.prev, pcb_t, list);
        if (wait_pcb -> status != TASK_EXITED)
            do_unblock(wait_pcb);
    }

    // Release lock
    do_mutex_lock_release(killed_pcb -> lock);

    // Recycle PCB and memory
    killed_pcb -> status = (killed_pcb -> mode == ENTER_ZOMBIE_ON_EXIT) ? TASK_ZOMBIE : TASK_EXITED;

    killed_pcb -> pid = 0;
    delete_item(&(killed_pcb -> list));

    if (killed_pcb == current_running)
        do_scheduler();
    
    return 1;
}

pid_t do_getpid() {
    return current_running -> pid;
}

// Add current running to waiting list of pid
int do_waitpid(pid_t pid){
    if (pcb[pid - 1].status != TASK_EXITED && pcb[pid - 1].status != TASK_ZOMBIE)
        do_block(current_running, &(pcb[pid - 1].wait_list));
}

void do_exit(){
    pcb_t *exited_pcb = current_running;

    // Unblock tasks in its waiting list
    while (!is_list_empty(&exited_pcb -> wait_list)){
        // Find last pcb in the waiting list
        pcb_t *wait_pcb = container_of(exited_pcb -> wait_list.prev, pcb_t, list);
        if (wait_pcb -> status != TASK_EXITED)
            do_unblock(wait_pcb);
    }

    // Release lock
    do_mutex_lock_release(exited_pcb -> lock);

    // Recycle PCB and memory
    exited_pcb -> status = (exited_pcb -> mode == ENTER_ZOMBIE_ON_EXIT) ? TASK_ZOMBIE : TASK_EXITED;

    exited_pcb -> pid = 0;
    delete_item(&(exited_pcb -> list));

    do_scheduler();
}

void do_process_show() {
    prints("\n----------[PROCESS TABLE]----------\n");
    int i;
    char* state_table[5] = {"BLOCKED", "RUNNING", "READY", "ZOMBIE", "EXITED"};
    for (i = 0; i < NUM_MAX_TASK; i++){
        if (pcb[i].pid != 0){
            prints("[%d] PID : %d STATUS : %s\n", i, pcb[i].pid, state_table[pcb[i].status]);
        }
    }
    
}