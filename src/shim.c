/*
 * Copyright (c) 2023 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: GPL-2.0
 */

/**
 * @brief Header containing OS specific definitions for the
 * Linux OS layer of the Wi-Fi driver.
 */

#include <linux/slab.h>
#include <linux/spinlock.h>
#include <linux/printk.h>
#include <linux/skbuff.h>
#include <linux/interrupt.h>
#include <linux/delay.h>
#include <linux/dma-mapping.h>
#include <linux/firmware.h>
#include <linux/spi/spi.h>
#include <linux/pci.h>
#include <linux/netdevice.h>
#include <linux/bug.h>
#include <linux/irq.h>
#include <net/cfg80211.h>

#include "osal_api.h"
#include "osal_ops.h"
#include "fmac_api.h"
#include "main.h"
#include "shim.h"
#include "pal.h"
#include "rpu_hw_if.h"
#include "spi_if.h"

#define WIFI_NRF_SPI_DRV_NAME "wifi_nrf_spi"
#define WIFI_NRF_SPI_DEV_NAME "nrf70-spi"

struct of_device_id nrf7002_driver_ids[] = { {
						     .compatible =
							     "nordic,nrf70-spi",
					     },
					     { /* sentinel */ } };

struct spi_device_id nrf7002[] = {
	{ "nrf70-spi", 0 },
	{},
};

static void *shim_mem_alloc(size_t size)
{
	return kmalloc(size, GFP_ATOMIC);
}

static void *shim_mem_zalloc(size_t size)
{
	return kzalloc(size, GFP_ATOMIC);
}

static void shim_mem_free(void *addr)
{
	kfree((const void *)addr);
}

static void *shim_mem_cpy(void *dest, const void *src, size_t count)
{
	return memcpy(dest, src, count);
}

static void *shim_mem_set(void *start, int val, size_t size)
{
	return memset(start, val, size);
}

static void *shim_iomem_mmap(unsigned long addr, unsigned long size)
{
	return ioremap(addr, size);
}

static void shim_iomem_unmap(volatile void *addr)
{
	iounmap(addr);
}

static unsigned int shim_iomem_read_reg32(const volatile void *addr)
{
	return readl(addr);
}

static void shim_iomem_write_reg32(volatile void *addr, unsigned int val)
{
	writel(val, addr);
}

static void shim_iomem_cpy_from(void *dest, const volatile void *src,
				size_t count)
{
	memcpy_fromio(dest, src, count);
}

static void shim_iomem_cpy_to(volatile void *dest, const void *src,
			      size_t count)
{
	memcpy_toio(dest, src, count);
}

static unsigned int shim_spi_read_reg32(void *dev_ctx, unsigned long addr)
{
	unsigned int val;
	struct shim_bus_spi_dev_ctx *lnx_spi_dev_ctx = dev_ctx;
	struct shim_bus_spi_priv *lnx_spi_priv = lnx_spi_dev_ctx->lnx_spi_priv;
	struct spdev *dev = lnx_spi_priv->spdev;

	if (addr < 0x0C0000) {
		dev->hl_read(addr, &val, 4);
	} else {
		dev->read(addr, &val, 4);
	}

	return val;
}

static void shim_spi_write_reg32(void *dev_ctx, unsigned long addr,
				 unsigned int val)
{
	struct shim_bus_spi_dev_ctx *lnx_spi_dev_ctx = dev_ctx;
	struct shim_bus_spi_priv *lnx_spi_priv = lnx_spi_dev_ctx->lnx_spi_priv;
	struct spdev *dev = lnx_spi_priv->spdev;

	dev->write(addr, val, 4);
}

static void shim_spi_cpy_from(void *dev_ctx, void *dest, unsigned long addr,
			      size_t count)
{
	struct shim_bus_spi_dev_ctx *lnx_spi_dev_ctx = dev_ctx;
	struct shim_bus_spi_priv *lnx_spi_priv = lnx_spi_dev_ctx->lnx_spi_priv;
	struct spdev *dev = lnx_spi_priv->spdev;

	if (count % 4 != 0) {
		count = (count + 4) & 0xFFFFFFFC;
	}

	dev->cp_from(dest, addr, count);
}

