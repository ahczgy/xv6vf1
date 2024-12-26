/* SPDX-License-Identifier: GPL-2.0-or-later */
/**
  ******************************************************************************
  * @file  gpio.c
  * @author  StarFive Technology
  * @version  V1.0
  * @date  08/13/2020
  * @brief
  ******************************************************************************
  * @copy
  *
  * THE PRESENT SOFTWARE WHICH IS FOR GUIDANCE ONLY AIMS AT PROVIDING CUSTOMERS
  * WITH CODING INFORMATION REGARDING THEIR PRODUCTS IN ORDER FOR THEM TO SAVE
  * TIME. AS A RESULT, STARFIVE SHALL NOT BE HELD LIABLE FOR ANY
  * DIRECT, INDIRECT OR CONSEQUENTIAL DAMAGES WITH RESPECT TO ANY CLAIMS ARISING
  * FROM THE CONTENT OF SUCH SOFTWARE AND/OR THE USE MADE BY CUSTOMERS OF THE
  * CODING INFORMATION CONTAINED HEREIN IN CONNECTION WITH THEIR PRODUCTS.
  *
  *  COPYRIGHT 2020 Shanghai StarFive Technology Co., Ltd.
  */
#include <types.h>

#include "gpio.h"

#define MODE_SELECT_REG	0x1702002c
#define SYS_SYSCON_BASE 0x13030000

extern void printf(char *fmt, ...);

static void jh7110_gpio_init(void)
{
	/* This is for fixing don't detect wm8960 occassionally.
	 * Set scl/sda gpio output enable
	 * Set drive strength to 12mA
	 * Set gpio pull up
	 */
	SYS_IOMUX_COMPLEX(57, 9, 0, 1);
	SYS_IOMUX_SET_DS(57, 3);
	SYS_IOMUX_SET_PULL(57, GPIO_PULL_UP);

	SYS_IOMUX_COMPLEX(58, 10, 0, 1);
	SYS_IOMUX_SET_DS(58, 3);
	SYS_IOMUX_SET_PULL(58, GPIO_PULL_UP);
}

void board_init_f(void)
{

	/* gpio initializing */
	jh7110_gpio_init();

	/* set GPIO to 3.3v */
	setbits_le32(SYS_SYSCON_BASE + 0xC, 0x0);

	/*uart0 tx*/
	SYS_IOMUX_DOEN(5, LOW);
        SYS_IOMUX_DOUT(5, 20);
        /*uart0 rx*/
        SYS_IOMUX_DOEN(6, HIGH);
        SYS_IOMUX_DIN(6, 14);

	 /*jtag*/
        SYS_IOMUX_DOEN(36, HIGH);
        SYS_IOMUX_DIN(36, 4);
        SYS_IOMUX_DOEN(61, HIGH);
        SYS_IOMUX_DIN(61, 19);
        SYS_IOMUX_DOEN(63, HIGH);
        SYS_IOMUX_DIN(63, 20);
        SYS_IOMUX_DOEN(60, HIGH);
        SYS_IOMUX_DIN(60, 29);
        SYS_IOMUX_DOEN(44, 8);
        SYS_IOMUX_DOUT(44, 22);
	
	/* reset emmc */
	SYS_IOMUX_DOEN(62, LOW);
        SYS_IOMUX_DOUT(62, 19);
        SYS_IOMUX_SET_DS(64, 2);
        SYS_IOMUX_SET_SLEW(64, 1);
        SYS_IOMUX_SET_DS(65, 1);
        SYS_IOMUX_SET_DS(66, 1);
        SYS_IOMUX_SET_DS(67, 1);
        SYS_IOMUX_SET_DS(68, 1);
        SYS_IOMUX_SET_DS(69, 1);
        SYS_IOMUX_SET_DS(70, 1);
	SYS_IOMUX_SET_DS(71, 1);
        SYS_IOMUX_SET_DS(72, 1);
        SYS_IOMUX_SET_DS(73, 1);

	/* reset sdio */
	SYS_IOMUX_DOEN(10, LOW);
	SYS_IOMUX_DOUT(10, 55);
	SYS_IOMUX_SET_DS(10, 3);
	SYS_IOMUX_COMPLEX(9, 44, 57, 19);
	SYS_IOMUX_SET_DS(9, 3);
	SYS_IOMUX_COMPLEX(11, 45, 58, 20);
	SYS_IOMUX_SET_DS(11, 3);
	SYS_IOMUX_COMPLEX(12, 46, 59, 21);
	SYS_IOMUX_SET_DS(12, 3);
	SYS_IOMUX_COMPLEX(7, 47, 60, 22);
	SYS_IOMUX_SET_DS(7, 3);
	SYS_IOMUX_COMPLEX(8, 48, 61, 23);
	SYS_IOMUX_SET_DS(8, 3);
}


void boot_device(void)
{
	int boot_mode = 0;
	boot_mode = readl((volatile void *)MODE_SELECT_REG) & 0x3;
	switch (boot_mode) {
		case 0:
			printf("boot from spi.\r\n");
			break;
		case 1:
			printf("boot from mmc2.\r\n");
			break;
		case 2:
			printf("boot from mmc1.\r\n");
			break;
		case 3:
			printf("boot from spi.\r\n");
			break;
		default:
			printf("Unsupported boot device 0x%x. \r\n", boot_mode);
			break;
	}
}
