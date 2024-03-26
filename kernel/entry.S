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
	csrwi 0x7c1, 0
        la sp, stack0
        li a0, 1024*4
        csrr a1, mhartid
	mv t0, a1
        addi a1, a1, 1
        mul a0, a0, a1
        add sp, sp, a0
	# hart0 clears the bss
	beqz t0, clear_bss
        # jump to start() in start.c
        call start

clear_bss:
	la t0, bss_start
	la t1, ebss
	bgeu t0, t1, clear_bss_end
loop:
	sd zero, (t0)
	addi t0, t0, 8
	bltu t0, t1, loop

clear_bss_end:
	call start

spin:
        j spin

.globl kputchar

kputchar:
	li t0, 0x12440000 
	sb a0, 0x00(t0)

	ret

.globl mmu
mmu:
	csrw satp, a0
	sfence.vma zero, zero
	ret
