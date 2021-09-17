/* SPDX-License-Identifier: GPL-2.0-only */
/*
 * Copyright (C) 2012 Regents of the University of California
 */

#define CSR_SIE			0x104
#define CSR_SIP			0x144
#define KERNEL_STACK		0x50500000

.section ".entry_function","ax"
.global _start
_start:
  /* Mask all interrupts */
  csrw CSR_SIE, zero
  csrw CSR_SIP, zero

  /* Load the global pointer */
  .option push
  .option norelax
  la gp, __global_pointer$
  .option pop
  
  /* Clear BSS for flat non-ELF images */
  la t3, __bss_start
  la t4, __BSS_END__
  // if t3 >= BSS_END goto clear_done
  bge t3, t4, clear_done
clear:
  sw zero, (t3)
  add t3, t3, 4
  // if t3 < BSS_END goto clear
  blt t3, t4, clear
clear_done:

  /* setup C environment (set sp register)*/
  la sp, KERNEL_STACK
  /* Jump to the code in kernel.c*/
 
  la t1, main
  jr t1
loop:
  wfi
  j loop