static void shim_spi_cpy_to(void *dev_ctx, unsigned long addr, const void *src,
			    size_t count)
{
	struct shim_bus_spi_dev_ctx *lnx_spi_dev_ctx = dev_ctx;
	struct shim_bus_spi_priv *lnx_spi_priv = lnx_spi_dev_ctx->lnx_spi_priv;
	struct spdev *dev = lnx_spi_priv->spdev;

	if (count % 4 != 0) {
		count = (count + 4) & 0xFFFFFFFC;
	}

	dev->cp_to(addr, src, count);
}

static void *shim_spinlock_alloc(void)
{
	struct semaphore *lock;

	lock = kmalloc(sizeof(*lock), GFP_KERNEL);

	if (!lock)
		pr_err("%s: Unable to allocate memory for spinlock\n",
		       __func__);

	return lock;
}

static void shim_spinlock_free(void *lock)
{
	kfree(lock);
}

static void shim_spinlock_init(void *lock)
{
	sema_init((struct semaphore *)lock, 1);
}

static void shim_spinlock_take(void *lock)
{
	down((struct semaphore *)lock);
}

static void shim_spinlock_rel(void *lock)
{
	up((struct semaphore *)lock);
}

static void shim_spinlock_irq_take(void *lock, unsigned long *flags)
{
	down((struct semaphore *)lock);
}

static void shim_spinlock_irq_rel(void *lock, unsigned long *flags)
{
	up((struct semaphore *)lock);
}

static int shim_pr_dbg(const char *fmt, va_list args)
{
	char *mod_fmt = NULL;
	int ret = -1;

	mod_fmt = kmalloc(strlen(fmt) + 1 + 3, GFP_ATOMIC);

	if (!mod_fmt) {
		pr_err("%s: Unable to allocate memory for mod_fmt\n", __func__);
		return -1;
	}

	strcpy(mod_fmt, KERN_DEBUG);
	strcat(mod_fmt, fmt);

	ret = vprintk(mod_fmt, args);

	return ret;
}

static int shim_pr_info(const char *fmt, va_list args)
{
	char *mod_fmt = NULL;
	int ret = -1;

	mod_fmt = kmalloc(strlen(fmt) + 1 + 3, GFP_ATOMIC);

	if (!mod_fmt) {
		pr_err("%s: Unable to allocate memory for mod_fmt\n", __func__);
		return -1;
	}

	strcpy(mod_fmt, KERN_INFO);
	strcat(mod_fmt, fmt);

	ret = vprintk(mod_fmt, args);

	return ret;
}

static int shim_pr_err(const char *fmt, va_list args)
{
	char *mod_fmt = NULL;
	int ret = -1;

	mod_fmt = kmalloc(strlen(fmt) + 1 + 3, GFP_ATOMIC);

	if (!mod_fmt) {
		pr_err("%s: Unable to allocate memory for mod_fmt\n", __func__);
		return -1;
	}

	strcpy(mod_fmt, KERN_ERR);
	strcat(mod_fmt, fmt);

	ret = vprintk(mod_fmt, args);

	return ret;
}

static void *shim_nbuf_alloc(unsigned int size)
{
	struct sk_buff *nbuf = NULL;

	nbuf = alloc_skb(size, GFP_ATOMIC);

	if (!nbuf)
		pr_err("%s: Unable to allocate memory for network buffer\n",
		       __func__);

	return nbuf;
}

static void shim_nbuf_free(void *nbuf)
{
	kfree_skb(nbuf);
}

static void shim_nbuf_headroom_res(void *nbuf, unsigned int size)
{
	skb_reserve(nbuf, size);
}

static unsigned int shim_nbuf_headroom_get(void *nbuf)
{
	struct sk_buff *skb = (struct sk_buff *)nbuf;

	return (skb->data - skb->head);
}

static unsigned int shim_nbuf_data_size(void *nbuf)
{
	struct sk_buff *skb = (struct sk_buff *)nbuf;

	return skb->len;
}

static void *shim_nbuf_data_get(void *nbuf)
{
	struct sk_buff *skb = (struct sk_buff *)nbuf;

	return skb->data;
}

static void *shim_nbuf_data_put(void *nbuf, unsigned int size)
{
	return skb_put(nbuf, size);
}

static void *shim_nbuf_data_push(void *nbuf, unsigned int size)
{
	return skb_push(nbuf, size);
}

static void *shim_nbuf_data_pull(void *nbuf, unsigned int size)
{
	return skb_pull(nbuf, size);
}

static unsigned char shim_nbuf_get_priority(void *nbuf)
{
	struct sk_buff *skb = (struct sk_buff *)nbuf;

	/* Note: Linux supports u32 but OSAL supports u8 */
	return skb->priority;
}

