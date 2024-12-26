#include "types.h"
#include "param.h"
#include "memlayout.h"
#include "riscv.h"
#include "defs.h"

volatile static int started = 0;

extern char sbss[], end[], stack0[];
extern int get_boot_mode();
extern void kputchar(char);


// start() jumps here in supervisor mode on all CPUs.

void
main()
{
  kputchar('K');
  if(cpuid() == 1){
    consoleinit();
    printfinit();
    printf("\r\n");
    printf("xv6 kernel is booting.\r\n");
    printf("\r\n");
    boot_device();
    kinit();         // physical page allocator
    kvminit();       // create kernel page table
    kvminithart();   // turn on paging
    procinit();      // process table
    trapinit();      // trap vectors
    trapinithart();  // install kernel trap vector
    plicinit();      // set up interrupt controller
    plicinithart();  // ask PLIC for device interrupts
    //board_init_f();  // board init 
    sdioinit();
    binit();         // buffer cache
    iinit();         // inode table
    fileinit();      // file table
    //virtio_disk_init(); // emulated hard disk
    userinit();      // first user process
    __sync_synchronize();
    started = 1;
    printf("sbss %p, ebss %p\r\n", sbss,  end);
  } else {
    while(started == 0)
      ;
    __sync_synchronize();
    printf("hart %d starting\r\n", cpuid());
    kvminithart();    // turn on paging
    trapinithart();   // install kernel trap vector
    plicinithart();   // ask PLIC for device interrupts
  }

  scheduler();        
}
