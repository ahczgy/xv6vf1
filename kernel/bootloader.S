#include "memlayout.h"

        # qemu -kernel loads the kernel at 0x80000000
        # and causes each hart (i.e. CPU) to jump there.
        # kernel.ld causes the following code to
        # be placed at 0x80000000.
.section .text
.global _bootloader
_bootloader:
        # set up a stack for C.
        # stack0 is declared in start.c,
        # with a 4096-byte stack per CPU.
        # sp = stack0 + (hartid * 4096)
	csrr t0, mhartid
	mv tp, t0

        la sp, stacks
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

	# hartid 0 is e24 core, wfi first.
	li a0, 1
	sub a1, a1, a0 
	beqz a1, e24park

	# hart1 do clear bss and relocate, other hart waiting for jump to _entry.
	sub a1, a1, a0 
	bnez a1, ohart 

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

	# move to 0x80000000 DDR.
	la a0, _entry
	la a1, end
	li a2, KERNBASE
	bgeu a0, a1, 4f
3:

	ld a3, (a0)
	sd a3, (a2)
	addi a0, a0, 8
	addi a2, a2, 8
	bltu a0, a1, 3b 
4:
	

ohart:
	la t4, _entry
	jr t4 
	

	

e24park:
	wfi
	j e24park

.align 4 
stacks:
        .skip   4096 * 8 # allocate space for all the harts stacks