static void *shim_llist_node_alloc(void)
{
	struct shim_llist_node *llist_node = NULL;

	llist_node = kzalloc(sizeof(*llist_node), GFP_ATOMIC);

	if (!llist_node)
		pr_err("%s: Unable to allocate memory for linked list node\n",
		       __func__);

	return llist_node;
}

static void shim_llist_node_free(void *llist_node)
{
	kfree(llist_node);
}

static void *shim_llist_node_data_get(void *llist_node)
{
	struct shim_llist_node *shim_llist_node = NULL;

	shim_llist_node = (struct shim_llist_node *)llist_node;

	return shim_llist_node->data;
}

static void shim_llist_node_data_set(void *llist_node, void *data)
{
	struct shim_llist_node *shim_llist_node = NULL;

	shim_llist_node = (struct shim_llist_node *)llist_node;

	shim_llist_node->data = data;
}

static void *shim_llist_alloc(void)
{
	struct shim_llist *llist = NULL;

	llist = kzalloc(sizeof(*llist), GFP_ATOMIC);

	if (!llist)
		pr_err("%s: Unable to allocate memory for linked list\n",
		       __func__);

	return llist;
}

static void shim_llist_free(void *llist)
{
	kfree(llist);
}

static void shim_llist_init(void *llist)
{
	struct shim_llist *shim_llist = NULL;

	shim_llist = (struct shim_llist *)llist;

	INIT_LIST_HEAD(&shim_llist->head);
}

static void shim_llist_add_node_tail(void *llist, void *llist_node)
{
	struct shim_llist *shim_llist = NULL;

	shim_llist = (struct shim_llist *)llist;

	list_add_tail(llist_node, &shim_llist->head);

	shim_llist->len += 1;
}

static void *shim_llist_get_node_head(void *llist)
{
	struct shim_llist_node *head_node = NULL;
	struct shim_llist *shim_llist = NULL;

	shim_llist = (struct shim_llist *)llist;

	if (!shim_llist->len)
		return NULL;

	head_node = list_first_entry(&shim_llist->head, struct shim_llist_node,
				     head);

	return head_node;
}

static void *shim_llist_get_node_nxt(void *llist, void *llist_node)
{
	struct shim_llist_node *node = NULL;
	struct shim_llist_node *nxt_node = NULL;
	struct shim_llist *shim_llist = NULL;

	shim_llist = (struct shim_llist *)llist;
	node = (struct shim_llist_node *)llist_node;

	if (node->head.next == &shim_llist->head)
		return NULL;

	nxt_node = list_next_entry(node, head);

	return nxt_node;
}

static void shim_llist_del_node(void *llist, void *llist_node)
{
	struct shim_llist_node *node = NULL;
	struct shim_llist *shim_llist = NULL;

	shim_llist = (struct shim_llist *)llist;
	node = (struct shim_llist_node *)llist_node;

	list_del(&node->head);

	shim_llist->len -= 1;
}

static unsigned int shim_llist_len(void *llist)
{
	struct shim_llist *shim_llist = NULL;

	shim_llist = (struct shim_llist *)llist;

	return shim_llist->len;
}

static void *shim_tasklet_alloc(int type)
{
	struct work_item *item = NULL;

	item = kcalloc(sizeof(*item), sizeof(char), GFP_KERNEL);

	if (!item) {
		pr_err("%s: Unable to allocate memory for work\n", __func__);
		goto out;
	}
out:
	return item;
}

static void shim_tasklet_free(void *tasklet)
{
	kfree(tasklet);
}

static void fn_worker(struct work_struct *worker)
{
	struct work_item *item_ctx;

	item_ctx = container_of(worker, struct work_item, work);
	item_ctx->callback(item_ctx->data);
}

static void shim_tasklet_init(void *tasklet, void (*callback)(unsigned long),
			      unsigned long data)
{
	struct work_item *item_ctx;

	item_ctx = tasklet;
	item_ctx->data = data;
	item_ctx->callback = callback;

	INIT_WORK(&item_ctx->work, fn_worker);
}

static void shim_tasklet_schedule(void *tasklet)
{
	struct work_item *item_ctx = NULL;

	item_ctx = tasklet;
	schedule_work(&item_ctx->work);
}

static void shim_tasklet_kill(void *tasklet)
{
	struct work_item *item_ctx = NULL;

	item_ctx = tasklet;
	cancel_work_sync(&item_ctx->work);
}

