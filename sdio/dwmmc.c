/*
 * (C) Copyright 2012 SAMSUNG Electronics
 * Jaehoon Chung <jh80.chung@samsung.com>
 * Rajeshawari Shinde <rajeshwari.s@samsung.com>
 *
 * SPDX-License-Identifier:	GPL-2.0+
 */

//#include <comdef.h>
//#include <mmc.h>
//#include <timer.h>
//#include "errno.h"

#include "bouncebuf.h"
#include "comdef.h"
#include "mmc.h"
#include "dwmmc.h"
#include "timer.h"
#include "errno.h"

extern void printf(char *fmt, ...);
extern void proc_print(void);

#define PAGE_SIZE 4096

static int dwmci_wait_reset(struct dwmci_host *host, u32 value)
{
	unsigned long timeout = 1000;
	u32 ctrl;

	dwmci_writel(host, DWMCI_CTRL, value);

	while (timeout--) {
		ctrl = dwmci_readl(host, DWMCI_CTRL);
		if (!(ctrl & DWMCI_RESET_ALL))
			return 1;
	}
	return 0;
}

static void dwmci_set_idma_desc(struct dwmci_idmac *idmac,
		u32 desc0, u32 desc1, u32 desc2)
{
	struct dwmci_idmac *desc = idmac;

	desc->flags = desc0;
	desc->cnt = desc1;
	desc->addr = desc2;
	desc->next_addr =(uintptr_t)desc + sizeof(struct dwmci_idmac);
}

static void dwmci_prepare_data(struct dwmci_host *host,
			       struct mmc_data *data,
			       struct dwmci_idmac *cur_idmac,
			       void *bounce_buffer)
{
	unsigned long ctrl;
	unsigned int i = 0, flags, cnt, blk_cnt;
	//uintptr_t data_start, data_end;


	blk_cnt = data->blocks;

	dwmci_wait_reset(host, DWMCI_CTRL_FIFO_RESET);

	//data_start = (uintptr_t)(void *)cur_idmac;
	dwmci_writel(host, DWMCI_DBADDR, (uintptr_t)cur_idmac);

	do {
		flags = DWMCI_IDMAC_OWN | DWMCI_IDMAC_CH ;
		flags |= (i == 0) ? DWMCI_IDMAC_FS : 0;
		if (blk_cnt <= 8) {
			flags |= DWMCI_IDMAC_LD;
			cnt = data->blocksize * blk_cnt;
		} else
			cnt = data->blocksize * 8;

		dwmci_set_idma_desc(cur_idmac, flags, cnt,
				    (uintptr_t)bounce_buffer + (i * PAGE_SIZE));

		if (blk_cnt <= 8)
			break;
		blk_cnt -= 8;
		cur_idmac++;
		i++;
	} while(1);

	//data_end = (uintptr_t)cur_idmac;
	//flush_dcache_range(data_start, data_end + ARCH_DMA_MINALIGN);

	ctrl = dwmci_readl(host, DWMCI_CTRL);
	ctrl |= DWMCI_IDMAC_EN | DWMCI_DMA_EN;
	dwmci_writel(host, DWMCI_CTRL, ctrl);

	ctrl = dwmci_readl(host, DWMCI_BMOD);
	ctrl |= DWMCI_BMOD_IDMAC_FB | DWMCI_BMOD_IDMAC_EN;
	//ctrl |= DWMCI_BMOD_IDMAC_EN;
	dwmci_writel(host, DWMCI_BMOD, ctrl);

	dwmci_writel(host, DWMCI_BLKSIZ, data->blocksize);
	dwmci_writel(host, DWMCI_BYTCNT, data->blocksize * data->blocks);
}

