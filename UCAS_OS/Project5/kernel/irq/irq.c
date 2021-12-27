#include <os/irq.h>
#include <os/time.h>
#include <os/sched.h>
#include <os/string.h>
#include <os/stdio.h>
#include <assert.h>
#include <sbi.h>
#include <screen.h>
#include <csr.h>
#include <pgtable.h>
#include <emacps/xil_utils.h>
#include <emacps/xemacps_example.h>
#include <plic.h>
#include <net.h>

handler_t irq_table[IRQC_COUNT];
handler_t exc_table[EXCC_COUNT];
uintptr_t riscv_dtb;

void reset_irq_timer()
{
    screen_reflush();
    check_timer();
    if (net_poll_mode == INT_MODE) {
        check_net_queue();
    }
    // note: use sbi_set_timer
    sbi_set_timer(get_ticks() + time_base / 200);
    // remember to reschedule
    do_scheduler();
}

void check_net_queue()
{
    u32 reg_txsr;
    u32 reg_rxsr;
    if (!is_list_empty(&net_recv_queue)) {
        printk("[DEBUG] Checking recv queue...\n");
        if (((reg_rxsr = XEmacPs_ReadReg(EmacPsInstance.Config.BaseAddress, XEMACPS_RXSR_OFFSET)) & XEMACPS_RXSR_FRAMERX_MASK) == XEMACPS_RXSR_FRAMERX_MASK) {
            // Clear rxsr.framerx
            XEmacPs_WriteReg(EmacPsInstance.Config.BaseAddress, XEMACPS_RXSR_OFFSET, reg_rxsr | XEMACPS_RXSR_FRAMERX_MASK);
            do_unblock(dequeue(&net_recv_queue));
        }
        Xil_DCacheFlushRange(0, 64);
    }
    if (!is_list_empty(&net_send_queue)) {
        printk("[DEBUG] Checking send queue...\n");
        if (((reg_txsr = XEmacPs_ReadReg(EmacPsInstance.Config.BaseAddress, XEMACPS_TXSR_OFFSET)) & XEMACPS_TXSR_TXCOMPL_MASK) == XEMACPS_TXSR_TXCOMPL_MASK) {
            // Clear txsr.txcompl
            XEmacPs_WriteReg(EmacPsInstance.Config.BaseAddress, XEMACPS_TXSR_OFFSET, reg_txsr | XEMACPS_TXSR_TXCOMPL_MASK);
            do_unblock(dequeue(&net_send_queue));
        }
        Xil_DCacheFlushRange(0, 64);
    }
}

void interrupt_helper(regs_context_t *regs, uint64_t stval, uint64_t cause)
{
    // call corresponding handler by the value of `cause`
    // handler_t *table = (cause >> 63)? irq_table : exc_table;
    // uint64_t exec_code = cause & ~(1 << 63);
    // table[exec_code](regs, stval, cause);

    if (cause >> 63) {
        irq_table[cause & ~SCAUSE_IRQ_FLAG](regs, stval, cause);
    } else {
        exc_table[cause & ~SCAUSE_IRQ_FLAG](regs, stval, cause);
    }
    // uint64_t interrupt = cause & SCAUSE_IRQ_FLAG;
    // cause &= ~SCAUSE_IRQ_FLAG;
    // if (interrupt) {
    //     irq_table[cause](regs, stval, cause);
    // } else {
    //     exc_table[cause](regs, stval, cause);
    // }
}

void handle_int(regs_context_t *regs, uint64_t interrupt, uint64_t cause)
{
    reset_irq_timer();
}

void handle_irq(regs_context_t *regs, int irq)
{
    // TODO: 
    // handle external irq from network device
    // let PLIC know that handle_irq has been finished
}

