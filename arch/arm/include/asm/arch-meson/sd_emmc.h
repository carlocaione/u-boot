/*
 * arch/arm/include/asm/arch-gxb/sd_emmc.h
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

#ifndef __SD_EMMC_H__
#define __SD_EMMC_H__

#include <mmc.h>
#include "cpu_sdio.h"

#define SD_EMMC_BASE_A			0xd0070000
#define SD_EMMC_BASE_B			0xd0072000
#define SD_EMMC_BASE_C			0xd0074000

#define NEWSD_IRQ_ALL			0x3fff

#define SD_EMMC_CLKSRC_24M		24000000
#define SD_EMMC_CLKSRC_DIV2		1000000000

#define NEWSD_IRQ_EN_ALL_INIT
#define NEWSD_MAX_DESC_MUN		512
#define NEWSD_BOUNCE_REQ_SIZE		(512 * 1024)
#define MAX_BLOCK_COUNTS		256
#define SD_EMMC_CLKSRC			24000000

#define MMC_RSP_136_NUM			4
#define MAX_RESPONSE_BYTES		4

#define RESPONSE_R1_R3_R6_R7_LENGTH	6
#define RESPONSE_R2_CID_CSD_LENGTH	17
#define RESPONSE_R4_R5_NONE_LENGTH	0

#define SDIO_PORT_A			0
#define SDIO_PORT_B			1
#define SDIO_PORT_C			2

#define CARD_SD_SDIO_INIT		BIT(0)
#define CARD_SD_SDIO_DETECT		BIT(1)
#define CARD_SD_SDIO_PWR_PREPARE	BIT(2)
#define CARD_SD_SDIO_PWR_ON		BIT(3)
#define CARD_SD_SDIO_PWR_OFF		BIT(4)

typedef enum _SD_Error_Status_t {
	SD_NO_ERROR			= 0,
	SD_ERROR_OUT_OF_RANGE,
	SD_ERROR_ADDRESS,
	SD_ERROR_BLOCK_LEN,
	SD_ERROR_ERASE_SEQ,
	SD_ERROR_ERASE_PARAM,
	SD_ERROR_WP_VIOLATION,
	SD_ERROR_CARD_IS_LOCKED,
	SD_ERROR_LOCK_UNLOCK_FAILED,
	SD_ERROR_COM_CRC,
	SD_ERROR_ILLEGAL_COMMAND,
	SD_ERROR_CARD_ECC_FAILED,
	SD_ERROR_CC,
	SD_ERROR_GENERAL,
	SD_ERROR_Reserved1,
	SD_ERROR_Reserved2,
	SD_ERROR_CID_CSD_OVERWRITE,
	SD_ERROR_AKE_SEQ,
	SD_ERROR_STATE_MISMATCH,
	SD_ERROR_HEADER_MISMATCH,
	SD_ERROR_DATA_CRC,
	SD_ERROR_TIMEOUT,
	SD_ERROR_DRIVER_FAILURE,
	SD_ERROR_WRITE_PROTECTED,
	SD_ERROR_NO_MEMORY,
	SD_ERROR_SWITCH_FUNCTION_COMUNICATION,
	SD_ERROR_NO_FUNCTION_SWITCH,
	SD_ERROR_NO_CARD_INS
} SD_Error_Status_t;


typedef enum _SD_Bus_Width {
	SD_BUS_SINGLE			= 1,
	SD_BUS_WIDE			= 4
} SD_Bus_Width_t;

typedef struct _SD_Card_Status {
	unsigned Reserved3:2;
	unsigned Reserved4:1;
	unsigned AKE_SEQ_ERROR:1;
	unsigned Reserved5:1;
	unsigned APP_CMD:1;
	unsigned NotUsed:2;

	unsigned READY_FOR_DATA:1;
	unsigned CURRENT_STATE:4;
	unsigned ERASE_RESET:1;
	unsigned CARD_ECC_DISABLED:1;
	unsigned WP_ERASE_SKIP:1;

	unsigned CID_CSD_OVERWRITE:1;
	unsigned Reserved1:1;
	unsigned Reserved2:1;
	unsigned ERROR:1;
	unsigned CC_ERROR:1;
	unsigned CARD_ECC_FAILED:1;
	unsigned ILLEGAL_COMMAND:1;
	unsigned COM_CRC_ERROR:1;

	unsigned LOCK_UNLOCK_FAILED:1;

	unsigned CARD_IS_LOCKED:1;
	unsigned WP_VIOLATION:1;
	unsigned ERASE_PARAM:1;
	unsigned ERASE_SEQ_ERROR:1;
	unsigned BLOCK_LEN_ERROR:1;
	unsigned ADDRESS_ERROR:1;
	unsigned OUT_OF_RANGE:1;
} SD_Card_Status_t;

typedef struct _SD_Response_R1 {
	SD_Card_Status_t card_status;
} SD_Response_R1_t;

struct aml_card_sd_info {
	unsigned sd_emmc_port;
	unsigned sd_emmc_reg_base;
	char *name;
	int inited_flag;
	int removed_flag;
	int init_retry;
	int single_blk_failed;
	char *desc_buf;
	struct mmc_config cfg;
	struct sd_emmc_global_regs *sd_emmc_reg;
	dma_addr_t desc_dma_addr;
	int  (* sd_emmc_init)(unsigned port);
	int  (* sd_emmc_detect)(unsigned port);
	void (* sd_emmc_pwr_prepare)(unsigned port);
	void (* sd_emmc_pwr_on)(unsigned port);
	void (* sd_emmc_pwr_off)(unsigned port);
};

struct aml_card_sd_info *aml_cpu_sd_emmc_get(unsigned port);
struct mmc *aml_sd_emmc_register(struct aml_card_sd_info *);

#endif
