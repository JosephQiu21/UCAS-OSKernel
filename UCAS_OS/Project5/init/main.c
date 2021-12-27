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
#include <plic.h>
#include <emacps/xemacps_example.h>
#include <net.h>
#include <assert.h>
#include <common.h>
#include <os/irq.h>
#include <os/mm.h>
#include <os/list.h>
#include <os/sched.h>
#include <os/ioremap.h>
#include <screen.h>
#include <sbi.h>
#include <os/time.h>
#include <os/lock.h>
#include <os/syscall.h>
#include <os/sync.h>
#include <os/elf.h>
#include <pgtable.h>
#include <csr.h>
#include <user_programs.h>


extern void ret_from_exception();
extern void __global_pointer$();


int find_freepcb();

void init_pcb_stack(
    ptr_t kernel_stack, ptr_t user_stack, ptr_t entry_point,
    pcb_t *pcb, void *argc, char *argv[])
{
    int i;

    // set regs context
    regs_context_t *pt_regs = (regs_context_t *)(kernel_stack - sizeof(regs_context_t));
    for (i = 0; i < 32; i++)
    {
        pt_regs->regs[i] = 0;
    }
    pt_regs->regs[1] = entry_point;
    pt_regs->regs[2] = user_stack;
    pt_regs->regs[3] = (reg_t)__global_pointer$;
    pt_regs->regs[10] = argc;
    pt_regs->regs[11] = argv;
    pt_regs->sepc = entry_point;
    pt_regs->sstatus = SR_SUM;
    pt_regs->scause = 0;
    pt_regs->sbadaddr = 0;

    // Update kernel_sp in pcb
    pcb -> kernel_sp = kernel_stack - sizeof(regs_context_t) - sizeof(switchto_context_t);
    pcb -> user_sp = user_stack;

    // set switch-to context
    switchto_context_t *st_regs = (switchto_context_t *)pcb->kernel_sp;

    // Set ra & sp
    st_regs->regs[0] = &ret_from_exception;
    st_regs->regs[1] = user_stack;
    for (i = 2; i < 14; i++)
    {
        st_regs->regs[i] = 0;
    }
}

// pid_t do_spawn(task_info_t *task, void* arg, spawn_mode_t mode){
//     int i = find_freepcb();
//     if (i == -1) {
//         printf("> [ERROR] Unable to spawn another task.");
//         return -1;
//     }
//     pcb[i].status = TASK_READY;
//     pcb[i].pid = i + 1;
//     pcb[i].mode = mode;
//     pcb[i].type = task -> type;
//     pcb[i].kernel_sp = kernel_stack[i];
//     pcb[i].user_sp = user_stack[i];
//     list_init(&pcb[i].wait_list);
//     init_pcb_stack(pcb[i].kernel_sp, pcb[i].user_sp, task -> entry_point, arg, &pcb[i]);
//     enqueue(&ready_queue, &(pcb[i].list));
//     return i + 1;
// }

static void init_shell(){
    extern pid_t process_id;
    /* pid/status/type */
    pcb[0].pid = 1;
    pcb[0].status = TASK_READY;
    pcb[0].type = USER_PROCESS;
    pcb[0].mode = AUTO_CLEANUP_ON_EXIT;

    /* Page Directory */
    pcb[0].pgdir = allocPage();
    clear_pgdir(pcb[0].pgdir);
    share_pgtable(pcb[0].pgdir, pa2kva(PGDIR_PA));

    list_init(&ready_queue);

    /* Register context */
    pcb[0].kernel_sp = alloc_page_helper(KERNEL_STACK_ADDR - PAGE_SIZE, pcb[0].pgdir, 0) + PAGE_SIZE;
    // allocPage() + PAGE_SIZE;
    // alloc_page_helper(KERNEL_STACK_ADDR - PAGE_SIZE, pcb[0].pgdir, KERNEL_MODE);
    pcb[0].user_sp = alloc_page_helper(USER_STACK_ADDR - PAGE_SIZE, pcb[0].pgdir, 1) + PAGE_SIZE;
    // alloc_page_helper(USER_STACK_ADDR - PAGE_SIZE, pcb[0].pgdir, 1);
    // USER_STACK_ADDR - 0x100;
    // alloc_page_helper(USER_STACK_ADDR - PAGE_SIZE, pcb[0].pgdir, USER_MODE);
    pcb[0].user_sp = USER_STACK_ADDR - 0x80;
    /* Enqueue */
    enqueue(&ready_queue, &pcb[0].list);

    pcb[0].cursor_x = 0;
    pcb[0].cursor_y = 0;

    ptr_t shell_entry = (ptr_t)load_elf(_elf___test_test_shell_elf,
                                        _length___test_test_shell_elf, pcb[0].pgdir, elf_alloc_page_helper);
    init_pcb_stack(pcb[0].kernel_sp, pcb[0].user_sp, shell_entry, &pcb[0], NULL, NULL);

    
    current_running = &pid0_pcb;
}

int find_freepcb() {
    int i;
    for (i = 1; i < NUM_MAX_TASK; i++){
        if (pcb[i].pid == 0)
            return i;
    }
    return -1;
}

