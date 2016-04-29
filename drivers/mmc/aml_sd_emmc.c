/*
 * drivers/mmc/aml_sd_emmc.c
 *
 * Copyright (C) 2015 Amlogic, Inc. All rights reserved.
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful, but WITHOUT
 * ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or
 * FITNESS FOR A PARTICULAR PURPOSE.  See the GNU General Public License for
 * more details.
 *
 * You should have received a copy of the GNU General Public License along
 * with this program; if not, write to the Free Software Foundation, Inc.,
 * 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA.
*/

#include <common.h>
#include <malloc.h>
#include <asm/io.h>
#include <mmc.h>
#include <asm/arch/sd_emmc.h>
#include <asm/arch/cpu_sdio.h>
#include <dm/device.h>
#include <fdtdec.h>

DECLARE_GLOBAL_DATA_PTR;

static void meson_mmc_set_ios(struct mmc *mmc)
{
	unsigned meson_mmc_clkc = 0;
	unsigned clk, clk_src, clk_div;
	unsigned vconf;
	unsigned bus_width;
	struct meson_mmc_platdata *pdata;
	struct meson_mmc_global_regs *meson_mmc_reg;
	struct meson_mmc_config *meson_mmc_cfg;

	pdata = mmc->priv;
	meson_mmc_reg = pdata->meson_mmc_reg;
	meson_mmc_cfg = (struct meson_mmc_config *) &vconf;
	bus_width = (mmc->bus_width == 1) ? 0 : (mmc->bus_width / 4);

	if (mmc->clock > 12000000) {
		clk = SD_EMMC_CLKSRC_DIV2;
		clk_src = 1;
	} else {
		clk = SD_EMMC_CLKSRC_24M;
		clk_src = 0;
	}
	clk_div= clk / mmc->clock;

	if (mmc->clock < mmc->cfg->f_min)
		mmc->clock=mmc->cfg->f_min;
	if (mmc->clock > mmc->cfg->f_max)
		mmc->clock=mmc->cfg->f_max;

	meson_mmc_clkc = ((0 << Cfg_irq_sdio_sleep_ds) |
			(0 << Cfg_irq_sdio_sleep) |
			(1 << Cfg_always_on) |
			(0 << Cfg_rx_delay) |
			(0 << Cfg_tx_delay) |
			(0 << Cfg_sram_pd) |
			(0 << Cfg_rx_phase) |
			(0 << Cfg_tx_phase) |
			(2 << Cfg_co_phase) |
			(clk_src << Cfg_src) |
			(clk_div << Cfg_div));

	meson_mmc_reg->gclock = meson_mmc_clkc;
	vconf = meson_mmc_reg->gcfg;

	meson_mmc_cfg->bus_width = bus_width;
	meson_mmc_cfg->bl_len = 9;
	meson_mmc_cfg->resp_timeout = 7;
	meson_mmc_cfg->rc_cc = 4;
	meson_mmc_reg->gcfg = vconf;

	return;
}


static void meson_mmc_clear_response(unsigned *res_buf)
{
	int i;

	if (res_buf == NULL)
		return;

	for (i = 0; i < MAX_RESPONSE_BYTES; i++)
		res_buf[i]=0;
}

static int meson_mmc_init(struct mmc *mmc)
{
	mmc->clock = 400000;
	meson_mmc_set_ios(mmc);

	return SD_NO_ERROR;
}

static int meson_mmc_send_cmd(struct mmc *mmc, struct mmc_cmd *cmd,
			   struct mmc_data *data)
{
        struct meson_mmc_status *status_irq_reg;
        struct meson_mmc_start *desc_start;
        struct meson_mmc_platdata *pdata;
        struct meson_mmc_global_regs *meson_mmc_reg;
        struct cmd_cfg *des_cmd_cur = NULL;
        struct meson_mmc_desc_info *desc_cur;
        int ret = SD_NO_ERROR;
        u32 buffer = 0;
        u32 resp_buffer;
        u32 vstart = 0;
        u32 status_irq = 0;
        u32 *write_buffer = NULL;

