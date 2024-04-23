#ifdef CONFIG_64BIT
#define BITS_PER_LONG 64
#else
#define BITS_PER_LONG 32
#endif /* CONFIG_64BIT */

#define BIT(nr) (1UL <<(nr))

/*
 *  Create a contiguous bitmask starting at bit position @l and ending at
 *  position @h. For example
 *  GENMASK_ULL(39, 21) gives us the 64bit vector 0x000000ffffe00000.
 *     
 */
#define GENMASK(h, l) \
	        (((~0UL) - (1UL << (l)) + 1) & (~0UL >> (BITS_PER_LONG - 1 - (h))))