static int shim_msleep(int msecs)
{
	msleep((unsigned int)msecs);

	return 0;
}

static int shim_udelay(int usecs)
{
	udelay((unsigned long)usecs);

	return 0;
}

static unsigned long shim_time_get_curr_us(void)
{
	return ktime_get_real_ns() / 1000;
}

static unsigned int shim_time_elapsed_us(unsigned long start_time_us)
{
	return (unsigned int)(ktime_get_real_ns() / 1000 - start_time_us);
}

#ifdef CONFIG_NRF_WIFI_LOW_POWER

struct shim_timer_data {
	struct timer_list timer;
	void (*callback)(unsigned long);
	unsigned long data;
	struct work_struct work;
};

static void shim_timer_callback(struct timer_list *t)
{
	struct shim_timer_data *timer = from_timer(timer, t, timer);

	schedule_work(&timer->work);
}

static void shim_timer_invoke_callback(struct work_struct *work)
{
	struct shim_timer_data *timer =
		container_of(work, struct shim_timer_data, work);

	timer->callback(timer->data);
}

static void *shim_timer_alloc(void)
{
	struct shim_timer_data *timer = NULL;

	timer = kmalloc(sizeof(*timer), GFP_ATOMIC);

	if (!timer)
		pr_err("%s: Unable to allocate memory for tasklet\n", __func__);

	INIT_WORK(&timer->work, shim_timer_invoke_callback);

	return timer;
}

static void shim_timer_init(void *timer, void (*callback)(unsigned long),
			    unsigned long data)
{
	struct shim_timer_data *timer_data = timer;

	timer_data->callback = callback;
	timer_data->data = data;
	timer_setup(&timer_data->timer, shim_timer_callback, 0);
}

static void shim_timer_free(void *timer)
{
	kfree(timer);
}

static void shim_timer_schedule(void *timer, unsigned long duration)
{
	struct shim_timer_data *timer_data = timer;

	mod_timer(&timer_data->timer, jiffies + msecs_to_jiffies(duration));
}

static void shim_timer_kill(void *timer)
{
	struct shim_timer_data *timer_data = timer;

	del_timer_sync(&timer_data->timer);
}

static int shim_bus_qspi_ps_sleep(void *os_qspi_priv)
{
	(void)os_qspi_priv;

	rpu_sleep();

	return 0;
}

static int shim_bus_qspi_ps_wake(void *os_qspi_priv)
{
	(void)os_qspi_priv;

	rpu_wakeup();

	return 0;
}

static int shim_bus_qspi_ps_status(void *os_qspi_priv)
{
	(void)os_qspi_priv;

	return rpu_sleep_status();
}
#endif

static void shim_assert(int test_val, int val, enum wifi_nrf_assert_op_type op,
			char *msg)
{
	switch (op) {
	case WIFI_NRF_ASSERT_EQUAL_TO:
		WARN(test_val != val, "%s", msg);
		break;
	case WIFI_NRF_ASSERT_NOT_EQUAL_TO:
		WARN(test_val == val, "%s", msg);
		break;
	case WIFI_NRF_ASSERT_LESS_THAN:
		WARN(test_val >= val, "%s", msg);
		break;
	case WIFI_NRF_ASSERT_LESS_THAN_EQUAL_TO:
		WARN(test_val > val, "%s", msg);
		break;
	case WIFI_NRF_ASSERT_GREATER_THAN:
		WARN(test_val <= val, "%s", msg);
		break;
	case WIFI_NRF_ASSERT_GREATER_THAN_EQUAL_TO:
		WARN(test_val < val, "%s", msg);
		break;
	default:
		pr_err("%s: Invalid assertion operation\n", __func__);
	}
}

static int shim_mem_cmp(const void *addr1, const void *addr2, size_t size)
{
	return memcmp(addr1, addr2, size);
}

static unsigned int shim_strlen(const void *str)
{
	return strlen(str);
}

static void shim_bus_spi_reg_drv(struct work_struct *drv_reg_work)
{
	struct shim_bus_spi_priv *lnx_spi_priv = NULL;

	lnx_spi_priv =
		container_of(drv_reg_work, struct shim_bus_spi_priv, drv_reg);

	if (spi_register_driver(lnx_spi_priv->spi_drv)) {
		pr_err("%s: Registration of RPI SPI driver failed\n", __func__);
		kfree(lnx_spi_priv->spi_dev_id);
		kfree(lnx_spi_priv->spi_drv);
		kfree(lnx_spi_priv);
	}
}