static int dwmci_data_transfer(struct dwmci_host *host, struct mmc_data *data)
{
	int ret = 0;
	u32 timeout = 40000000;
	u32 mask=0, size, i, len = 0;
	u32 *buf = NULL;
	unsigned int start = get_timer(0);
	u32 fifo_depth = (((host->fifoth_val & RX_WMARK_MASK) >>
			    RX_WMARK_SHIFT) + 1) * 2;

	size = data->blocksize * data->blocks / 4;
	if (data->flags == MMC_DATA_READ)
		buf = (unsigned int *)data->dest;
	else
		buf = (unsigned int *)data->src;

	for (;;) {
		mask = dwmci_readl(host, DWMCI_RINTSTS);
		/* Error during data transfer. */
		if (mask & (DWMCI_DATA_ERR | DWMCI_DATA_TOUT)) {
			printf("%s DATA ERROR! mask 0x%x  0x%x\r\n", __func__,mask,(DWMCI_DATA_ERR | DWMCI_DATA_TOUT));
			ret = -EINVAL;
			break;
		}

		if (host->fifo_mode && size) {
			printf("%s host->fifo_mode\r\n", __func__);
			len = 0;
			if (data->flags == MMC_DATA_READ &&
			    (mask & DWMCI_INTMSK_RXDR)) {
				while (size) {
					len = dwmci_readl(host, DWMCI_STATUS);
					len = (len >> DWMCI_FIFO_SHIFT) &
						    DWMCI_FIFO_MASK;
					len = min(size, len);
					for (i = 0; i < len; i++)
						*buf++ =
						dwmci_readl(host, DWMCI_DATA);
					size = size > len ? (size - len) : 0;
				}
				dwmci_writel(host, DWMCI_RINTSTS,
					     DWMCI_INTMSK_RXDR);
			} else if (data->flags == MMC_DATA_WRITE &&
				   (mask & DWMCI_INTMSK_TXDR)) {
				while (size) {
					len = dwmci_readl(host, DWMCI_STATUS);
					len = fifo_depth - ((len >>
						   DWMCI_FIFO_SHIFT) &
						   DWMCI_FIFO_MASK);
					len = min(size, len);
					for (i = 0; i < len; i++)
						dwmci_writel(host, DWMCI_DATA,
							     *buf++);
					size = size > len ? (size - len) : 0;
				}
				dwmci_writel(host, DWMCI_RINTSTS,
					     DWMCI_INTMSK_TXDR);
			}
		}

		/* Data arrived correctly. */
		if (mask & DWMCI_INTMSK_DTO) {
			ret = 0;
			break;
		}
		/* Check for timeout. */
		if (get_timer(start) > timeout) {
			printf("%s: Timeout waiting for data!\n",__func__);
			ret = -ETIMEDOUT;
			break;


		}	
	}

	dwmci_writel(host, DWMCI_RINTSTS, mask);

	return ret;
}


static int dwmci_set_transfer_mode(struct dwmci_host *host,
		struct mmc_data *data)
{
	unsigned long mode;

	mode = DWMCI_CMD_DATA_EXP;
	if (data->flags & MMC_DATA_WRITE)
		mode |= DWMCI_CMD_RW;

	return mode;
}


static int dwmci_send_cmd(struct mmc *mmc, struct mmc_cmd *cmd,
		struct mmc_data *data)
{
	struct dwmci_host *host = mmc->priv;

	ALLOC_CACHE_ALIGN_BUFFER(struct dwmci_idmac, cur_idmac, 
				 data ? DIV_ROUND_UP(data->blocks, 8) : 0);

	int ret = 0, flags = 0, i;
	unsigned int timeout = 100000;
	u32 retry = 10000;
	u32 mask, ctrl;
	unsigned int start = get_timer(0);
	struct bounce_buffer bbstate;

	while ((dwmci_readl(host, DWMCI_STATUS) & DWMCI_BUSY)) {
		if (get_timer(start) > timeout) {
			printf("%s: Timeout on data busy.\r\n", __func__);
			return -ETIMEDOUT;
		}
	}

	dwmci_writel(host, DWMCI_RINTSTS, DWMCI_INTMSK_ALL);

	if (data) {
		if (host->fifo_mode) {
			dwmci_writel(host, DWMCI_BLKSIZ, data->blocksize);
			dwmci_writel(host, DWMCI_BYTCNT,
				     data->blocksize * data->blocks);
			dwmci_wait_reset(host, DWMCI_CTRL_FIFO_RESET);
		} else {
			if (data->flags == MMC_DATA_READ) {
				bounce_buffer_start(&bbstate, (void*)data->dest,
						data->blocksize *
						data->blocks, GEN_BB_WRITE);
			} else {
				bounce_buffer_start(&bbstate, (void*)data->src,
						data->blocksize *
						data->blocks, GEN_BB_READ);
			}
			dwmci_prepare_data(host, data, cur_idmac,
					   bbstate.bounce_buffer);
		}
	}

	dwmci_writel(host, DWMCI_CMDARG, cmd->cmdarg);

	if (data)
		flags = dwmci_set_transfer_mode(host, data);

	if ((cmd->resp_type & MMC_RSP_136) && (cmd->resp_type & MMC_RSP_BUSY))
		return -1;

	if (cmd->cmdidx == MMC_CMD_STOP_TRANSMISSION)
		flags |= DWMCI_CMD_ABORT_STOP;
	else
		flags |= DWMCI_CMD_PRV_DAT_WAIT;


