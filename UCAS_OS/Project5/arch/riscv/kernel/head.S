/* SPDX-License-Identifier: GPL-2.0-only */
/*
 * Copyright (C) 2012 Regents of the University of California
 */

#include <asm.h>
#include <csr.h>
#include <asm/sbidef.h>
#include <asm/sbiasm.h>

.equ boot_stack_top_base, 0x50202000

.section ".entry_function","ax"
ENTRY(_start)
  /* Mask all interrupts */
  csrw CSR_SIE, zero
  csrw CSR_SIP, zero

  /*
   * Disable FPU to detect illegal usage of
   * floating point in kernel space
   */
  li t0, SR_FS
  csrc sstatus, t0

  /* Load the global pointer */
  .option push
  .option norelax
  la gp, __global_pointer$
  .option pop

  /* Clear BSS for flat non-ELF images */
  la a3, __bss_start
  la a4, __BSS_END__
  ble a4, a3, clear_bss_done
clear_bss:
  sd zero, (a3)
  add a3, a3, RISCV_SZPTR
  blt a3, a4, clear_bss
clear_bss_done:
  la a0, msg2
  SBI_CALL SBI_CONSOLE_PUTSTR 
  /* setup C environment */
  li sp, boot_stack_top_base
  li a0, 0
  call boot_kernel

loop:
  wfi
  j loop

msg2: .string "Clear BSS done!\n\r"

END(_start)

