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
	csrr a0, mstatus
	andi a0, a0, ~(1L << 3 | 1L << 1 | 1L << 0)
	csrw mstatus, a0 
	csrw mie, zero 

	# hartid 0 is e24 core, so wfi.
	li a0, 1
	sub a1, a1, a0 
	beqz a1, e24park

	sub a1, a1, a0 
	bnez a1, park

	la a0, _entry
	la a1, end
	li a2, KERNBASE
	bgeu a0, a1, 0f
loop:

	ld a3, (a0)
	sd a3, (a2)
	addi a0, a0, 8
	addi a1, a1, 8
	bltu a0, a1, loop 
0:
	
	li t2, '\r'
	li t1, 0x10000000L 
	sb t2, 0x00(t1)

	li t2, '\n'
	li t1, 0x10000000L 
	sb t2, 0x00(t1)

	li t2, 'B'
	li t1, 0x10000000L 
	sb t2, 0x00(t1)
	

	
	# set bss section to zero.
	la a0, sbss
	la a1, end
	bgeu a0, a1, 2f 
1:
	sd zero, (a0)
	addi a0, a0, 8
	bltu a0, a1, 1b 

2:
	# never return.
	call start


park:

	li t3, 2 
	bgeu t0, t3, 3f
	li t2, 'B'
	j 6f
3:
	li t3, 3 
	bgeu t0, t3, 4f
	li t2, 'C'
	j 6f
4:
	li t3, 4 
	bgeu t0, t3, 5f
	li t2, 'D'
	j 6f
5:
	li t2, 'E'
	
6:	
	
	li t1, 0x10000000L
	sb t2, 0x00(t1)
	
	call start
	
	wfi
        j park 

e24park:
	wfi
	j e24park


.globl kputchar
kputchar:
	mv t2, a0
	li t1, 0x10000000L 
	sb t2, 0x00(t1)

	ret