        status_irq_reg = (void *) &status_irq;
        desc_start = (struct meson_mmc_start *) &vstart;
        pdata = mmc->priv;
        meson_mmc_reg = pdata->meson_mmc_reg;
        desc_cur = (struct meson_mmc_desc_info *) pdata->desc_buf;

        memset(desc_cur, 0,
	       (SD_MAX_DESC_MUN >> 2) * sizeof(struct meson_mmc_desc_info));

        des_cmd_cur = (struct cmd_cfg *) &(desc_cur->cmd_info);
        des_cmd_cur->cmd_index = 0x80 | cmd->cmdidx;
        desc_cur->cmd_arg = cmd->cmdarg;

        meson_mmc_clear_response(cmd->response);

        if (cmd->resp_type & MMC_RSP_PRESENT) {
                resp_buffer = (unsigned long) cmd->response;
                des_cmd_cur->no_resp = 0;

                if (cmd->resp_type & MMC_RSP_136)
                        des_cmd_cur->resp_128 = 1;

                if (cmd->resp_type & MMC_RSP_BUSY)
                        des_cmd_cur->r1b = 1;

                if (!(cmd->resp_type & MMC_RSP_CRC))
                        des_cmd_cur->resp_nocrc = 1;

                des_cmd_cur->resp_num = 0;
                desc_cur->resp_addr = resp_buffer;
        } else {
                des_cmd_cur->no_resp = 1;
	}

        if (data) {
                des_cmd_cur->data_io = 1;
                if (data->flags == MMC_DATA_READ) {
                        des_cmd_cur->data_wr = 0;
                        buffer = (unsigned long) data->dest;
                        invalidate_dcache_range((unsigned long) data->dest,
						(unsigned long) (data->dest +
							data->blocks * data->blocksize));
                } else {
                        des_cmd_cur->data_wr = 1;
                        write_buffer = (u32 *) malloc(128 * 1024);
                        memset(write_buffer, 0, 128 * 1024);
                        memcpy(write_buffer, (u32 *) data->src, data->blocks * data->blocksize);
                        flush_dcache_range((unsigned long) write_buffer,
					   (unsigned long) (write_buffer +
						   data->blocks * data->blocksize));
                }

                if (data->blocks > 1) {
                        des_cmd_cur->block_mode = 1;
                        des_cmd_cur->length = data->blocks;
                } else {
                        des_cmd_cur->block_mode = 0;
                        des_cmd_cur->length = data->blocksize;
                }

                des_cmd_cur->data_num = 0;
                if (des_cmd_cur->data_wr == 1)
                        desc_cur->data_addr = (unsigned long) write_buffer;
                else
                        desc_cur->data_addr = buffer;
                desc_cur->data_addr &= ~(1<<0);

        }
        if (data) {
                if ((data->blocks * data->blocksize < 0x200) &&
				(data->flags == MMC_DATA_READ)) {
                        desc_cur->data_addr = (unsigned long) meson_mmc_reg->gping;
                        desc_cur->data_addr |= 1<<0;
                }
        }

        des_cmd_cur->owner = 1;
        des_cmd_cur->end_of_chain = 0;

        des_cmd_cur->end_of_chain = 1;

        meson_mmc_reg->gstatus = SD_IRQ_ALL;

        invalidate_dcache_range((unsigned long) pdata->desc_buf,
				(unsigned long) (pdata->desc_buf + SD_MAX_DESC_MUN * (sizeof(struct meson_mmc_desc_info))));
        desc_start->init = 0;
        desc_start->busy = 1;
        desc_start->addr = (unsigned long) pdata->desc_buf >> 2;

        meson_mmc_reg->gcmd_cfg = desc_cur->cmd_info;
        meson_mmc_reg->gcmd_dat = desc_cur->data_addr;
        meson_mmc_reg->gcmd_arg = desc_cur->cmd_arg;

        while (1) {
                status_irq = meson_mmc_reg->gstatus;
                if (status_irq_reg->end_of_chain)
                        break;
        }

