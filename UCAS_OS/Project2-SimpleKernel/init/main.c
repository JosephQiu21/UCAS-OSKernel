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
#include <test.h>
#include <csr.h>

// #define LOCK_TEST
// #define SCHEDULE_TEST
// #define TIMER_TEST
#define INTERRUPT_TEST

#define INTERRUPT
//#define NONPREEMPT

extern void ret_from_exception();
extern void __global_pointer$();
list_head ready_queue, block_queue, sleep_queue;

static void init_pcb_stack(
    ptr_t kernel_stack, ptr_t user_stack, ptr_t entry_point,
    pcb_t *pcb)
{
    int i;

    // set regs context
    regs_context_t *pt_regs = (regs_context_t *)(kernel_stack - sizeof(regs_context_t));
    for (i = 0; i < 32; i++)
    {
        pt_regs -> regs[i] = 0;
    }
    pt_regs -> regs[2] = user_stack;
    pt_regs -> regs[3] = (reg_t)__global_pointer$;
    pt_regs -> sepc = entry_point;
    pt_regs -> sstatus = 0;
    pt_regs -> scause = 0;
    pt_regs -> sbadaddr = 0;

    // Update kernel_sp in pcb
    pcb -> kernel_sp = kernel_stack - sizeof(regs_context_t) - sizeof(switchto_context_t);

    // set switch-to context
    switchto_context_t *st_regs = (switchto_context_t *)pcb -> kernel_sp;

    // Set ra & sp
    st_regs -> regs[0] = &ret_from_exception;
    st_regs -> regs[1] = user_stack;
    for (i = 2; i < 14; i++)
    {
        st_regs->regs[i] = 0;
    }
    
}

static void init_pcb()
{
    int i;
    list_init(&ready_queue);
    list_init(&sleep_queue);

    // initialize pcb using task info
    task_info_t *task;

    // Tasks
    #ifdef SCHEDULE_TEST
    for (i = 0; i < num_sched1_tasks; i++)
    {
        task = sched1_tasks[i];
    #endif

    #ifdef LOCK_TEST
    for (i = 0; i < num_lock_tasks; i++)
    {
        task = lock_tasks[i];
    #endif

    #ifdef TIMER_TEST
    for (i = 0; i < num_timer_tasks; i++)
    {
        task = timer_tasks[i];
    #endif

    #ifdef INTERRUPT_TEST
    for (i = 0; i < num_sched2_tasks + num_lock2_tasks; i++)
    {
        task = (i < num_sched2_tasks)? sched2_tasks[i] : lock2_tasks[i - num_sched2_tasks];
    #endif

        // Alloc stack
        pcb[i].kernel_sp = allocPage(1);
        pcb[i].user_sp = allocPage(1);
        init_pcb_stack(pcb[i].kernel_sp, pcb[i].user_sp, task->entry_point, &pcb[i]);

        // Set pcb properties
        pcb[i].pid = i + 1;
        pcb[i].status = TASK_READY;
        pcb[i].type = task->type;
        enqueue(&ready_queue, &(pcb[i].list));
    }
    current_running = &pid0_pcb;
}

void error_syscall(void){
    printk("> Undefined syscall.\n\r");
}

static void init_syscall(void)
{
    // initialize system call table.
    for (int i = 0; i < NUM_SYSCALLS; i++) syscall[i] = &error_syscall;
    syscall[SYSCALL_SLEEP       ] = &do_sleep;
    syscall[SYSCALL_WRITE       ] = &screen_write;
    syscall[SYSCALL_CURSOR      ] = &screen_move_cursor;
    syscall[SYSCALL_REFLUSH     ] = &screen_reflush;
    syscall[SYSCALL_GET_TIMEBASE] = &get_time_base;
    syscall[SYSCALL_GET_TICK    ] = &get_ticks;
    syscall[SYSCALL_YIELD       ] = &do_scheduler;
    syscall[SYSCALL_LOCK_INIT   ] = &do_mutex_lock_init;
    syscall[SYSCALL_LOCK_ACQUIRE] = &do_mutex_lock_acquire;
    syscall[SYSCALL_LOCK_RELEASE] = &do_mutex_lock_release;
}

// jump from bootloader.
// The beginning of everything >_< ~~~~~~~~~~~~~~
int main()
{
    // init Process Control Block (-_-!)
    init_pcb();
    printk("> [INIT] PCB initialization succeeded.\n\r");

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
    sbi_set_timer(get_ticks() + get_time_base() / 500);
#endif

    while (1)
    {
        // (QAQQQQQQQQQQQ)
        // If you do non-preemptive scheduling, you need to use it
        // to surrender control do_scheduler();
#ifdef INTERRUPT
        enable_interrupt();
        __asm__ __volatile__("wfi\n\r":::);
#endif

#ifdef NONPREEMPT
        do_scheduler();
#endif
    };
    return 0;
}
