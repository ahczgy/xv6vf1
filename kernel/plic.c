#include "types.h"
#include "param.h"
#include "memlayout.h"
#include "riscv.h"
#include "defs.h"


#define BITS_OF_UINT32 (32)

//
// the riscv Platform Level Interrupt Controller (PLIC).
//

void
plicinit(void)
{
  // set desired IRQ priorities non-zero (otherwise disabled).
  *(uint32*)(PLIC + SDIO1_IRQ*4) = 1;
  *(uint32*)(PLIC + GPIO_IRQ*4) = 1;
  *(uint32*)(PLIC + UART0_IRQ*4) = 1;
  *(uint32*)(PLIC + SPI0_IRQ*4) = 1;
 // *(uint32*)(PLIC + VIRTIO0_IRQ*4) = 1;
}

void
plicinithart(void)
{
  int hart = cpuid();
  
  uint32 *plic = (uint32 *)PLIC_SENABLE(hart);

  // set enable bits for this hart's S-mode
  // for the uart3 and gpio.
  // *plic = (1 << SDIO0_IRQ) | (1 << GPIO_IRQ);
  plic[SDIO1_IRQ / BITS_OF_UINT32] |= (1 << (SDIO1_IRQ % BITS_OF_UINT32));
  plic[GPIO_IRQ / BITS_OF_UINT32] |= (1 << (GPIO_IRQ % BITS_OF_UINT32));
  plic[UART0_IRQ / BITS_OF_UINT32] |= (1 << (UART0_IRQ % BITS_OF_UINT32));
  plic[SPI0_IRQ / BITS_OF_UINT32] |= (1 << (SPI0_IRQ % BITS_OF_UINT32));
  //printf("%d, %d\r\n", GPIO_IRQ / BITS_OF_UINT32, GPIO_IRQ % BITS_OF_UINT32);

  //plic += (UART0_IRQ / BITS_OF_UINT32);
  //*plic = (1 << (UART0_IRQ % BITS_OF_UINT32));


  // set this hart's S-mode priority threshold to 0.
  *(uint32*)PLIC_SPRIORITY(hart) = 0;
}

// ask the PLIC what interrupt we should serve.
int
plic_claim(void)
{
  int hart = cpuid();
  int irq = *(uint32*)PLIC_SCLAIM(hart);
  return irq;
}

// tell the PLIC we've served this IRQ.
void
plic_complete(int irq)
{
  int hart = cpuid();
  *(uint32*)PLIC_SCLAIM(hart) = irq;
}
