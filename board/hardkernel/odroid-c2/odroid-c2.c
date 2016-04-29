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
#include <dm/platdata.h>

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
		setbits_le32(P_PERIPHS_PIN_MUX_2, 0x3f << 10);
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

static const struct meson_sd_platdata gxbb_sd_platdata = {
	.sd_emmc_reg = (struct sd_emmc_global_regs *) SD_EMMC_BASE_B,
};

U_BOOT_DEVICE(meson_mmc) = {
	.name = "meson_mmc",
	.platdata = &gxbb_sd_platdata,
};

int board_mmc_init(bd_t *bis)
{
	aml_sd_emmc_init(SDIO_PORT_B);

	return 0;
}

#endif /* CONFIG_GENERIC_MMC */
