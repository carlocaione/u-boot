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

extern unsigned aml_sd_debug_board_1bit_flag;

static unsigned long aml_sd_emmc_base_addr[3] = {
	SD_EMMC_BASE_A,
	SD_EMMC_BASE_B,
	SD_EMMC_BASE_C
};

static struct aml_card_sd_info aml_sd_emmc_ports[] = {
	{ .sd_emmc_port = SDIO_PORT_A, .name = "SDIO Port A" },
	{ .sd_emmc_port = SDIO_PORT_B, .name = "SDIO Port B" },
	{ .sd_emmc_port = SDIO_PORT_C, .name = "SDIO Port C" },
};

struct aml_card_sd_info *aml_cpu_sd_emmc_get(unsigned port)
{
	if (port < SDIO_PORT_C + 1)
		return &aml_sd_emmc_ports[port];

	return NULL;
}

static void aml_sd_cfg_swth(struct mmc *mmc)
{
	unsigned sd_emmc_clkc =	0, clk, clk_src, clk_div;
	unsigned vconf;
	unsigned bus_width;
	struct aml_card_sd_info *aml_priv;
	struct sd_emmc_global_regs *sd_emmc_reg;
	struct sd_emmc_config *sd_emmc_cfg;

	aml_priv = mmc->priv;
	sd_emmc_reg = aml_priv->sd_emmc_reg;
	sd_emmc_cfg = (struct sd_emmc_config *) &vconf;
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

	sd_emmc_clkc = ((0 << Cfg_irq_sdio_sleep_ds) |
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

	sd_emmc_reg->gclock = sd_emmc_clkc;
	vconf = sd_emmc_reg->gcfg;

	sd_emmc_cfg->bus_width = bus_width;
	sd_emmc_cfg->bl_len = 9;
	sd_emmc_cfg->resp_timeout = 7;
	sd_emmc_cfg->rc_cc = 4;
	sd_emmc_reg->gcfg = vconf;

	return;
}

static int aml_sd_inand_check_insert(struct mmc *mmc)
{
	int level;
	struct aml_card_sd_info *sd_inand_info;
	unsigned sd_emmc_port;

	sd_inand_info = mmc->priv;
	sd_emmc_port = sd_inand_info->sd_emmc_port;
	level = sd_inand_info->sd_emmc_detect(sd_emmc_port);

	if (level) {
		if (sd_inand_info->init_retry) {
			sd_inand_info->sd_emmc_pwr_off(sd_emmc_port);
			sd_inand_info->init_retry = 0;
		}
		if (sd_inand_info->inited_flag) {
			sd_inand_info->sd_emmc_pwr_off(sd_emmc_port);
			sd_inand_info->removed_flag = 1;
			sd_inand_info->inited_flag = 0;
		}
		return 0;
	}

	return 1;
}

static void aml_sd_inand_clear_response(unsigned *res_buf)
{
	int i;

	if (res_buf == NULL)
		return;

	for (i = 0; i < MAX_RESPONSE_BYTES; i++)
		res_buf[i]=0;
}

static int aml_sd_inand_staff_init(struct mmc *mmc)
{
	struct aml_card_sd_info *sdio = mmc->priv;
	struct mmc_config *cfg;
	unsigned base;

	sdio->sd_emmc_pwr_prepare(sdio->sd_emmc_port);
	sdio->sd_emmc_pwr_off(sdio->sd_emmc_port);

	mmc->clock = 400000;
	aml_sd_cfg_swth(mmc);

	if (sdio->sd_emmc_port == SDIO_PORT_B) {
		base = get_timer(0);

		while (get_timer(base) < 200)
			;
	}

	sdio->sd_emmc_pwr_on(sdio->sd_emmc_port);
	sdio->sd_emmc_init(sdio->sd_emmc_port);

	if (aml_sd_debug_board_1bit_flag == 1) {
		cfg = &((struct aml_card_sd_info *) mmc->priv)->cfg;
		cfg->host_caps = MMC_MODE_HS;
		mmc->cfg = cfg;
	}

	if (sdio->sd_emmc_port == SDIO_PORT_B) {
		base = get_timer(0);
		while (get_timer(base) < 200)
			;
	}

	if (!sdio->inited_flag)
		sdio->inited_flag = 1;

	return SD_NO_ERROR;
}

static int aml_sd_send_cmd(struct mmc *mmc, struct mmc_cmd *cmd,
			   struct mmc_data *data)
{
        struct sd_emmc_status *status_irq_reg;
        struct sd_emmc_start *desc_start;
        struct aml_card_sd_info *aml_priv;
        struct sd_emmc_global_regs *sd_emmc_reg;
        struct cmd_cfg *des_cmd_cur = NULL;
        struct sd_emmc_desc_info *desc_cur;
        int ret = SD_NO_ERROR;
        u32 buffer = 0;
        u32 resp_buffer;
        u32 vstart = 0;
        u32 status_irq = 0;
        u32 *write_buffer = NULL;

        status_irq_reg = (void *) &status_irq;
        desc_start = (struct sd_emmc_start *) &vstart;
        aml_priv = mmc->priv;
        sd_emmc_reg = aml_priv->sd_emmc_reg;
        desc_cur = (struct sd_emmc_desc_info *) aml_priv->desc_buf;

        memset(desc_cur, 0,
	       (NEWSD_MAX_DESC_MUN >> 2) * sizeof(struct sd_emmc_desc_info));

        des_cmd_cur = (struct cmd_cfg *) &(desc_cur->cmd_info);
        des_cmd_cur->cmd_index = 0x80 | cmd->cmdidx;
        desc_cur->cmd_arg = cmd->cmdarg;

        aml_sd_inand_clear_response(cmd->response);

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
                        desc_cur->data_addr = (unsigned long) sd_emmc_reg->gping;
                        desc_cur->data_addr |= 1<<0;
                }
        }

        des_cmd_cur->owner = 1;
        des_cmd_cur->end_of_chain = 0;

        des_cmd_cur->end_of_chain = 1;

        sd_emmc_reg->gstatus = NEWSD_IRQ_ALL;

        invalidate_dcache_range((unsigned long) aml_priv->desc_buf,
				(unsigned long) (aml_priv->desc_buf + NEWSD_MAX_DESC_MUN * (sizeof(struct sd_emmc_desc_info))));
        desc_start->init = 0;
        desc_start->busy = 1;
        desc_start->addr = (unsigned long) aml_priv->desc_buf >> 2;

        sd_emmc_reg->gcmd_cfg = desc_cur->cmd_info;
        sd_emmc_reg->gcmd_dat = desc_cur->data_addr;
        sd_emmc_reg->gcmd_arg = desc_cur->cmd_arg;

        while (1) {
                status_irq = sd_emmc_reg->gstatus;
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
			       (const void *) sd_emmc_reg->gping,
			       data->blocks * data->blocksize);
                }
        }

        if (cmd->resp_type & MMC_RSP_136) {
                cmd->response[0] = sd_emmc_reg->gcmd_rsp3;
                cmd->response[1] = sd_emmc_reg->gcmd_rsp2;
                cmd->response[2] = sd_emmc_reg->gcmd_rsp1;
                cmd->response[3] = sd_emmc_reg->gcmd_rsp0;
        } else {
                cmd->response[0] = sd_emmc_reg->gcmd_rsp0;
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

static int aml_sd_init(struct mmc *mmc)
{
	struct aml_card_sd_info *sdio = mmc->priv;

	if (sdio->inited_flag) {
		sdio->sd_emmc_init(sdio->sd_emmc_port);
		mmc->cfg->ops->set_ios(mmc);
		return 0;
	}

	if (aml_sd_inand_check_insert(mmc)) {
		aml_sd_inand_staff_init(mmc);
		return 0;
	}

	return 1;
}

static const struct mmc_ops aml_sd_emmc_ops = {
	.send_cmd	= aml_sd_send_cmd,
	.set_ios	= aml_sd_cfg_swth,
	.init		= aml_sd_init,
};

struct mmc *aml_sd_emmc_register(struct aml_card_sd_info *aml_priv)
{
	struct mmc_config *cfg;

	aml_priv->removed_flag = 1;
	aml_priv->inited_flag = 0;
	aml_priv->sd_emmc_reg = (struct sd_emmc_global_regs *) aml_sd_emmc_base_addr[aml_priv->sd_emmc_port];

	cfg = &aml_priv->cfg;
	cfg->name = aml_priv->name;
	cfg->ops = &aml_sd_emmc_ops;

	cfg->voltages = MMC_VDD_33_34 | MMC_VDD_32_33 | MMC_VDD_31_32 | MMC_VDD_165_195;
	cfg->host_caps = MMC_MODE_8BIT | MMC_MODE_4BIT | MMC_MODE_HS_52MHz | MMC_MODE_HS;
	cfg->f_min = 400000;
	cfg->f_max = 50000000;
	cfg->b_max = 256;

	return mmc_create(cfg, aml_priv);
}
