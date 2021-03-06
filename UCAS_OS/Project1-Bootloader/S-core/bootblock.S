#include <asm/sbidef.h>
#include <asm/sbiasm.h>

// #define BOOT_LOADER_SIG_OFFSET 0x1fe
// #define OS_SIZE_LOC (BOOT_LOADER_SIG_OFFSET - 2)
// #define OS_SIZE_LOC 0x1fc

.equ os_size_loc, 0x502001fc

// 1. kernel address (move kernel to here ~)
.equ kernel, 0x50201000

// 2. kernel main address (jmp here to start kernel main!)
.equ kernel_main, 0x50201000

.text
.global main

main:
	// fence on all memory and I/O
	fence

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
    la   t1, kernel_main
	jr   t1
	
// while(1) --> stop here
stop:
	j stop
	nop

.data

msg: .string "It's a bootloader...\n\r"

