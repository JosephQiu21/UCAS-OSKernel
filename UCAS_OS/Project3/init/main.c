/* * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *  * * * * * * * * * * *
 *            Copyright (C) 2018 Institute of Computing Technology, CAS
 *               Author : Han Shukai (email : hanshukai@ict.ac.cn)
 * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *  * * * * * * * * * * *
 *         The kernel's entry, where most of the initialization work is done.
 * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *  * * * * * * * * * * *
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy of this
 * software and associated documentation files (the "Software"), to deal in the Software
 * without restriction, including without limitation the rights to use, copy, modify,
 * merge, publish, distribute, sublicense, and/or sell copies of the Software, and to permit
 * persons to whom the Software is furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in
 * all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
 * THE SOFTWARE.
 *
 * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *  * * * * * * * * * * */

#include <common.h>
#include <os/irq.h>
#include <os/mm.h>
#include <os/list.h>
#include <os/sched.h>
#include <screen.h>
#include <sbi.h>
#include <stdio.h>
#include <os/time.h>
#include <os/lock.h>
#include <os/syscall.h>
#include <os/sync.h>
#include <test.h>
#include <csr.h>

#define INTERRUPT
// #define NONPREEMPT

extern void ret_from_exception();
extern void __global_pointer$();

uint64_t kernel_stack[NUM_MAX_TASK];
uint64_t user_stack[NUM_MAX_TASK];

list_head ready_queue;
list_head sleep_queue;


int find_freepcb();

static void init_pcb_stack(
    ptr_t kernel_stack, ptr_t user_stack, ptr_t entry_point, void *arg,
    pcb_t *pcb)
{
    int i;

    // set regs context
    regs_context_t *pt_regs = (regs_context_t *)(kernel_stack - sizeof(regs_context_t));
    for (i = 0; i < 32; i++)
    {
        pt_regs->regs[i] = 0;
    }
    pt_regs->regs[1] = (reg_t)entry_point;
    pt_regs->regs[2] = user_stack;
    pt_regs->regs[3] = (reg_t)__global_pointer$;
    pt_regs->regs[10] = (reg_t)arg;
    pt_regs->sepc = entry_point;
    pt_regs->sstatus = 0;
    pt_regs->scause = 0;
    pt_regs->sbadaddr = 0;

    // Update kernel_sp in pcb
    pcb->kernel_sp = kernel_stack - sizeof(regs_context_t) - sizeof(switchto_context_t);

    // set switch-to context
    switchto_context_t *st_regs = (switchto_context_t *)pcb->kernel_sp;

    // Set ra & sp
    st_regs->regs[0] = (reg_t)&ret_from_exception;
    st_regs->regs[1] = user_stack;
    for (i = 2; i < 14; i++)
    {
        st_regs->regs[i] = 0;
    }
}

pid_t do_spawn(task_info_t *task, void* arg, spawn_mode_t mode){
    int i = find_freepcb();
    if (i == -1) {
        printf("> [ERROR] Unable to spawn another task.");
        return -1;
    }
    pcb[i].status = TASK_READY;
    pcb[i].pid = i + 1;
    pcb[i].mode = mode;
    pcb[i].type = task -> type;
    pcb[i].kernel_sp = kernel_stack[i];
    pcb[i].user_sp = user_stack[i];
    list_init(&pcb[i].wait_list);
    init_pcb_stack(pcb[i].kernel_sp, pcb[i].user_sp, task -> entry_point, arg, &pcb[i]);
    enqueue(&ready_queue, &(pcb[i].list));
    return i + 1;
}

int find_freepcb() {
    int i;
    for (i = 1; i < NUM_MAX_TASK; i++){
        if (pcb[i].pid == 0)
            return i;
    }
    return -1;
}

void init_all_stack(){
    int i;
    for (i = 0; i < NUM_MAX_TASK; i++){
        kernel_stack[i] = allocPage(1);
        user_stack[i] = allocPage(1);
    }
}