static enum wifi_nrf_status shim_bus_spi_dev_init(void *os_spi_dev_ctx)
{
	enum wifi_nrf_status status = WIFI_NRF_STATUS_FAIL;
	struct shim_bus_spi_dev_ctx *lnx_spi_dev_ctx = NULL;
	struct shim_bus_spi_priv *lnx_spi_priv = NULL;

	lnx_spi_dev_ctx = os_spi_dev_ctx;
	lnx_spi_priv = lnx_spi_dev_ctx->lnx_spi_priv;

	lnx_spi_priv->dev_init = true;

	status = WIFI_NRF_STATUS_SUCCESS;

	return status;
}

static void shim_bus_spi_dev_deinit(void *os_spi_dev_ctx)
{
	struct shim_bus_spi_dev_ctx *lnx_spi_dev_ctx = NULL;
	struct shim_bus_spi_priv *lnx_spi_priv = NULL;
	struct spdev *dev = NULL;

	lnx_spi_dev_ctx = os_spi_dev_ctx;
	lnx_spi_priv = lnx_spi_dev_ctx->lnx_spi_priv;
	dev = lnx_spi_priv->spdev;

	dev->deinit();
}

static void *shim_bus_spi_dev_add(void *os_spi_priv, void *osal_spi_dev_ctx)
{
	struct shim_bus_spi_priv *lnx_spi_priv = NULL;
	struct shim_bus_spi_dev_ctx *lnx_spi_dev_ctx = NULL;

	lnx_spi_dev_ctx = kzalloc(sizeof(*lnx_spi_dev_ctx), GFP_ATOMIC);
	if (!lnx_spi_dev_ctx) {
		pr_err("%s: Unable to allocate memory for lnx_spi_dev_ctx\n",
		       __func__);
		goto out;
	}

	lnx_spi_priv = os_spi_priv;

	rpu_enable(lnx_spi_priv->spi_dev);

	lnx_spi_priv->spdev = sp_dev();

	lnx_spi_dev_ctx->lnx_spi_priv = lnx_spi_priv;
	lnx_spi_dev_ctx->osal_spi_dev_ctx = osal_spi_dev_ctx;

	spi_set_drvdata(lnx_spi_priv->spi_dev, lnx_spi_dev_ctx);

	lnx_spi_priv->dev_added = true;

out:
	return lnx_spi_dev_ctx;
}

static void shim_bus_spi_dev_rem(void *os_spi_dev_ctx)
{
	struct shim_bus_spi_priv *lnx_spi_priv = NULL;
	struct shim_bus_spi_dev_ctx *lnx_spi_dev_ctx = NULL;

	lnx_spi_dev_ctx = os_spi_dev_ctx;
	lnx_spi_priv = lnx_spi_dev_ctx->lnx_spi_priv;

	rpu_disable(lnx_spi_priv->spdev);

	if (lnx_spi_priv->wq) {
		destroy_workqueue(lnx_spi_priv->wq);
	}

	spi_set_drvdata(lnx_spi_priv->spi_dev, NULL);
	kfree(lnx_spi_dev_ctx);
}

static int shim_bus_spi_probe(struct spi_device *spi_dev)
{
	enum wifi_nrf_status status = WIFI_NRF_STATUS_FAIL;
	struct shim_bus_spi_priv *lnx_spi_priv = NULL;
	struct shim_bus_spi_dev_ctx *lnx_spi_dev_ctx = NULL;
	struct wifi_nrf_ctx_lnx *lnx_rpu_ctx = NULL;
	int ret = -1;
	const struct spi_device_id *id = spi_get_device_id(spi_dev);

	lnx_spi_priv = (struct shim_bus_spi_priv *)id->driver_data;

	if (lnx_spi_priv->spi_dev) {
		pr_err("%s: Previous detected device still not added\n",
		       __func__);
		goto out;
	}

	lnx_spi_priv->spi_dev = spi_dev;

	lnx_rpu_ctx = wifi_nrf_fmac_dev_add_lnx();

	if (!lnx_rpu_ctx) {
		pr_err("%s: wifi_nrf_fmac_dev_add_lnx failed\n", __func__);
		goto out;
	}

	lnx_spi_dev_ctx = spi_get_drvdata(spi_dev);

	lnx_spi_dev_ctx->lnx_rpu_ctx = lnx_rpu_ctx;

	status = wifi_nrf_fmac_dev_init_lnx(lnx_rpu_ctx);

	if (status != WIFI_NRF_STATUS_SUCCESS) {
		pr_err("%s: wifi_nrf_fmac_dev_init_lnx failed\n", __func__);
		goto out;
	}

	lnx_spi_priv->spi_dev = NULL;

	ret = 0;
out:
	return ret;
}

