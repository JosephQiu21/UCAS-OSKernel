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
#include <os/syscall.h>
#include <test.h>
#include <csr.h>

#define LOCK_TEST
// #define SCHEDULE_TEST

extern void ret_from_exception();
extern void __global_pointer$();
list_head ready_queue, block_queue;

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

    // Update kernel_sp in pcb
    pcb -> kernel_sp = kernel_stack - sizeof(regs_context_t) - sizeof(switchto_context_t);

    // set switch-to context
    switchto_context_t *st_regs = (switchto_context_t *)pcb -> kernel_sp;

    // Set ra & sp
    st_regs -> regs[0] = entry_point;
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

static void init_syscall(void)
{
    // initialize system call table.
}

// jump from bootloader.
// The beginning of everything >_< ~~~~~~~~~~~~~~
int main()
{
    // init Process Control Block (-_-!)
    init_pcb();
    printk("> [INIT] PCB initialization succeeded.\n\r");

    // read CPU frequency
    // time_base = sbi_read_fdt(TIMEBASE);

    // init interrupt (^_^)
    // init_exception();
    // printk("> [INIT] Interrupt processing initialization succeeded.\n\r");

    // init system call table (0_0)
    // init_syscall();
    // printk("> [INIT] System call initialized successfully.\n\r");

    // fdt_print(riscv_dtb);

    // init screen (QAQ)
    init_screen();
    printk("> [INIT] SCREEN initialization succeeded.\n\r");

    // TODO:
    // Setup timer interrupt and enable all interrupt

    while (1)
    {
        // (QAQQQQQQQQQQQ)
        // If you do non-preemptive scheduling, you need to use it
        // to surrender control do_scheduler();
        // enable_interrupt();
        // __asm__ __volatile__("wfi\n\r":::);
        do_scheduler();
    };
    return 0;
}
