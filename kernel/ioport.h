#define NULL 		0
#define FALSE		0
#define TRUE		1

#ifndef BIT
#define BIT(nr) (1UL << (nr))
#endif


#define INB(port) (uint8)(*((volatile uint8 *)(port)))
#define INS(port) (uint16)(*((volatile uint16 *)(port)))
#define INW(port) (uint32)(*((volatile uint32 *)(port)))

#define OUTB(port, v) (*((volatile uint8 *)(port)) = ((uint8)(v)))
#define OUTS(port, v) (*((volatile uint16 *)(port)) = ((uint16)(v)))
#define OUTW(port, v) (*((volatile uint32 *)(port)) = ((uint32)(v)))


#ifndef readl
static inline uint32
readl(volatile void *addr)
{
   uint32 val;
   asm volatile("lw %0, 0(%1)" : "=r"(val) : "r"(addr));
   return val;
}
#endif 

#ifndef writel
static inline void
writel(uint32 val, volatile void *addr)
{
  asm volatile("sw %0, 0(%1)" : : "r"(val), "r" (addr));
}
#endif
