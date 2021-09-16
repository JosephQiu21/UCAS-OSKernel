#include <asm/sbidef.h>
#include <asm/sbiasm.h>

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

.option push
.option norelax
la gp, __global_pointer$
.option pop

main:
	// fence on all memory and I/O
	fence

    // read os_size to s1
    la s0, os_size_loc
    lh s1, 0(s0)
    li t1, 2
    sub s0, s0, t1
    lh s2, 0(s0)
    bge s1, s2, os1
    addi s1, s2, 0
os1:
    
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

	la a0, msg
	SBI_CALL SBI_CONSOLE_PUTSTR

	// Print message
    la a0, msg1
    SBI_CALL SBI_CONSOLE_PUTSTR
get:
    SBI_CALL SBI_CONSOLE_GETCHAR
    li  t0, 48
    li  t1, 49
    beq a0, t0, readA
    beq a0, t1, readB
    j   get
readA:
	// read kernel A
	la   s0, os_size_loc    // load address of section number
	la   a0, kernel
	lh   a1, 0(s0)
	li   a2, 1
	SBI_CALL SBI_SD_READ
    j code
readB:
    // read kernel B
    // os_size
    la   s0, os_size_loc
    li   t1, 2
    sub s1, s0, t1
    // number of blocks
    lh   a1, 0(s1)
    // block id
    lh   a2, 0(s0)
    addi a2, a2, 1
    // target address
    la   a0, kernel
    SBI_CALL SBI_SD_READ
    j code
code:
	// jump to kernel code
    la   t5, kernel_main
    jr   t5
    
// while(1) --> stop here
stop:
	j stop
	nop

.data

msg: .string "It's a bootloader...\n"
msg1: .string "Please choose kernel 0 or 1\n"