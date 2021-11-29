#include <asm/sbidef.h>
#include <asm/sbiasm.h>
#include <csr.h>

.equ os_size_loc, 0x502001fc

// 1. kernel address (move kernel to here ~) 
.equ kernel, 0x50301000

// 2. kernel main address (jmp here to start kernel main!) 
.equ kernel_main, 0x50301000

.text 
.global main 
main:
	// fence on all memory and I/O
	fence


	// Load the global pointer(can see gp in disassembly) 
	.option push
	.option norelax
	la gp, __global_pointer$
	.option pop

	// output a \n
	li a0, 10
	SBI_CALL SBI_CONSOLE_PUTCHAR

	// your code 
    la a0, msg
    SBI_CALL SBI_CONSOLE_PUTSTR 
    
    la t0, os_size_loc
	lh t0, (t0)         // number of all sectors
	la a0, kernel       // kernel_addr    
	li a1, 64           // 64 sectors a time   
	li a2, 1            // start of first sector
    move t1, a0              
	move t2, a1
    slli t2, t2, 9      // size: 64 * 512 B
read_sectors:
    bleu t0, a1, end_read_sectors
	SBI_CALL SBI_SD_READ
	add a0, t2, t1      // a0: start copy addr
	move t1, a0        
	add a2, a2, a1      // a2: start copy sector
	sub t0, t0, a1      // t0: number of sectors left

end_read_sectors:
	move a1, t0
	SBI_CALL SBI_SD_READ

    // your code 
    la a0, msg1
    SBI_CALL SBI_CONSOLE_PUTSTR 

    fence.i
    
    la t0, kernel_main
    jr t0

wait_for_wakeup:
	wfi
	j wait_for_wakeup 

stop:
	j stop
	nop

.data

msg: .string "It's a bootloader...\n\r"

msg1: .string "Bootloading finished!\n\r"