	if (cmd->resp_type & MMC_RSP_PRESENT) {
		flags |= DWMCI_CMD_RESP_EXP;
		if (cmd->resp_type & MMC_RSP_136)
			flags |= DWMCI_CMD_RESP_LENGTH;
	}


	if (cmd->resp_type & MMC_RSP_CRC)
		flags |= DWMCI_CMD_CHECK_CRC;


	flags |= (cmd->cmdidx | DWMCI_CMD_START | DWMCI_CMD_USE_HOLD_REG);


	//printf("Sending CMD%p, flags :%p\r\n", cmd->cmdidx, flags);

	dwmci_writel(host, DWMCI_CMD, flags);


	for (i = 0; i < retry; i++) {
		mask = dwmci_readl(host, DWMCI_RINTSTS);
		if (mask & DWMCI_INTMSK_CDONE) {
			if (!data)
				dwmci_writel(host, DWMCI_RINTSTS, mask);
			break;
		}
	}

	if (i == retry) {
		printf("%s: Timeout.\r\n", __func__);
		return -ETIMEDOUT;
	}

	if (mask & DWMCI_INTMSK_RTO) {
		/*
		 * Timeout here is not necessarily fatal. (e)MMC cards
		 * will splat here when they receive CMD55 as they do
		 * not support this command and that is exactly the way
		 * to tell them apart from SD cards. Thus, this output
		 * below shall be printf(). eMMC cards also do not favor
		 * CMD8, please keep that in mind.
		 */
		printf("%s: Response Timeout.\r\n", __func__);
		return -ETIMEDOUT;
	} else if (mask & DWMCI_INTMSK_RE) {
		printf("%s: Response Error.\r\n", __func__);
		return -EIO;
	}


	if (cmd->resp_type & MMC_RSP_PRESENT) {
		if (cmd->resp_type & MMC_RSP_136) {
			cmd->response[0] = dwmci_readl(host, DWMCI_RESP3);
			cmd->response[1] = dwmci_readl(host, DWMCI_RESP2);
			cmd->response[2] = dwmci_readl(host, DWMCI_RESP1);
			cmd->response[3] = dwmci_readl(host, DWMCI_RESP0);
		} else {
			cmd->response[0] = dwmci_readl(host, DWMCI_RESP0);
		}
	}

	if (data) {
		ret = dwmci_data_transfer(host, data);

		/* only dma mode need it */
		if (!host->fifo_mode) {
			ctrl = dwmci_readl(host, DWMCI_CTRL);
			ctrl &= ~(DWMCI_DMA_EN);
			dwmci_writel(host, DWMCI_CTRL, ctrl);
		//	bounce_buffer_stop(&bbstate);
		}
	}

	return ret;
}


static int dwmci_setup_bus(struct dwmci_host *host, u32 freq)
{
	u32 div, status;
	int timeout = 100000;
	unsigned long sclk;

	if ((freq == host->clock) || (freq == 0)) {
		return 0;
	}
	/*
	 * If host->get_mmc_clk isn't defined,
	 * then assume that host->bus_hz is source clock value.
	 * host->bus_hz should be set by user.
	 */
	if (host->get_mmc_clk) {
		printf("%s: Timeout!\n", __func__);
		sclk = host->get_mmc_clk(host, freq);
	} else if (host->bus_hz)
		sclk = host->bus_hz;
	else {
		printf("%s: Didn't get source clock value.\r\n", __func__);
		return -EINVAL;
	}

	if (sclk == freq)
		div = 0;	/* bypass mode */
	else
		div = DIV_ROUND_UP(sclk, 2 * freq);

	dwmci_writel(host, DWMCI_CLKENA, 0);
	dwmci_writel(host, DWMCI_CLKSRC, 0);

	dwmci_writel(host, DWMCI_CLKDIV, div);
	dwmci_writel(host, DWMCI_CMD, DWMCI_CMD_PRV_DAT_WAIT |
			DWMCI_CMD_UPD_CLK | DWMCI_CMD_START);

	do {
		status = dwmci_readl(host, DWMCI_CMD);
		if (timeout-- < 0) {
			printf("%s: Timeout!\n", __func__);
			return -ETIMEDOUT;
		}
	} while (status & DWMCI_CMD_START);

	dwmci_writel(host, DWMCI_CLKENA, DWMCI_CLKEN_ENABLE |
			DWMCI_CLKEN_LOW_PWR);

	dwmci_writel(host, DWMCI_CMD, DWMCI_CMD_PRV_DAT_WAIT |
			DWMCI_CMD_UPD_CLK | DWMCI_CMD_START);

	timeout = 100000;
	do {
		status = dwmci_readl(host, DWMCI_CMD);
		if (timeout-- < 0) {
			printf("%s: Timeout!\r\n", __func__);
			return -ETIMEDOUT;
		}
	} while (status & DWMCI_CMD_START);

	host->clock = freq;

	return 0;
}


