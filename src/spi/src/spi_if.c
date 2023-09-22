/*
 * Copyright (c) 2023 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: GPL-2.0
 */

/**
 * @brief File containing SPI device interface specific definitions for the
 * Linux OS layer of the Wi-Fi driver.
 */

#include <linux/printk.h>
#include <linux/spi/spi.h>
#include <linux/delay.h>

#include "spi_if.h"

static struct spdev_config *config;

#define SPI_SPEED_FROM_DTS 0

static void set_spi_speed(unsigned int speed)
{
	struct spi_device *spi_dev = config->dev;
	struct device_node *np = spi_dev->dev.of_node;

	if (speed == SPI_SPEED_FROM_DTS) {
		const unsigned int *max_speed_hz;

		max_speed_hz = of_get_property(np, "spi-max-frequency", NULL);
		if (!max_speed_hz) {
			pr_err("%s: spi-max-frequency property not found\n",
			       __func__);
			return;
		}

		spi_dev->max_speed_hz = *max_speed_hz;
	} else {
		spi_dev->max_speed_hz = speed;
	}
	spi_setup(spi_dev);
}

static int spdev_read_reg32_hl(unsigned int addr, void *data, unsigned int len,
			       unsigned int discard_bytes)
{
	int err;
	struct spi_device *spi_dev = config->dev;

	uint8_t hdr[] = {
		0x0b, /* FASTREAD opcode */
		(addr >> 16) & 0xFF, (addr >> 8) & 0xFF, addr & 0xFF,
		0 /* dummy byte */
	};

	struct spi_transfer tr = { .tx_buf = hdr,
				   .len = sizeof(hdr) + discard_bytes };
	struct spi_transfer tr_payload = { .rx_buf = data, .len = len };
	struct spi_message m;

	spi_message_init(&m);
	spi_message_add_tail(&tr, &m);
	spi_message_add_tail(&tr_payload, &m);

	err = spi_sync(spi_dev, &m);
	if (err) {
		pr_err("%s: SPI error: %d\n", __func__, err);
		return err;
	}

	return 0;
}

int spdev_cp_to(unsigned long addr, const void *src, int count)
{
	int err;
	struct spi_device *spi_dev = config->dev;
	uint8_t hdr[4] = { 0x02, /* PP opcode */
			   (((addr >> 16) & 0xFF) | 0x80), (addr >> 8) & 0xFF,
			   (addr & 0xFF) };
	struct spi_transfer tr = { .tx_buf = hdr, .len = sizeof(hdr) };
	struct spi_transfer tr_payload = { .tx_buf = src, .len = count };
	struct spi_message m;

	spi_message_init(&m);
	spi_message_add_tail(&tr, &m);
	spi_message_add_tail(&tr_payload, &m);

	err = spi_sync(spi_dev, &m);
	if (err < 0) {
		pr_err("%s: SPI error: %d\n", __func__, err);
	}
	return err;
}

int spdev_cp_from(void *dest, unsigned long addr, int count)
{
	int err;
	struct spi_device *spi_dev = config->dev;
	uint8_t hdr[] = {
		0x0b, /* FASTREAD opcode */
		((addr >> 16) & 0xFF) | 0x80, (addr >> 8) & 0xFF, addr & 0xFF,
		0 /* dummy byte */
	};
	struct spi_transfer tr_hdr = { .tx_buf = hdr, .len = sizeof(hdr) };
	struct spi_transfer tr_payload = { .rx_buf = dest, .len = count };
	struct spi_message m;

	if (addr < 0x0C0000) {
		int offset = 0;
		while (count > 0) {
			spdev_read_reg32_hl(addr + offset, dest + offset, 4,
					    4 * config->spi_slave_latency);
			offset += 4;
			count -= 4;
		}
		return 0;
	}

	spi_message_init(&m);
	spi_message_add_tail(&tr_hdr, &m);
	spi_message_add_tail(&tr_payload, &m);

	err = spi_sync(spi_dev, &m);
	if (err) {
		pr_err("%s: SPI error: %d\n", __func__, err);
		return err;
	}

	return 0;
}

int spdev_read_reg(uint32_t reg_addr, uint8_t *reg_value)
{
	int err;
	uint8_t tx_buffer[6] = { reg_addr };
	uint8_t sr[6];
	struct spi_device *spi_dev = config->dev;
	struct spi_message m;
	struct spi_transfer tr = { .tx_buf = tx_buffer,
				   .rx_buf = sr,
				   .len = sizeof(tx_buffer) };

	spi_message_init(&m);
	spi_message_add_tail(&tr, &m);

	err = spi_sync(spi_dev, &m);

	if (err) {
		pr_err("%s: SPI error: %d\n", __func__, err);
		return err;
	}

	if (err == 0)
		*reg_value = sr[1];

	return err;
}

int spdev_write_reg(uint32_t reg_addr, const uint8_t reg_value)
{
	int err;
	uint8_t tx_buffer[] = { reg_addr, reg_value };
	struct spi_device *spi_dev = config->dev;
	struct spi_message m;
	struct spi_transfer tr = { .tx_buf = tx_buffer,
				   .len = sizeof(tx_buffer) };

	spi_message_init(&m);
	spi_message_add_tail(&tr, &m);

	err = spi_sync(spi_dev, &m);

	if (err) {
		pr_err("%s: SPI error: %d\n", __func__, err);
	}

	return err;
}

int spdev_RDSR1(uint8_t *rdsr1)
{
	uint8_t val = 0;

	return spdev_read_reg(0x1F, &val);
}

int spdev_RDSR2(uint8_t *rdsr2)
{
	uint8_t val = 0;

	return spdev_read_reg(0x2F, &val);
}