static void init_shell()
{
    list_init(&ready_queue);
    list_init(&sleep_queue);
    pcb[0].pid = 1;
    pcb[0].status = TASK_READY;
    pcb[0].type = USER_PROCESS;
    pcb[0].kernel_sp = kernel_stack[0];
    pcb[0].user_sp = user_stack[0];
    init_pcb_stack(pcb[0].kernel_sp, pcb[0].user_sp, &test_shell, NULL, &pcb[0]);
    enqueue(&ready_queue, &(pcb[0].list));
    current_running = &pid0_pcb;
}

    void error_syscall(void)
    {
        printk("> Undefined syscall.\n\r");
    }

    static void init_syscall(void)
    {
        // initialize system call table.
        for (int i = 0; i < NUM_SYSCALLS; i++)
            syscall[i] = &error_syscall;
        syscall[SYSCALL_SLEEP            ] = &do_sleep;
        syscall[SYSCALL_WRITE            ] = &screen_write;
        syscall[SYSCALL_CURSOR           ] = &screen_move_cursor;
        syscall[SYSCALL_REFLUSH          ] = &screen_reflush;
        syscall[SYSCALL_SERIAL_READ      ] = &sbi_console_getchar;
        syscall[SYSCALL_SERIAL_WRITE     ] = &screen_serial_write;
        syscall[SYSCALL_SCREEN_CLEAR     ] = &screen_clear;
        syscall[SYSCALL_GET_TIMEBASE     ] = &get_time_base;
        syscall[SYSCALL_GET_TICK         ] = &get_ticks;
        syscall[SYSCALL_YIELD            ] = &do_scheduler;
        syscall[SYSCALL_MUTEX_GET        ] = &mutex_get;
        syscall[SYSCALL_MUTEX_OP         ] = &mutex_op;
        syscall[SYSCALL_GET_WALL         ] = &get_wall_time;
        syscall[SYSCALL_SPAWN            ] = &do_spawn;
        syscall[SYSCALL_KILL             ] = &do_kill;
        syscall[SYSCALL_PS               ] = &do_process_show;
        syscall[SYSCALL_GETPID           ] = &do_getpid;
        syscall[SYSCALL_WAITPID          ] = &do_waitpid;
        syscall[SYSCALL_EXIT             ] = &do_exit;
        syscall[SYSCALL_SEMAPHORE_GET    ] = &semaphore_get;
        syscall[SYSCALL_SEMAPHORE_INIT   ] = &semaphore_init;
        syscall[SYSCALL_SEMAPHORE_OP     ] = &semaphore_op;
        syscall[SYSCALL_BARRIER_GET      ] = &barrier_get;
        syscall[SYSCALL_BARRIER_INIT     ] = &barrier_init;
        syscall[SYSCALL_BARRIER_OP       ] = &barrier_op;
    }

    // jump from bootloader.
    // The beginning of everything >_< ~~~~~~~~~~~~~~
    int main()
    {
        init_all_stack();

        // init Shell (-_-!)
        init_shell();
        printk("> [INIT] Shell initialization succeeded.\n\r");

        // read CPU frequency
        time_base = sbi_read_fdt(TIMEBASE);

        // init interrupt (^_^)
        init_exception();
        printk("> [INIT] Interrupt processing initialization succeeded.\n\r");

        // init system call table (0_0)
        init_syscall();
        printk("> [INIT] System call initialized successfully.\n\r");

        // fdt_print(riscv_dtb);

        // init screen (QAQ)
        init_screen();
        printk("> [INIT] SCREEN initialization succeeded.\n\r");

        // TODO:
        // Setup timer interrupt and enable all interrupt
#ifdef INTERRUPT
        sbi_set_timer(get_ticks() + get_time_base() / 1000);
#endif

        while (1)
        {
            // (QAQQQQQQQQQQQ)
            // If you do non-preemptive scheduling, you need to use it
            // to surrender control do_scheduler();


#ifdef NONPREEMPT
            do_scheduler();
#endif
        };
        return 0;
    }
