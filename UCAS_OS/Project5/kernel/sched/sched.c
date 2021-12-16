#include <os/list.h>
#include <os/mm.h>
#include <os/lock.h>
#include <os/sched.h>
#include <os/time.h>
#include <os/irq.h>
#include <screen.h>
#include <assert.h>
#include <pgtable.h>
#include <user_programs.h>
#include <os/elf.h>

pcb_t pcb[NUM_MAX_TASK];
const ptr_t pid0_stack = INIT_KERNEL_STACK + PAGE_SIZE;
pcb_t pid0_pcb = {
    .pid = 0,
    .kernel_sp = (ptr_t)pid0_stack,
    .user_sp = (ptr_t)pid0_stack,
    .preempt_count = 0
};

LIST_HEAD(ready_queue);
LIST_HEAD(sleep_queue);

extern pid_t process_id = 1;

/* current running task PCB */
pcb_t * volatile current_running;


int match_elf(char *file_name)
{
    for (int i = 1; i < 5; i++)
        if (kstrcmp(file_name, elf_files[i].file_name) == 0)
            return i;
    return -1;
}

pid_t do_exec(const char* file_name, int argc, char* argv[], spawn_mode_t mode){
    // Match ELF file
    if (match_elf(file_name) == -1) {
        prints("> Error 404: Program `%s` not found.\n", file_name);
        return -1;
    }

    int i = find_freepcb();
    if (i == -1) {
        prints("> [ERROR] Unable to execute another task.");
        return -1;
    }

    pcb_t *new_pcb = &pcb[i];
    new_pcb -> pid = i + 1;
    new_pcb -> pgdir = allocPage();
    clear_pgdir(new_pcb -> pgdir);
    share_pgtable(new_pcb -> pgdir, pa2kva(PGDIR_PA));

    new_pcb -> kernel_sp = alloc_page_helper(KERNEL_STACK_ADDR - PAGE_SIZE, new_pcb -> pgdir, 0) + PAGE_SIZE;
    // allocPage() + PAGE_SIZE;
    // alloc_page_helper(KERNEL_STACK_ADDR - PAGE_SIZE, new_pcb -> pgdir, 0) + PAGE_SIZE;
    new_pcb -> user_sp = alloc_page_helper(USER_STACK_ADDR - PAGE_SIZE, new_pcb -> pgdir, 1) + PAGE_SIZE - 0xa0;
    

    // Copy new arguments to new user stack
    uintptr_t new_argv_base = USER_STACK_ADDR - 0xa0;
    uint64_t *new_argv = new_pcb -> user_sp;
    for (i = 0; i < argc; i++) {
        *(new_argv + i) = (uint64_t)(new_argv_base + 0x10 * (i + 1));
        memcpy((char *)(new_pcb -> user_sp + 0x10 * (i + 1)), argv[i], 0x10 * (i + 1));
    }

    enqueue(&ready_queue, &new_pcb -> list);

    list_init(&new_pcb -> wait_list);


    new_pcb -> type = USER_PROCESS;
    new_pcb -> status = TASK_READY;
    new_pcb -> mode = mode;

    new_pcb -> cursor_x = 0;
    new_pcb -> cursor_y = 0;

    new_pcb -> num_lock = 0;
    new_pcb -> num_thread = 0;

    // Load elf file
    int elf_length;
    char *elf_binary;
    get_elf_file(file_name, &elf_binary, &elf_length);
    uintptr_t elf_entry = load_elf(elf_binary, elf_length, new_pcb -> pgdir, elf_alloc_page_helper);

    new_pcb -> user_sp = USER_STACK_ADDR - 0xa0;
    init_pcb_stack(new_pcb -> kernel_sp, new_pcb -> user_sp, elf_entry, new_pcb, argc, new_argv_base);

    return new_pcb -> pid;
}

