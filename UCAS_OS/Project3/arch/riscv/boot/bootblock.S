#include <asm/sbidef.h>
#include <asm/sbiasm.h>
#include <csr.h>

// #define BOOT_LOADER_SIG_OFFSET 0x1fe
// #define OS_SIZE_LOC (BOOT_LOADER_SIG_OFFSET - 2)
// #define OS_SIZE_LOC 0x1fc

.equ os_size_loc, 0x502001fc

// 1. kernel address (move kernel to here ~)
.equ kernel, 0x50200000

// 2. kernel main address (jmp here to start kernel main!)
.equ kernel_main, 0x50200000

.text
.global main

main:
	// fence on all memory and I/O
	fence

    // read os_size to s1
    la s0, os_size_loc
    lh s1, 0(s0)

    // calculate bootblock target address
    la s2, kernel
    li t0, 0
    // if(i >= os_size) goto done
    bge t0, s1, done
loop:
    addi s2, s2, 512
    addi t0, t0, 1
    // if (i < os_size) goto loop
    blt t0, s1, loop
done: 

    // copy bootblock to target address
    addi a0, s2, 0
    addi a1, zero, 1
    addi a2, zero, 0
    SBI_CALL SBI_SD_READ

    fence.i

    // jump to new bootblock and continue running
    la t3, new
    la t4, kernel
    sub t3, t3, t4
    add t3, t3, s2 
    jr t3

new:
	// print a \n to avoid system output error...
	li a0, 10
	SBI_CALL SBI_CONSOLE_PUTCHAR

	// 1) task1 call BIOS print string "It's a bootloader..."
	la a0, msg
	SBI_CALL SBI_CONSOLE_PUTSTR
	
	// 2) task2 call BIOS read kernel in SD card and jump to kernel start
	// read SD card
	la   s0, os_size_loc    // load address of section number
	la   a0, kernel
	lh   a1, 0(s0)
	li   a2, 1
	SBI_CALL SBI_SD_READ

    fence.i
	// jump to kernel code
    la   t5, kernel_main
    jr   t5
    
// while(1) --> stop here
stop:
	j stop
	nop

.data

msg: .string "It's a bootloader...\n\r"

