/*
 * Copyright (c) 2023 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: GPL-2.0
 */

/**
 * @brief File containing common functions for RPU hardware interaction
 * using QSPI and SPI that can be invoked by shell or the driver.
 */

#include <linux/spi/spi.h>
#include <linux/irq.h>
#include <linux/gpio/consumer.h>
#include <linux/delay.h>

#include "rpu_hw_if.h"
#include "spi_if.h"

struct gpio_desc *iovdd;
struct gpio_desc *bucken;
struct gpio_desc *host_irq;
struct workqueue_struct *wq;
bool irq_enabled;

static const struct spdev *spi_dev;
static struct spdev_config *cfg;

int rpu_gpio_config(void *dev)
{
	int ret;
	struct spi_device *spi_dev = dev;

	/* IOVDD */
	if (!device_property_present(&spi_dev->dev, "iovdd-gpio")) {
		pr_err("dt_gpio - Error! Device property 'iovdd-gpio' not found!\n");
		return -ENODEV;
	}

	iovdd = devm_gpiod_get(&spi_dev->dev, "iovdd", 0);
	if (IS_ERR(iovdd)) {
		pr_err("Cannot get iovdd gpio handle\n");
		return -ENODEV;
	}

	ret = gpiod_direction_output(iovdd, 0);
	if (ret < 0) {
		pr_err("Cannot set iovdd gpio direction\n");
		return ret;
	}
	pr_debug("Set iovdd direction out\n");

	/* Bucken */
	if (!device_property_present(&spi_dev->dev, "bucken-gpio")) {
		pr_err("dt_gpio - Error! Device property 'bucken' not found!\n");
		return -ENODEV;
	}

	bucken = devm_gpiod_get(&spi_dev->dev, "bucken", 0);
	if (IS_ERR(bucken)) {
		pr_err("Cannot get bucken gpio handle\n");
		return -ENODEV;
	}

	ret = gpiod_direction_output(bucken, 0);
	if (ret < 0) {
		pr_err("Cannot set bucken gpio direction\n");
		return ret;
	}

	pr_debug("Set bucken direction out\n");

	return 0;
}

int rpu_pwron(void)
{
	int ret = -1;

	if (bucken) {
		gpiod_set_value(bucken, 0);
	} else {
		pr_err("BUCKEN GPIO set failed...\n");
		return ret;
	}

	if (iovdd) {
		gpiod_set_value(iovdd, 0);
	} else {
		pr_err("IOVDD GPIO set failed...\n");
		return ret;
	}

	msleep(1);

	if (bucken) {
		gpiod_set_value(bucken, 1);
	} else {
		pr_err("BUCKEN GPIO set failed...\n");
		return ret;
	}

	msleep(1);

	if (iovdd) {
		gpiod_set_value(iovdd, 1);
	} else {
		pr_err("IOVDD GPIO set failed...\n");
		return ret;
	}

	msleep(1);

	return 0;
}

int rpu_spi_init(void *dev)
{
	spi_dev = sp_dev();

	cfg = spdev_defconfig();
	cfg->dev = dev;
	spi_dev->init(cfg);

	return 0;
}

int rpu_sleep(void)
{
	return spdev_cmd_sleep_rpu();
}

int rpu_wakeup(void)
{
	rpu_wrsr2(1);
	rpu_rdsr2();
	rpu_rdsr1();

	return 0;
}

int rpu_sleep_status(void)
{
	return rpu_rdsr1();
}

int rpu_wrsr2(uint8_t data)
{
	spdev_cmd_wakeup_rpu(data);
	return 0;
}

int rpu_rdsr2(void)
{
	return spdev_validate_rpu_wake_writecmd();
}

int rpu_rdsr1(void)
{
	return spdev_wait_while_rpu_awake();
}

int rpu_clks_on(void *dev)
{
	struct spi_device *spi_dev = dev;
	int err;
	uint8_t tx_buffer[] = { 0x02, 0x84, 0x8C, 0x20, 0x00, 0x01, 0x00, 0x00 };
	struct spi_transfer tr = { .tx_buf = tx_buffer,
				   .len = sizeof(tx_buffer) };
	struct spi_message m;

	spi_message_init(&m);
	spi_message_add_tail(&tr, &m);

	err = spi_sync(spi_dev, &m);
	if (err) {
		pr_err("%s: SPI error: %d\n", __func__, err);
		return err;
	}

	pr_debug("RPU Clocks ON...\n");

	return 0;
}

int rpu_enable(void *dev)
{
	rpu_gpio_config(dev);
	rpu_pwron();
	rpu_spi_init(dev);
	rpu_wakeup();
	rpu_clks_on(dev);

	return 0;
}

int rpu_disable(void *dev)
{
	int err = -1;

	if (bucken) {
		gpiod_set_value(bucken, 0);
	} else {
		pr_err("BUCKEN GPIO set failed...\n");
		return err;
	}

	if (iovdd) {
		gpiod_set_value(iovdd, 0);
	} else {
		pr_err("IOVDD GPIO set failed...\n");
		return err;
	}

	return 0;
}
