/*
 * (C) Copyright 2016 Beniamino Galvani <b.galvani@gmail.com>
 *
 * SPDX-License-Identifier:	GPL-2.0+
 */

#include <common.h>
#include <mmc.h>
#include <malloc.h>
#include <asm/io.h>
#include <asm/arch/cpu_sdio.h>
#include <asm/arch/sd_emmc.h>
#include <asm/arch/secure_apb.h>

#ifdef CONFIG_GENERIC_MMC

unsigned aml_sd_debug_board_1bit_flag = 0;

static int aml_sd_emmc_init(unsigned port)
{
	switch (port) {
	case SDIO_PORT_A:
		setbits_le32(P_PERIPHS_PIN_MUX_8, 0x3f);
		printf("PORT A\n");
		break;
	case SDIO_PORT_B:
		if (aml_sd_debug_board_1bit_flag)
			setbits_le32(P_PERIPHS_PIN_MUX_2, (0x3 << 10) | (0x1 << 15));
		else
			setbits_le32(P_PERIPHS_PIN_MUX_2, 0x3f << 10);
		printf("PORT B\n");
		break;
	case SDIO_PORT_C:
		clrbits_le32(P_PERIPHS_PIN_MUX_2, (0x1f << 22));
		setbits_le32(P_PERIPHS_PIN_MUX_4, (0x3 << 18) | (3 << 30));
		printf("PORT C\n");
		break;
	default:
		printf("PORT default\n");
		return -1;
	}

	printf("PORT stocazzo\n");
	return 0;
}

static int aml_sd_emmc_detect(unsigned port)
{
	if (port == SDIO_PORT_B) {
		setbits_le32(P_PREG_PAD_GPIO5_EN_N, 1 << 29);
		printf("ok, is port B\n");
		if ((readl(P_PERIPHS_PIN_MUX_8) & (3 << 9))) {
			if (!(readl(P_PREG_PAD_GPIO2_I) & (1 << 24))) {
				printf("aml_sd_debug_board_1bit_flag = 1\n");
				aml_sd_debug_board_1bit_flag = 1;
			} else {
				printf("aml_sd_debug_board_1bit_flag = 0\n");
				aml_sd_debug_board_1bit_flag = 0;
			}
			printf("===> aml_sd_debug_board_1bit_flag = %d\n", aml_sd_debug_board_1bit_flag);

			if (!aml_sd_debug_board_1bit_flag) {
				printf("aml_sd_debug_board_1bit_flag = 0, returning 1\n");
				return 1;
			}
		}
	}

	printf("returning 0\n");
	return 0;
}

static void aml_sd_emmc_pwr_nop(unsigned port)
{
	return;
}

static int board_mmc_register(unsigned port)
{
	struct aml_card_sd_info *aml_priv = aml_cpu_sd_emmc_get(port);
	struct mmc *mmc0;

	if (aml_priv == NULL)
		return -1;

	aml_priv->sd_emmc_init		= aml_sd_emmc_init;
	aml_priv->sd_emmc_detect	= aml_sd_emmc_detect;
	aml_priv->sd_emmc_pwr_off	= aml_sd_emmc_pwr_nop;
	aml_priv->sd_emmc_pwr_on	= aml_sd_emmc_pwr_nop;
	aml_priv->sd_emmc_pwr_prepare	= aml_sd_emmc_pwr_nop;

	aml_priv->desc_buf = (char *) malloc(NEWSD_MAX_DESC_MUN * sizeof(struct sd_emmc_desc_info));
	if (!aml_priv->desc_buf)
		return -1;

	mmc0 = aml_sd_emmc_register(aml_priv);
	if (!mmc0)
		return -1;

	return 0;
}

int board_mmc_init(bd_t *bis)
{
	return board_mmc_register(CONFIG_MMC_MESON_SD_PORT);
}

#endif /* CONFIG_GENERIC_MMC */