static int shim_bus_spi_remove(struct spi_device *spi_dev)
{
	struct shim_bus_spi_dev_ctx *lnx_spi_dev_ctx = NULL;
	struct shim_bus_spi_priv *lnx_spi_priv = NULL;

	lnx_spi_dev_ctx = spi_get_drvdata(spi_dev);
	lnx_spi_priv = lnx_spi_dev_ctx->lnx_spi_priv;

	if (lnx_spi_priv->dev_init) {
		wifi_nrf_fmac_dev_deinit_lnx(lnx_spi_dev_ctx->lnx_rpu_ctx);
	}
	if (lnx_spi_priv->dev_added) {
		wifi_nrf_fmac_dev_rem_lnx(lnx_spi_dev_ctx->lnx_rpu_ctx);
	}
	return 0;
}

static void irq_work_handler(struct work_struct *work)
{
	struct shim_intr_priv *intr_priv = NULL;
	int ret = 0;

	intr_priv = (struct shim_intr_priv *)container_of(
		work, struct shim_intr_priv, work);

	if (!intr_priv) {
		pr_err("fail to get back intr priv\n");
	}

	ret = intr_priv->intr_callbk_fn(intr_priv->intr_callbk_data);
	if (ret) {
		pr_err("%s: Interrupt callback failed\n", __func__);
	}
}

static irqreturn_t shim_spi_irq_handler(int irq, void *p)
{
	struct shim_bus_spi_priv *lnx_spi_priv = NULL;

	lnx_spi_priv = (struct shim_bus_spi_priv *)p;

	queue_work(lnx_spi_priv->wq, &lnx_spi_priv->intr_priv.work);

	return IRQ_HANDLED;
}

static enum wifi_nrf_status
shim_bus_spi_intr_reg(void *os_spi_dev_ctx, void *callbk_data,
		      int (*callbk_fn)(void *callbk_data))
{
	struct shim_bus_spi_dev_ctx *lnx_spi_dev_ctx = NULL;
	struct shim_bus_spi_priv *lnx_spi_priv = NULL;
	enum wifi_nrf_status status = WIFI_NRF_STATUS_FAIL;
	unsigned int irq_flags = IRQ_TYPE_EDGE_RISING;
	int ret = -1;
	int irq_number;
	struct gpio_desc *host_irq;

	lnx_spi_dev_ctx = os_spi_dev_ctx;
	lnx_spi_priv = lnx_spi_dev_ctx->lnx_spi_priv;

	lnx_spi_dev_ctx = os_spi_dev_ctx;

	lnx_spi_priv->wq = alloc_workqueue("nRF7002 WQ", WQ_HIGHPRI, 0);

	if (lnx_spi_priv->wq == NULL) {
		pr_err("Cannot allocate nRF7002 WQ\n");
		goto out;
	}

	lnx_spi_priv->intr_priv.intr_callbk_data = callbk_data;
	lnx_spi_priv->intr_priv.intr_callbk_fn = callbk_fn;

	host_irq = devm_gpiod_get(&lnx_spi_priv->spi_dev->dev, "irq", 0);

	if (IS_ERR(host_irq)) {
		goto out;
	}

	ret = gpiod_direction_input(host_irq);
	if (ret < 0) {
		pr_err("Cannot set irq gpio direction\n");
		goto out;
	} else {
		pr_debug("Set irq direction in\n");
	}

	lnx_spi_priv->host_irq = host_irq;

	irq_number = gpiod_to_irq(lnx_spi_priv->host_irq);

	if (irq_number) {
		ret = request_irq(irq_number, shim_spi_irq_handler, irq_flags,
				  "NRF7002 IRQ", lnx_spi_priv);
		if (ret < 0) {
			pr_err("Cannot request irq\n");
			free_irq(irq_number, NULL);
			lnx_spi_priv->irq_enabled = false;
			goto out;
		} else {
			pr_debug("IRQ requested\n");
			lnx_spi_priv->irq_enabled = true;
		}
	}

	INIT_WORK(&lnx_spi_priv->intr_priv.work, irq_work_handler);
	status = WIFI_NRF_STATUS_SUCCESS;
out:
	return status;
}

