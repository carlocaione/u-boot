/*
 * (C) Copyright 2016 Beniamino Galvani <b.galvani@gmail.com>
 *
 * SPDX-License-Identifier:	GPL-2.0+
 */

#include <common.h>
#include <asm/arch/s905.h>

DECLARE_GLOBAL_DATA_PTR;

int board_init(void)
{
	return 0;
}

int board_early_init_r(void)
{
	return 0;
}

int dram_init(void)
{
	gd->ram_size = 1024 * 1024 * 1024;

	return 0;
}

void reset_cpu(ulong addr)
{
}

int board_late_init(void)
{
	return 0;
}

int checkboard(void)
{
	puts("Board: Hardkernel ODROID-C2\n");
	return 0;
}
