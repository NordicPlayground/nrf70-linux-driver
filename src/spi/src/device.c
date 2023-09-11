/*
 * Copyright (c) 2023 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: GPL-2.0
 */

/**
 * @brief File containing QSPI device specific definitions for the
 * Zephyr OS layer of the Wi-Fi driver.
 */

#include "spi_if.h"

static struct spdev_config config;

static struct spdev dev = { .init = spdev_init,
			    .read = spdev_read,
			    .write = spdev_write,
			    .hl_read = spdev_hl_read,
			    .cp_to = spdev_cp_to,
			    .cp_from = spdev_cp_from };

struct spdev_config *spdev_defconfig(void)
{
	memset(&config, 0, sizeof(struct spdev_config));

	config.addrmask = 0x000000;

	config.spi_slave_latency = 3;

	return &config;
}

struct spdev *sp_dev(void)
{
	return &dev;
}