static void shim_bus_spi_intr_unreg(void *os_spi_dev_ctx)
{
	struct shim_bus_spi_priv *lnx_spi_priv = NULL;
	struct shim_bus_spi_dev_ctx *lnx_spi_dev_ctx = NULL;

	lnx_spi_dev_ctx = os_spi_dev_ctx;
	lnx_spi_priv = lnx_spi_dev_ctx->lnx_spi_priv;

	free_irq(gpiod_to_irq(lnx_spi_priv->host_irq), lnx_spi_priv);

	lnx_spi_priv->irq_enabled = false;
}

static void
shim_bus_spi_dev_host_map_get(void *os_spi_dev_ctx,
			      struct wifi_nrf_osal_host_map *host_map)
{
	if (!os_spi_dev_ctx || !host_map) {
		pr_err("%s: Invalid parameters\n", __func__);
		return;
	}

	host_map->addr = 0;
}

static void *shim_bus_spi_init(void)
{
	struct spi_driver *lnx_spi_drv = NULL;
	struct spi_device_id *lnx_spi_dev_id = NULL;

	struct shim_bus_spi_priv *lnx_spi_priv = NULL;

	lnx_spi_priv = kzalloc(sizeof(*lnx_spi_priv), sizeof(char));

	if (!lnx_spi_priv) {
		pr_err("%s: Unable to allocate memory for spi_priv\n",
		       __func__);
		goto out;
	}

	lnx_spi_drv = kzalloc(sizeof(*lnx_spi_drv), GFP_ATOMIC);

	if (!lnx_spi_drv) {
		pr_err("%s: Unable to allocate memory for pcie_drv\n",
		       __func__);
		kfree(lnx_spi_drv);
		lnx_spi_drv = NULL;
		goto out;
	}

	lnx_spi_dev_id = kzalloc(sizeof(*lnx_spi_dev_id), GFP_ATOMIC);

	if (!lnx_spi_dev_id) {
		pr_err("%s: Unable to allocate memory for pdev_id\n", __func__);
		kfree(lnx_spi_drv);
		lnx_spi_drv = NULL;
		kfree(lnx_spi_priv);
		lnx_spi_priv = NULL;
		goto out;
	}

	lnx_spi_priv->spi_drv = lnx_spi_drv;
	lnx_spi_priv->spi_dev_id = lnx_spi_dev_id;

	lnx_spi_dev_id = &nrf7002[0];
	memcpy(lnx_spi_dev_id->name, WIFI_NRF_SPI_DEV_NAME,
	       sizeof(WIFI_NRF_SPI_DEV_NAME));
	lnx_spi_dev_id->driver_data = (kernel_ulong_t)lnx_spi_priv;

	lnx_spi_drv->driver.name = WIFI_NRF_SPI_DRV_NAME;
	lnx_spi_drv->driver.of_match_table = of_match_ptr(nrf7002_driver_ids);
	lnx_spi_drv->id_table = lnx_spi_dev_id;
	lnx_spi_drv->probe = shim_bus_spi_probe;
	lnx_spi_drv->remove = shim_bus_spi_remove;

	INIT_WORK(&lnx_spi_priv->drv_reg, shim_bus_spi_reg_drv);

	schedule_work(&lnx_spi_priv->drv_reg);

	lnx_spi_priv->dev_init = true;

out:
	return lnx_spi_priv;
}

static void shim_bus_spi_deinit(void *os_spi_priv)
{
	struct shim_bus_spi_priv *lnx_spi_priv = NULL;

	lnx_spi_priv = os_spi_priv;

	spi_unregister_driver(lnx_spi_priv->spi_drv);

	kfree(lnx_spi_priv->spi_drv);
	lnx_spi_priv->spi_drv = NULL;

	kfree(lnx_spi_priv->spi_dev_id);
	lnx_spi_priv->spi_dev_id = NULL;

	kfree(lnx_spi_priv);
}