static void dwmci_set_ios(struct mmc *mmc)
{
	struct dwmci_host *host = (struct dwmci_host *)mmc->priv;
	u32 ctype, regs;

	dwmci_setup_bus(host, mmc->clock);
	switch (mmc->bus_width) {
	case 8:
		ctype = DWMCI_CTYPE_8BIT;
		break;
	case 4:
		ctype = DWMCI_CTYPE_4BIT;
		break;
	default:
		ctype = DWMCI_CTYPE_1BIT;
		break;
	}

	dwmci_writel(host, DWMCI_CTYPE, ctype);

	regs = dwmci_readl(host, DWMCI_UHS_REG);
	if (mmc->ddr_mode)
		regs |= DWMCI_DDR_MODE;
	else
		regs &= ~DWMCI_DDR_MODE;

	dwmci_writel(host, DWMCI_UHS_REG, regs);

	if (host->clksel)
		host->clksel(host);
}



static int dwmci_init(struct mmc *mmc)
{
	struct dwmci_host *host = mmc->priv;

	if (host->board_init) {
		printf("board_init\r\n");
		host->board_init(host);
	}

	dwmci_writel(host, DWMCI_PWREN, 1);
	udelay(100);

	if (!dwmci_wait_reset(host, DWMCI_RESET_ALL)) {
		printf("\r\n%s[%d] Fail-reset!!\r\n", __func__, __LINE__);
		return -EIO;
	}

	/* Enumerate at 400KHz */
	dwmci_setup_bus(host, mmc->cfg->f_min);

	dwmci_writel(host, DWMCI_RINTSTS, 0xFFFFFFFF);
	dwmci_writel(host, DWMCI_INTMASK, 0);

	dwmci_writel(host, DWMCI_TMOUT, 0xFFFFFFFF);

	dwmci_writel(host, DWMCI_IDINTEN, 0);
	dwmci_writel(host, DWMCI_BMOD, 1);

	if (!host->fifoth_val) {
		unsigned int fifo_size;

		fifo_size = dwmci_readl(host, DWMCI_FIFOTH);
		fifo_size = ((fifo_size & RX_WMARK_MASK) >> RX_WMARK_SHIFT) + 1;
		host->fifoth_val = MSIZE(0x2) | RX_WMARK(fifo_size / 2 - 1) |
				TX_WMARK(fifo_size / 2);
	}
	dwmci_writel(host, DWMCI_FIFOTH, host->fifoth_val);
        if (!host->fifo_mode) {
	  printf("debug %s line %d\r\n", __func__, __LINE__);
          dwmci_writel(host, DWMCI_IDINTEN, DWMCI_IDINTEN_MASK);
	}

	dwmci_writel(host, DWMCI_CLKENA, 0);
	dwmci_writel(host, DWMCI_CLKSRC, 0);

	return 0;
} 


static  struct mmc_ops dwmci_ops = {
	.send_cmd	= dwmci_send_cmd,
	.set_ios	= dwmci_set_ios,
	.init		= dwmci_init,
	.test		= proc_print,
};

int add_dwmci(struct dwmci_host *host, u32 max_clk, u32 min_clk,u32 dev_num)
{
	host->cfg.name = host->name;
	host->cfg.ops = &dwmci_ops;               // It's so strange, it dose'nt work.
						  // and needs the following three codes.
	host->cfg.ops->send_cmd = dwmci_send_cmd;
	host->cfg.ops->set_ios = dwmci_set_ios;
	host->cfg.ops->init = dwmci_init;

	host->cfg.f_min = min_clk;
	host->cfg.f_max = max_clk;

	host->cfg.voltages = MMC_VDD_32_33 | MMC_VDD_33_34;

	host->cfg.host_caps = host->caps;

	if (host->buswidth == 8) {
		host->cfg.host_caps |= MMC_MODE_8BIT;
		host->cfg.host_caps &= ~MMC_MODE_4BIT;
	} else {
		host->cfg.host_caps |= MMC_MODE_4BIT;
		host->cfg.host_caps &= ~MMC_MODE_8BIT;
	}
	host->cfg.host_caps |= MMC_MODE_HS | MMC_MODE_HS_52MHz;

	host->cfg.b_max = CONFIG_SYS_MMC_MAX_BLK_COUNT;

	host->mmc = mmc_create(&host->cfg, host, dev_num);
	if (host->mmc == NULL)
		return -1;

	return 0;
}
