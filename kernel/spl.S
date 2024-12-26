#include "memlayout.h"
	
	# just a simple SPL.
        # starfive2 ROM loads SPL at 0x8000000
        # and causes each hart (i.e. CPU) to jump there.
	# four u74mc cores and one e24 core
        # kernel.ld causes the following code(SPL) to
        # be placed at 0x8000000, not at 0x80000000.

.section .text
.global _spl
_spl:
        # gets each hart id 
	# and closes it's interupts.
	csrr t0, mhartid
	mv tp, t0

	
	# closes each hart's interupts.
	# otherwise sth happens that we don't want.
	csrr a0, mstatus
	andi a0, a0, ~(1L << 3 | 1L << 1 | 1L << 0)
	csrw mstatus, a0 
	csrw mie, zero 

	# hart 0 is e24 core, wfi first.
	mv a0, t0
	beqz a0, e24park

	# hart 1 clears bss section.
	# and relocates text, data, bss section to ddr at 0x40000000.
	# other harts jump to ohart.
	li a1, 1
	sub a0, a0, a1 
	bnez a0, ohart 

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

	# move to 0x40000000 DDR.
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
	li t1, KERNBASE
	jr t1 
	

	

e24park:
	wfi
	j e24park

.align 4 
stacks:
        .skip   4096 * 8 # allocate space for all the harts stacks
