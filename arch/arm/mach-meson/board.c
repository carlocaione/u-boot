/*
 * (C) Copyright 2016 Beniamino Galvani <b.galvani@gmail.com>
 *
 * SPDX-License-Identifier:	GPL-2.0+
 */

#include <common.h>
#include <libfdt.h>
#include <linux/err.h>
#include <asm/arch/s905.h>

DECLARE_GLOBAL_DATA_PTR;

int dram_init(void)
{
	const fdt32_t *val;
	int offset;
	int len;

	offset = fdt_path_offset(gd->fdt_blob, "/memory");
	if (offset < 0)
		return -EINVAL;

	val = fdt_getprop(gd->fdt_blob, offset, "reg", &len);
	if (len < sizeof(*val) * 4)
		return -EINVAL;

	/* Don't use fdt64_t to avoid unaligned access */
	gd->ram_size = (uint64_t)fdt32_to_cpu(val[2]) << 32;
	gd->ram_size |= fdt32_to_cpu(val[3]);

	debug("DRAM size = %lu MiB\n", (unsigned long)gd->ram_size >> 10);

	return 0;
}

void dram_init_banksize(void)
{
	gd->bd->bi_dram[0].start = CONFIG_SYS_SDRAM_BASE;
	gd->bd->bi_dram[0].size = gd->ram_size;
}

int board_init(void)
{
	return 0;
}

int board_early_init_r(void)
{
	return 0;
}

void reset_cpu(ulong addr)
{
}

int checkboard(void)
{
	puts("Board: Hardkernel ODROID-C2\n");
	return 0;
}
