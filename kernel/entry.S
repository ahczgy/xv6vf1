#include "memlayout.h"

        # qemu -kernel loads the kernel at 0x80000000
        # and causes each hart (i.e. CPU) to jump there.
        # kernel.ld causes the following code to
        # be placed at 0x80000000.
.section .text
.global _entry
_entry:
        # set up a stack for C.
        # stack0 is declared in start.c,
        # with a 4096-byte stack per CPU.
        # sp = stack0 + (hartid * 4096)
	csrr t0, mhartid
	mv tp, t0

        la sp, stack0
        li a0, 1024*4
        mv a1, tp 
        addi a1, a1, 1
        mul a0, a0, a1
        add sp, sp, a0
	
	
	# disable all interupts.
	#csrr a0, mstatus
	#andi a0, a0, ~(1L << 3 | 1L << 1 | 1L << 0)
	#csrw mstatus, a0 
	csrw mie, zero 

	# hartid 0 is e24 core, so wfi.
	li a0, 1
	sub a1, a1, a0 
	beqz a1, e24park

	sub a1, a1, a0 
	bnez a1, callstart 

	
	# set bss section to zero.
	la a0, sbss
	la a1, end
	bgeu a0, a1, callstart 
1:
	sd zero, (a0)
	addi a0, a0, 8
	bltu a0, a1, 1b 
	call start

callstart:
	call start
	

e24park:
	wfi
	j e24park


.globl kputchar
kputchar:
	mv t2, a0
	li t1, 0x10000000L 
	sb t2, 0x00(t1)

	ret
