/*
 * Copyright (c) 2023 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: GPL-2.0
 */

#ifndef __SHIM_H__
#define __SHIM_H__

struct shim_llist_node {
	struct list_head head;
	void *data;
};

struct shim_llist {
	struct list_head head;
	unsigned int len;
};

struct shim_intr_priv {
	void *intr_callbk_data;
	int (*intr_callbk_fn)(void *intr_callbk_data);
	struct work_struct work;
};

struct work_item {
	struct work_struct work;
	unsigned long data;
	void (*callback)(unsigned long data);
};

/**
 * struct shim_bus_spi_priv - Structure to hold context information for the Linux OS
 *                        shim.
 *
 * This structure maintains the context information necessary for the operation
 * of the Linux shim. Some of the elements of the structure need to be
 * initialized during the initialization of the Linux shim while others need to
 * be kept updated over the duration of the Linux shim operation.
 */
struct shim_bus_spi_priv {
	struct work_struct drv_reg;
	struct spi_driver *spi_drv;
	const struct spi_device_id *spi_dev_id;
	struct spi_device *spi_dev;
	struct gpio_desc *iovdd;
	struct gpio_desc *bucken;
	struct gpio_desc *host_irq;
	void *spdev;
	struct workqueue_struct *wq;
	struct shim_intr_priv intr_priv;
	bool irq_enabled;
	bool dev_added;
	bool dev_init;
};

/**
 * struct shim_bus_spi_dev_ctx - Structure to hold context information for the Linux
 *                                    specific Linux SPI device context.
 * @pcie_priv: Pointer to Linux specific PCIe driver context.
 */
struct shim_bus_spi_dev_ctx {
	struct shim_bus_spi_priv *lnx_spi_priv;
	void *osal_spi_dev_ctx;
	struct wifi_nrf_ctx_lnx *lnx_rpu_ctx;
	struct wifi_nrf_osal_host_map host_map;
	char *dev_name;
	bool is_msi;
};

#endif /* __SHIM_H__ */