const struct wifi_nrf_osal_ops wifi_nrf_os_ops = {
	.mem_alloc = shim_mem_alloc,
	.mem_zalloc = shim_mem_zalloc,
	.mem_free = shim_mem_free,
	.mem_cpy = shim_mem_cpy,
	.mem_set = shim_mem_set,
	.mem_cmp = shim_mem_cmp,

	.iomem_mmap = shim_iomem_mmap,
	.iomem_unmap = shim_iomem_unmap,
	.iomem_read_reg32 = shim_iomem_read_reg32,
	.iomem_write_reg32 = shim_iomem_write_reg32,
	.iomem_cpy_from = shim_iomem_cpy_from,
	.iomem_cpy_to = shim_iomem_cpy_to,

	.spi_read_reg32 = shim_spi_read_reg32,
	.spi_write_reg32 = shim_spi_write_reg32,
	.spi_cpy_from = shim_spi_cpy_from,
	.spi_cpy_to = shim_spi_cpy_to,

	.spinlock_alloc = shim_spinlock_alloc,
	.spinlock_free = shim_spinlock_free,
	.spinlock_init = shim_spinlock_init,
	.spinlock_take = shim_spinlock_take,
	.spinlock_rel = shim_spinlock_rel,

	.spinlock_irq_take = shim_spinlock_irq_take,
	.spinlock_irq_rel = shim_spinlock_irq_rel,

	.log_dbg = shim_pr_dbg,
	.log_info = shim_pr_info,
	.log_err = shim_pr_err,

	.llist_node_alloc = shim_llist_node_alloc,
	.llist_node_free = shim_llist_node_free,
	.llist_node_data_get = shim_llist_node_data_get,
	.llist_node_data_set = shim_llist_node_data_set,

	.llist_alloc = shim_llist_alloc,
	.llist_free = shim_llist_free,
	.llist_init = shim_llist_init,
	.llist_add_node_tail = shim_llist_add_node_tail,
	.llist_get_node_head = shim_llist_get_node_head,
	.llist_get_node_nxt = shim_llist_get_node_nxt,
	.llist_del_node = shim_llist_del_node,
	.llist_len = shim_llist_len,

	.nbuf_alloc = shim_nbuf_alloc,
	.nbuf_free = shim_nbuf_free,
	.nbuf_headroom_res = shim_nbuf_headroom_res,
	.nbuf_headroom_get = shim_nbuf_headroom_get,
	.nbuf_data_size = shim_nbuf_data_size,
	.nbuf_data_get = shim_nbuf_data_get,
	.nbuf_data_put = shim_nbuf_data_put,
	.nbuf_data_push = shim_nbuf_data_push,
	.nbuf_data_pull = shim_nbuf_data_pull,
	.nbuf_get_priority = shim_nbuf_get_priority,

	.tasklet_alloc = shim_tasklet_alloc,
	.tasklet_free = shim_tasklet_free,
	.tasklet_init = shim_tasklet_init,
	.tasklet_schedule = shim_tasklet_schedule,
	.tasklet_kill = shim_tasklet_kill,

	.sleep_ms = shim_msleep,
	.delay_us = shim_udelay,
	.time_get_curr_us = shim_time_get_curr_us,
	.time_elapsed_us = shim_time_elapsed_us,

	.bus_spi_init = shim_bus_spi_init,
	.bus_spi_deinit = shim_bus_spi_deinit,
	.bus_spi_dev_add = shim_bus_spi_dev_add,
	.bus_spi_dev_rem = shim_bus_spi_dev_rem,
	.bus_spi_dev_init = shim_bus_spi_dev_init,
	.bus_spi_dev_deinit = shim_bus_spi_dev_deinit,
	.bus_spi_dev_intr_reg = shim_bus_spi_intr_reg,
	.bus_spi_dev_intr_unreg = shim_bus_spi_intr_unreg,
	.bus_spi_dev_host_map_get = shim_bus_spi_dev_host_map_get,

#ifdef CONFIG_NRF_WIFI_LOW_POWER
	.timer_alloc = shim_timer_alloc,
	.timer_init = shim_timer_init,
	.timer_free = shim_timer_free,
	.timer_schedule = shim_timer_schedule,
	.timer_kill = shim_timer_kill,

	.bus_qspi_ps_sleep = shim_bus_qspi_ps_sleep,
	.bus_qspi_ps_wake = shim_bus_qspi_ps_wake,
	.bus_qspi_ps_status = shim_bus_qspi_ps_status,
#endif
	.assert = shim_assert,
	.strlen = shim_strlen,
};

const struct wifi_nrf_osal_ops *get_os_ops(void)
{
	return &wifi_nrf_os_ops;
}
