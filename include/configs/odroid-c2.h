/*
 * Configuration for ODROID-C2
 * (C) Copyright 2016 Beniamino Galvani <b.galvani@gmail.com>
 *
 * SPDX-License-Identifier:	GPL-2.0+
 */

#ifndef __CONFIG_H
#define __CONFIG_H

#define CONFIG_REMAKE_ELF
#define CONFIG_SYS_CACHELINE_SIZE	64
#define CONFIG_SYS_NO_FLASH
#define CONFIG_NR_DRAM_BANKS		1
#define CONFIG_ENV_IS_NOWHERE		1
#define CONFIG_ENV_SIZE		0x2000
#define CONFIG_SYS_MAXARGS		32
#define CONFIG_BAUDRATE		115200
#define CONFIG_SYS_MALLOC_LEN		(32 << 20)
#define CONFIG_SYS_CBSIZE		1024
#define CONFIG_OF_LIBFDT
#define CONFIG_DISPLAY_BOARDINFO

#define CONFIG_SYS_TEXT_BASE		0x01000000
#define CONFIG_SYS_INIT_SP_ADDR	0x20000000
#define CONFIG_SYS_LOAD_ADDR		CONFIG_SYS_TEXT_BASE

/* Generic Interrupt Controller Definitions */
#define GICD_BASE			0xf6801000
#define GICC_BASE			0xf6802000

#if !defined(CONFIG_IDENT_STRING)
# define CONFIG_IDENT_STRING		" odroid-c2"
#endif

/* Serial setup */
#define CONFIG_CPU_ARMV8

#define CONFIG_CONS_INDEX		0
#define CONFIG_BAUDRATE			115200
#define CONFIG_SYS_BAUDRATE_TABLE \
	{ 4800, 9600, 19200, 38400, 57600, 115200 }

/* Command line configuration */
#define CONFIG_CMD_ENV
/* #define CONFIG_MP */

#define CONFIG_PREBOOT		"run bootargs"
#define CONFIG_BOOTCOMMAND	"run $modeboot"
#define CONFIG_BOOTDELAY	5

#define CONFIG_BOARD_LATE_INIT

/* Monitor Command Prompt */
/* Console I/O Buffer Size */
#define CONFIG_SYS_PBSIZE		(CONFIG_SYS_CBSIZE + \
					sizeof(CONFIG_SYS_PROMPT) + 16)
#define CONFIG_SYS_HUSH_PARSER
#define CONFIG_SYS_BARGSIZE		CONFIG_SYS_CBSIZE
#define CONFIG_SYS_LONGHELP
#define CONFIG_CMDLINE_EDITING

#endif /* __CONFIG_H */