int spdev_WRSR2(const uint8_t wrsr2)
{
	return spdev_write_reg(0x3F, wrsr2);
}

int _spdev_wait_while_rpu_awake(void)
{
	int ret;
	uint8_t val = 0;
	int ii = 0;

	for (ii = 0; ii < 10; ii++) {
		ret = spdev_read_reg(0x1F, &val);

		if (!ret && (val & RPU_AWAKE_BIT)) {
			break;
		}

		msleep(10);
	}

	return val;
}

/* Wait until RDSR2 confirms RPU_WAKEUP_NOW write is successful */
int spdev_wait_while_rpu_wake_write(void)
{
	int ret;
	uint8_t val = 0;
	int ii = 0;

	for (ii = 0; ii < 10; ii++) {
		ret = spdev_read_reg(0x2F, &val);

		if (!ret && (val & RPU_WAKEUP_NOW)) {
			set_spi_speed(SPI_SPEED_FROM_DTS);
			break;
		}

		msleep(10);
	}

	return ret;
}

#define RPU_SPI_WAKEUP_FREQ_MHZ 8 * 1000 * 1000
int _spdev_cmd_wakeup_rpu(uint32_t data)
{
	/* Always use 8MHz to wake up RPU */
	set_spi_speed(RPU_SPI_WAKEUP_FREQ_MHZ);

	return spdev_write_reg(0x3F, data);
}

unsigned int _spdev_cmd_sleep_rpu(void)
{
	int err;
	uint8_t tx_buffer[] = { 0x3f, 0x0 };
	struct spi_device *spi_dev = config->dev;
	struct spi_message m;
	struct spi_transfer tr = { .tx_buf = tx_buffer,
				   .len = sizeof(tx_buffer) };

	spi_message_init(&m);
	spi_message_add_tail(&tr, &m);

	err = spi_sync(spi_dev, &m);

	if (err) {
		pr_err("%s: SPI error: %d\n", __func__, err);
		return err;
	}

	return 0;
}

int spdev_init(struct spdev_config *dev_config)
{
	config = dev_config;
	config->spi_slave_latency = 3;
	mutex_init(&config->lock);

	set_spi_speed(SPI_SPEED_FROM_DTS);
	return 0;
}

static void spdev_addr_check(unsigned int addr, const void *data,
			     unsigned int len)
{
	/* Unused for now */
}

static int _spdev_write(unsigned long addr, unsigned int val, unsigned int len)
{
	int err;
	struct spi_device *spi_dev = config->dev;
	uint8_t hdr[8] = {
		0x02, /* PP opcode */
		(((addr >> 16) & 0xFF) | 0x80),
		(addr >> 8) & 0xFF,
		(addr & 0xFF),
		(val & 0xFF),
		((val >> 8) & 0xFF),
		((val >> 16) & 0xFF),
		((val >> 24) & 0xFF),
	};
	struct spi_transfer tr = { .tx_buf = hdr, .len = sizeof(hdr) };
	struct spi_message m;

	spi_message_init(&m);
	spi_message_add_tail(&tr, &m);

	err = spi_sync(spi_dev, &m);
	if (err < 0) {
		pr_err("%s: SPI error: %d\n", __func__, err);
	}

	return err;
}

int spdev_write(unsigned long addr, unsigned int data, int len)
{
	int status = -1;

	spdev_addr_check(addr, &data, len);

	addr |= config->addrmask;

	status = _spdev_write(addr, data, len);

	return status;
}

static int _spdev_read(unsigned long addr, void *data, unsigned int len,
		       unsigned int discard_bytes)
{
	int err;
	struct spi_device *spi_dev = config->dev;
	uint8_t hdr[] = {
		0x0b, /* FASTREAD opcode */
		(addr >> 16) & 0xFF, (addr >> 8) & 0xFF, addr & 0xFF,
		0 /* dummy byte */
	};
	struct spi_transfer tr = { .tx_buf = hdr,
				   .len = sizeof(hdr) + discard_bytes };
	struct spi_transfer tr_payload = { .rx_buf = data, .len = len };
	struct spi_message m;

	spi_message_init(&m);
	spi_message_add_tail(&tr, &m);
	spi_message_add_tail(&tr_payload, &m);

	err = spi_sync(spi_dev, &m);
	if (err) {
		pr_err("%s: SPI error: %d\n", __func__, err);
		return err;
	}

	return 0;
}

int spdev_read(unsigned long addr, void *data, int len)
{
	int status;

	spdev_addr_check(addr, data, len);

	addr |= config->addrmask;

	status = _spdev_read(addr, data, len, 0);

	return status;
}

static int spdev_hl_readw(unsigned long addr, void *data)
{
	int status = -1;

	status = spdev_read_reg32_hl(addr, data, 4,
				     4 * config->spi_slave_latency);

	return status;
}

int spdev_hl_read(unsigned long addr, void *data, int len)
{
	int count = 0;

	spdev_addr_check(addr, data, len);

	while (count < (len / 4)) {
		spdev_hl_readw(addr + (4 * count), (char *)data + (4 * count));
		count++;
	}

	return 0;
}

/* ------------------------------added for wifi utils -------------------------------- */

int spdev_cmd_wakeup_rpu(uint32_t data)
{
	return _spdev_cmd_wakeup_rpu(data);
}

int spdev_cmd_sleep_rpu(void)
{
	return _spdev_cmd_sleep_rpu();
}

int spdev_wait_while_rpu_awake(void)
{
	return _spdev_wait_while_rpu_awake();
}

int spdev_validate_rpu_wake_writecmd(void)
{
	return spdev_wait_while_rpu_wake_write();
}