        if (status_irq_reg->rxd_err)
                ret |= SD_EMMC_RXD_ERROR;
        if (status_irq_reg->txd_err)
                ret |= SD_EMMC_TXD_ERROR;
        if (status_irq_reg->desc_err)
                ret |= SD_EMMC_DESC_ERROR;
        if (status_irq_reg->resp_err)
                ret |= SD_EMMC_RESP_CRC_ERROR;
        if (status_irq_reg->resp_timeout)
                ret |= SD_EMMC_RESP_TIMEOUT_ERROR;
        if (status_irq_reg->desc_timeout)
                ret |= SD_EMMC_DESC_TIMEOUT_ERROR;
        if (data) {
                if ((data->blocks * data->blocksize < 0x200) &&
		    (data->flags == MMC_DATA_READ)) {
                        memcpy(data->dest,
			       (const void *) meson_mmc_reg->gping,
			       data->blocks * data->blocksize);
                }
        }

        if (cmd->resp_type & MMC_RSP_136) {
                cmd->response[0] = meson_mmc_reg->gcmd_rsp3;
                cmd->response[1] = meson_mmc_reg->gcmd_rsp2;
                cmd->response[2] = meson_mmc_reg->gcmd_rsp1;
                cmd->response[3] = meson_mmc_reg->gcmd_rsp0;
        } else {
                cmd->response[0] = meson_mmc_reg->gcmd_rsp0;
        }

        if (des_cmd_cur->data_wr == 1) {
                free(write_buffer);
                write_buffer = NULL;
        }

        if (ret) {
                if (status_irq_reg->resp_timeout)
                        return TIMEOUT;
                else
                        return ret;
        }

        return SD_NO_ERROR;
}

static const struct mmc_ops meson_mmc_ops = {
	.send_cmd	= meson_mmc_send_cmd,
	.set_ios	= meson_mmc_set_ios,
	.init		= meson_mmc_init,
};

static int meson_mmc_ofdata_to_platdata(struct udevice *dev)
{
	struct meson_mmc_platdata *pdata = dev->platdata;
	fdt_addr_t addr;

	addr = dev_get_addr(dev);
	if (addr == FDT_ADDR_T_NONE)
		return -EINVAL;

	pdata->meson_mmc_reg = (struct meson_mmc_global_regs *) addr;

	return 0;
}

static int meson_mmc_probe(struct udevice *dev)
{
	struct meson_mmc_platdata *pdata = dev->platdata;
	struct mmc_uclass_priv *upriv = dev_get_uclass_priv(dev);
	struct mmc *mmc;
	struct mmc_config *cfg;

	cfg = &pdata->cfg;
	cfg->ops = &meson_mmc_ops;

	cfg->voltages = MMC_VDD_33_34 | MMC_VDD_32_33 | MMC_VDD_31_32 | MMC_VDD_165_195;
	cfg->host_caps = MMC_MODE_8BIT | MMC_MODE_4BIT | MMC_MODE_HS_52MHz | MMC_MODE_HS;
	cfg->f_min = 400000;
	cfg->f_max = 50000000;
	cfg->b_max = 256;

	mmc = mmc_create(cfg, pdata);
	if (!mmc)
		return -ENOMEM;

	upriv->mmc = mmc;

	pdata->desc_buf = malloc(SD_MAX_DESC_MUN * sizeof(struct meson_mmc_desc_info));
	if (!pdata->desc_buf)
		return -EINVAL;

	return 0;

}

static int meson_mmc_remove(struct udevice *dev)
{
	struct meson_mmc_platdata *pdata = dev->platdata;

	free(pdata->desc_buf);
	return 0;
}

static const struct udevice_id meson_mmc_match[] = {
	{ .compatible = "amlogic,meson-mmc" },
	{ /* sentinel */ }
};

U_BOOT_DRIVER(meson_mmc) = {
	.name = "meson_mmc",
	.id = UCLASS_MMC,
	.of_match = meson_mmc_match,
	.probe = meson_mmc_probe,
	.remove = meson_mmc_remove,
	.ofdata_to_platdata = meson_mmc_ofdata_to_platdata,
	.platdata_auto_alloc_size = sizeof(struct meson_mmc_platdata),
};
