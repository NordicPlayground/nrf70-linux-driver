/*
 * Copyright (c) 2023 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: GPL-2.0
 */

/**
 * @brief Header containing common functions for RPU hardware interaction
 * using SPI that can be invoked by the driver.
 */

#ifndef __RPU_HW_IF_H_
#define __RPU_HW_IF_H_

#include <linux/types.h>

enum {
	SYSBUS = 0,
	EXT_SYS_BUS,
	PBUS,
	PKTRAM,
	GRAM,
	LMAC_ROM,
	LMAC_RET_RAM,
	LMAC_SRC_RAM,
	UMAC_ROM,
	UMAC_RET_RAM,
	UMAC_SRC_RAM,
	NUM_MEM_BLOCKS
};

extern char blk_name[][15];
extern uint32_t rpu_7002_memmap[][3];

int rpu_read(unsigned int addr, void *data, int len);
int rpu_write(unsigned int addr, const void *data, int len);

int rpu_sleep_status(void);
int rpu_sleep(void);
int rpu_wakeup(void);
int rpu_gpio_config(void *dev);
int rpu_wrsr2(uint8_t data);
int rpu_rdsr2(void);
int rpu_rdsr1(void);
int rpu_clks_on(void *dev);
int rpu_enable(void *dev);
int rpu_disable(void *dev);

#endif /* __RPU_HW_IF_H_ */