void cancel_map()
{
    uint64_t va = 0x50200000;
    uint64_t pgdir = 0xffffffc05e000000;
    uint64_t vpn2 = va >> (NORMAL_PAGE_SHIFT + PPN_BITS + PPN_BITS);
    uint64_t vpn1 = (vpn2 << PPN_BITS) ^
                    (va >> (NORMAL_PAGE_SHIFT + PPN_BITS));
    PTE *pgdir_1 = (PTE *)pgdir + vpn2;
    PTE *pgdir_0 = (PTE *)get_va(*pgdir_1) + vpn1;
    *pgdir_1 = 0;
    *pgdir_0 = 0;
}

// static void init_shell()
// {
//     list_init(&ready_queue);
//     list_init(&sleep_queue);
//     pcb[0].pid = 1;
//     pcb[0].status = TASK_READY;
//     pcb[0].type = USER_PROCESS;
//     pcb[0].kernel_sp = kernel_stack[0];
//     pcb[0].user_sp = user_stack[0];
//     init_pcb_stack(pcb[0].kernel_sp, pcb[0].user_sp, &test_shell, NULL, &pcb[0]);
//     enqueue(&ready_queue, &(pcb[0].list));
//     current_running = &pid0_pcb;
// }

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
        // syscall[SYSCALL_SPAWN            ] = &do_spawn;
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
        syscall[SYSCALL_MBOX_OPEN        ] = &do_mbox_open;
        syscall[SYSCALL_MBOX_CLOSE       ] = &do_mbox_close;
        syscall[SYSCALL_MBOX_SEND        ] = &do_mbox_send;
        syscall[SYSCALL_MBOX_RECV        ] = &do_mbox_recv;
        syscall[SYSCALL_EXEC             ] = &do_exec;
        syscall[SYSCALL_LS               ] = &do_ls;
        syscall[SYSCALL_MTHREAD_CREATE   ] = &do_mthread_create;
        syscall[SYSCALL_NET_RECV         ] = &do_net_recv;
        syscall[SYSCALL_NET_SEND         ] = &do_net_send;
        syscall[SYSCALL_NET_IRQ_MODE     ] = &do_net_irq_mode;
    }

    // jump from bootloader.
    // The beginning of everything >_< ~~~~~~~~~~~~~~
    int main()
    {
        cancel_map();
        // init shell (-_-!)
        init_shell();
        printk("> [INIT] Shell initialization succeeded.\n\r");

        // read CPU frequency
        time_base = sbi_read_fdt(TIMEBASE);
        uint32_t slcr_bade_addr = 0, ethernet_addr = 0;

        // get_prop_u32(_dtb, "/soc/slcr/reg", &slcr_bade_addr);
        slcr_bade_addr = sbi_read_fdt(SLCR_BADE_ADDR);
        printk("[slcr] phy: 0x%x\n\r", slcr_bade_addr);

        // get_prop_u32(_dtb, "/soc/ethernet/reg", &ethernet_addr);
        ethernet_addr = sbi_read_fdt(ETHERNET_ADDR);
        printk("[ethernet] phy: 0x%x\n\r", ethernet_addr);

        uint32_t plic_addr = 0;
        // get_prop_u32(_dtb, "/soc/interrupt-controller/reg", &plic_addr);
        plic_addr = sbi_read_fdt(PLIC_ADDR);
        printk("[plic] plic: 0x%x\n\r", plic_addr);

        uint32_t nr_irqs = sbi_read_fdt(NR_IRQS);
        // get_prop_u32(_dtb, "/soc/interrupt-controller/riscv,ndev", &nr_irqs);
        printk("[plic] nr_irqs: 0x%x\n\r", nr_irqs);

        XPS_SYS_CTRL_BASEADDR =
            (uintptr_t)ioremap((uint64_t)slcr_bade_addr, NORMAL_PAGE_SIZE);
        xemacps_config.BaseAddress =
            (uintptr_t)ioremap((uint64_t)ethernet_addr, NORMAL_PAGE_SIZE);
        uintptr_t _plic_addr =
            (uintptr_t)ioremap((uint64_t)plic_addr, 0x4000*NORMAL_PAGE_SIZE);
        // XPS_SYS_CTRL_BASEADDR = slcr_bade_addr;
        // xemacps_config.BaseAddress = ethernet_addr;
        xemacps_config.DeviceId        = 0;
        xemacps_config.IsCacheCoherent = 0;

        printk(
            "[slcr_bade_addr] phy:%x virt:%lx\n\r", slcr_bade_addr,
            XPS_SYS_CTRL_BASEADDR);
        printk(
            "[ethernet_addr] phy:%x virt:%lx\n\r", ethernet_addr,
            xemacps_config.BaseAddress);
        printk("[plic_addr] phy:%x virt:%lx\n\r", plic_addr, _plic_addr);
        plic_init(_plic_addr, nr_irqs);
        
        long status = EmacPsInit(&EmacPsInstance);
        if (status != XST_SUCCESS) {
            printk("Error: initialize ethernet driver failed!\n\r");
            assert(0);
        }


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

        // Setup timer interrupt and enable all interrupt
        sbi_set_timer(get_ticks() + time_base / 200);
        enable_interrupt();

        net_poll_mode = 1;
        // xemacps_example_main();
        

        // do_scheduler();

        while (1) {};
        return 0;
    }