pid_t do_mthread_create(void (* start_routine)(void *), void *arg){
    int i = find_freepcb();
    if (i == -1) {
        prints("> [ERROR] Unable to execute another task.");
        return -1;
    }

    pcb_t *new_pcb = &pcb[i];
    new_pcb -> pid = i + 1;
    new_pcb -> pgdir = current_running -> pgdir;

    current_running -> num_thread++;

    new_pcb -> kernel_sp = allocPage() + PAGE_SIZE;
    // allocPage() + PAGE_SIZE;
    // alloc_page_helper(KERNEL_STACK_ADDR - PAGE_SIZE, new_pcb -> pgdir, 0) + PAGE_SIZE;
    new_pcb -> user_sp = alloc_page_helper(USER_STACK_ADDR - PAGE_SIZE - current_running -> num_thread * PAGE_SIZE, new_pcb -> pgdir, 1) + PAGE_SIZE;
    

    enqueue(&ready_queue, &new_pcb -> list);

    list_init(&new_pcb -> wait_list);


    new_pcb -> type = USER_THREAD;
    new_pcb -> status = TASK_READY;
    new_pcb -> mode = current_running -> mode;

    new_pcb -> cursor_x = 0;
    new_pcb -> cursor_y = 0;

    new_pcb -> num_lock = 0;

    new_pcb -> user_sp = USER_STACK_ADDR - current_running -> num_thread * PAGE_SIZE;
    init_pcb_stack(new_pcb -> kernel_sp, new_pcb -> user_sp, start_routine, new_pcb, 1, arg);

    regs_context_t *pt_regs_child = (regs_context_t *) (new_pcb -> kernel_sp + sizeof(switchto_context_t));
    regs_context_t *pt_regs_father = (regs_context_t *) (current_running -> kernel_sp + sizeof(switchto_context_t));
    pt_regs_child->regs[3] = pt_regs_father->regs[3]; //share gp

    return new_pcb -> pid;
}

void do_scheduler(void)
{
    // Check sleep queue to wake up
    // check_timer(); 
    // Modify the current_running pointer.
    
    pcb_t *next_running = is_list_empty(&ready_queue) ? &pcb[0] : 
                                                        container_of(dequeue(&ready_queue), pcb_t, list);
    pcb_t *tmp = current_running;

    // debug
    if (current_running == &pcb[1] && next_running == &pcb[0]) {
        int debug = 1;
    }

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

    if (current_running -> pgdir != tmp -> pgdir) {
        set_satp(SATP_MODE_SV39, current_running -> pid, kva2pa(current_running -> pgdir) >> NORMAL_PAGE_SHIFT);
        local_flush_tlb_all();
    }
    
    switch_to(tmp, current_running);
}

void do_sleep(uint32_t sleep_time)
{
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
    enqueue(&ready_queue, pcb_node);
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
        pcb_t *wait_pcb = container_of(dequeue(&killed_pcb -> wait_list), pcb_t, list);
        if (wait_pcb -> status != TASK_EXITED)
            do_unblock(&(wait_pcb -> list));
    }

    // Release lock
    for (int i = 0; i < killed_pcb -> num_lock; i++) 
        do_mutex_lock_release(killed_pcb -> locks[i]);

    // Recycle PCB and memory
    killed_pcb -> status = (killed_pcb -> mode == ENTER_ZOMBIE_ON_EXIT) ? TASK_ZOMBIE : TASK_EXITED;

    killed_pcb -> pid = 0;

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
        do_block(&(current_running -> list), &(pcb[pid - 1].wait_list));
    return pid;
}

void do_exit(){
    pcb_t *exited_pcb = current_running;

    // Unblock tasks in its waiting list
    while (!is_list_empty(&exited_pcb -> wait_list)){
        // Find last pcb in the waiting list
        pcb_t *wait_pcb = container_of(dequeue(&exited_pcb -> wait_list), pcb_t, list);
        if (wait_pcb -> status != TASK_EXITED)
            do_unblock(&(wait_pcb -> list));
    }

    // Release lock
    for (int i = 0; i < exited_pcb -> num_lock; i++) 
        do_mutex_lock_release(exited_pcb -> locks[i]);

    // Recycle PCB and memory
    exited_pcb -> status = (exited_pcb -> mode == ENTER_ZOMBIE_ON_EXIT) ? TASK_ZOMBIE : TASK_EXITED;

    exited_pcb -> pid = 0;

    do_scheduler();
}

void do_process_show() {
    prints("\n---------- [PROCESS TABLE] ----------\n");
    int t;
    char* state_table[5] = {"BLOCKED", "RUNNING", "READY", "ZOMBIE", "EXITED"};
    for (t = 0; t < NUM_MAX_TASK; t++){
        if (pcb[t].pid != 0){
            prints("[%d] PID : %d STATUS : %s\n", t, pcb[t].pid, state_table[pcb[t].status]);
        }
    }
    
}

void do_ls()
{
    for (int i = 1; i < ELF_FILE_NUM; i++)
    {
        prints("%s ", elf_files[i].file_name);
    }
    prints("\n");
}