void init_exception()
{
    /* initialize irq_table and exc_table */
    int i;
    for (i = 0; i < IRQC_COUNT; i++) irq_table[i] = &handle_other;
    for (i = 0; i < EXCC_COUNT; i++) exc_table[i] = &handle_other;
    irq_table[IRQC_S_SOFT ] = &sbi_clear_ipi;
    irq_table[IRQC_S_TIMER] = &handle_int;
    irq_table[IRQC_S_EXT  ] = &plic_handle_irq;
    exc_table[EXC_INST_PAGE_FAULT] = &handle_inst_page_fault;
    exc_table[EXC_LOAD_PAGE_FAULT] = &handle_load_page_fault;
    exc_table[EXC_STORE_PAGE_FAULT] = &handle_store_page_fault;
    exc_table[EXCC_SYSCALL] = &handle_syscall;
    setup_exception();
}

void handle_inst_page_fault(regs_context_t *regs, uint64_t stval, uint64_t cause){
    PTE* pte = find_pte(stval, current_running->pgdir);

    if(pte != 0) {
        set_attribute(pte, _PAGE_DIRTY | _PAGE_ACCESSED);
        local_flush_tlb_all();
    } else {
        handle_other(regs, stval, cause);
    }
}

void handle_load_page_fault(regs_context_t *regs, uint64_t stval, uint64_t cause){
    handle_ldst_pagefault(regs, stval, cause, 0);
}

void handle_store_page_fault(regs_context_t *regs, uint64_t stval, uint64_t cause){
    handle_ldst_pagefault(regs, stval, cause, 1);
}

void handle_ldst_pagefault(regs_context_t *regs, uint64_t stval, uint64_t cause, int load)
{
    uint64_t cpu_id = get_current_cpu_id();
    PTE* pte = find_pte(stval, current_running->pgdir);

    if(pte == 0) { 
        alloc_page_helper(stval, current_running->pgdir, 1);
        pte = find_pte(stval, current_running->pgdir);
        set_attribute(pte, load ? _PAGE_ACCESSED : (_PAGE_ACCESSED|_PAGE_DIRTY));
        local_flush_tlb_all();
    } else if(*pte & _PAGE_PRESENT) {
        set_attribute(pte, load ? _PAGE_ACCESSED : (_PAGE_ACCESSED|_PAGE_DIRTY));
        local_flush_tlb_all();
    }else{ //TODO: on disk
        
    }

}

void handle_other(regs_context_t *regs, uint64_t stval, uint64_t cause)
{
    char* reg_name[] = {
        "zero "," ra  "," sp  "," gp  "," tp  ",
        " t0  "," t1  "," t2  ","s0/fp"," s1  ",
        " a0  "," a1  "," a2  "," a3  "," a4  ",
        " a5  "," a6  "," a7  "," s2  "," s3  ",
        " s4  "," s5  "," s6  "," s7  "," s8  ",
        " s9  "," s10 "," s11 "," t3  "," t4  ",
        " t5  "," t6  "
    };
    for (int i = 0; i < 32; i += 3) {
        for (int j = 0; j < 3 && i + j < 32; ++j) {
            printk("%s : %016lx ",reg_name[i+j], regs->regs[i+j]);
        }
        printk("\n\r");
    }
    printk("sstatus: 0x%lx sbadaddr: 0x%lx scause: %lx\n\r",
           regs->sstatus, regs->sbadaddr, regs->scause);
    printk("stval: 0x%lx cause: %lx\n\r",
           stval, cause);
    printk("sepc: 0x%lx\n\r", regs->sepc);
    // printk("mhartid: 0x%lx\n\r", get_current_cpu_id());

    uintptr_t fp = regs->regs[8], sp = regs->regs[2];
    printk("[Backtrace]\n\r");
    printk("  addr: %lx sp: %lx fp: %lx\n\r", regs->regs[1] - 4, sp, fp);
    // while (fp < USER_STACK_ADDR && fp > USER_STACK_ADDR - PAGE_SIZE) {
    while (fp > 0x10000) {
        uintptr_t prev_ra = *(uintptr_t*)(fp-8);
        uintptr_t prev_fp = *(uintptr_t*)(fp-16);

        printk("  addr: %lx sp: %lx fp: %lx\n\r", prev_ra - 4, fp, prev_fp);

        fp = prev_fp;
    }

    assert(0);
}
