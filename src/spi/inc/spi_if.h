/*
 * Copyright (c) 2023 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: GPL-2.0
 */

/**
 * @brief Header containing SPI device interface specific declarations for the
 * Linux OS layer of the Wi-Fi driver.
 */

#ifndef __SPI_IF_H__
#define __SPI_IF_H__

#include <linux/mutex.h>
#include <linux/spi/spi.h>

#define RPU_WAKEUP_NOW BIT(0) /* WAKEUP RPU - RW */
#define RPU_AWAKE_BIT BIT(1) /* RPU AWAKE FROM SLEEP - RO */
#define RPU_READY_BIT BIT(2) /* RPU IS READY - RO*/

struct spdev_config {
	struct mutex lock;
	unsigned int addrmask;
	unsigned char spi_slave_latency;
	void *dev;
};

struct spdev {
	int (*deinit)(void);
	void *config;
	void *dev;
	int (*init)(struct spdev_config *config);
	int (*write)(unsigned long addr, unsigned int data, int len);
	int (*read)(unsigned long addr, void *data, int len);
	int (*hl_read)(unsigned long addr, void *data, int len);
	int (*cp_to)(unsigned long addr, const void *src, int count);
	int (*cp_from)(void *dst, unsigned long addr, int count);
	void (*hard_reset)(void);
};

int spdev_init(struct spdev_config *config);

int spdev_write(unsigned long addr, unsigned int data, int len);

int spdev_read(unsigned long addr, void *data, int len);

int spdev_hl_read(unsigned long addr, void *data, int len);

int spdev_cp_to(unsigned long addr, const void *src, int count);

int spdev_cp_from(void *dst, unsigned long addr, int count);

int spdev_deinit(void);

struct spdev_config *spdev_defconfig(void);

struct spdev *sp_dev(void);

int spdev_cmd_sleep_rpu(void);

void hard_reset(void);
void get_sleep_stats(uint32_t addr, uint32_t *buff, uint32_t wrd_len);

extern struct device spi_perip;

int spdev_validate_rpu_wake_writecmd(void);
int spdev_cmd_wakeup_rpu(uint32_t data);
int spdev_wait_while_rpu_awake(void);

int spdev_RDSR1(uint8_t *rdsr1);
int spdev_RDSR2(uint8_t *rdsr2);
int spdev_WRSR2(const uint8_t wrsr2);

#ifdef CONFIG_NRF_WIFI_LOW_POWER
int func_rpu_sleep(void);
int func_rpu_wake(void);
int func_rpu_sleep_status(void);
#endif /* CONFIG_NRF_WIFI_LOW_POWER */

int spdev_enable_encryption(uint8_t *key);

#endif /* __SPI_IF_H__ */
