/*
 * Copyright (c) 2023 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: GPL-2.0
 */

#include <linux/debugfs.h>
#include <linux/fs.h>
#include <linux/etherdevice.h>

#include "fmac_api.h"
#include "fmac_dbgfs_if.h"

#define MAX_CONF_BUF_SIZE 500
#define MAX_ERR_STR_SIZE 80

static __always_inline unsigned char
param_get_val(unsigned char *buf, unsigned char *str, unsigned long *val)
{
	unsigned char *temp = NULL;

	if (strstr(buf, str)) {
		temp = strstr(buf, "=") + 1;

		if (!kstrtoul(temp, 0, val)) {
			return 1;
		} else {
			return 0;
		}
	} else {
		return 0;
	}
}

static int wifi_nrf_wlan_fmac_conf_disp(struct seq_file *m, void *v)
{
	struct wifi_nrf_ctx_lnx *rpu_ctx_lnx =
		(struct wifi_nrf_ctx_lnx *)m->private;
	struct rpu_conf_params *conf_params = NULL;

	conf_params = &rpu_ctx_lnx->conf_params;

	seq_puts(m, "************* Configured Parameters ***********\n");

	seq_printf(m, "uapsd_queue = %d\n", conf_params->uapsd_queue);
	seq_printf(m, "power_save = %s\n",
		   conf_params->power_save ? "ON" : "OFF");
	seq_printf(m, "he_gi = %d\n", conf_params->he_gi);
	return 0;
}

void wifi_nrf_lnx_wlan_fmac_conf_init(struct rpu_conf_params *conf_params)
{
	memset(conf_params, 0, sizeof(*conf_params));

	conf_params->he_gi = -1;
	conf_params->power_save = 0;
}

static ssize_t wifi_nrf_wlan_fmac_conf_write(struct file *file,
					     const char __user *in_buf,
					     size_t count, loff_t *ppos)
{
	char *conf_buf = NULL;
	unsigned long val = 0;
	char err_str[MAX_ERR_STR_SIZE];
	ssize_t ret_val = count;
	struct wifi_nrf_ctx_lnx *rpu_ctx_lnx = NULL;
	enum wifi_nrf_status status = WIFI_NRF_STATUS_FAIL;

	rpu_ctx_lnx = (struct wifi_nrf_ctx_lnx *)file->f_inode->i_private;

	if (count >= MAX_CONF_BUF_SIZE) {
		snprintf(err_str, MAX_ERR_STR_SIZE,
			 "Size of input buffer cannot be more than %d chars\n",
			 MAX_CONF_BUF_SIZE);

		ret_val = -EFAULT;
		goto error;
	}

	conf_buf = kzalloc(MAX_CONF_BUF_SIZE, GFP_KERNEL);

	if (!conf_buf) {
		snprintf(err_str, MAX_ERR_STR_SIZE,
			 "Not enough memory available\n");

		ret_val = -EFAULT;
		goto error;
	}

	if (copy_from_user(conf_buf, in_buf, count)) {
		snprintf(err_str, MAX_ERR_STR_SIZE,
			 "Copy from input buffer failed\n");

		ret_val = -EFAULT;
		goto error;
	}

	conf_buf[count - 1] = '\0';

	if (param_get_val(conf_buf, "uapsd_queue=", &val)) {
		if ((val < 0) || (val > 15)) {
			snprintf(err_str, MAX_ERR_STR_SIZE,
				 "Invalid value %lu\n", val);
			ret_val = -EINVAL;
			goto error;
		}

		if (rpu_ctx_lnx->conf_params.uapsd_queue == val)
			goto out;

		status = wifi_nrf_fmac_set_uapsd_queue(rpu_ctx_lnx->rpu_ctx, 0,
						       val);

		if (status != WIFI_NRF_STATUS_SUCCESS) {
			snprintf(err_str, MAX_ERR_STR_SIZE,
				 "Programming uapsd queue failed\n");
			goto error;
		}
		rpu_ctx_lnx->conf_params.uapsd_queue = val;
		goto error;
	} else if (param_get_val(conf_buf, "power_save=", &val)) {
		if ((val != 0) && (val != 1)) {
			snprintf(err_str, MAX_ERR_STR_SIZE,
				 "Invalid value %lu\n", val);
			ret_val = -EINVAL;
			goto error;
		}

		if (rpu_ctx_lnx->conf_params.power_save == val)
			goto out;

		status = wifi_nrf_fmac_set_power_save(rpu_ctx_lnx->rpu_ctx, 0,
						      val);

		if (status != WIFI_NRF_STATUS_SUCCESS) {
			snprintf(err_str, MAX_ERR_STR_SIZE,
				 "Programming power save failed\n");
			goto error;
		}
		rpu_ctx_lnx->conf_params.power_save = val;
	} else if (param_get_val(conf_buf, "he_gi=", &val)) {
		if ((val < 0) || (val > 2)) {
			snprintf(err_str, MAX_ERR_STR_SIZE,
				 "Invalid value %lu\n", val);
			ret_val = -EINVAL;
			goto error;
		}

		if (rpu_ctx_lnx->conf_params.he_gi == val)
			goto out;
		rpu_ctx_lnx->conf_params.he_gi = val;
	} else {
		snprintf(err_str, MAX_ERR_STR_SIZE,
			 "Invalid parameter name: %s\n", conf_buf);
		ret_val = -EFAULT;
		goto error;
	}

	goto out;

error:
	pr_err("Error condition: %s\n", err_str);
out:
	kfree(conf_buf);

	return ret_val;
}

static int wifi_nrf_wlan_fmac_conf_open(struct inode *inode, struct file *file)
{
	struct wifi_nrf_ctx_lnx *rpu_ctx_lnx =
		(struct wifi_nrf_ctx_lnx *)inode->i_private;

	return single_open(file, wifi_nrf_wlan_fmac_conf_disp, rpu_ctx_lnx);
}

static const struct file_operations fops_wlan_conf = {
	.open = wifi_nrf_wlan_fmac_conf_open,
	.read = seq_read,
	.llseek = seq_lseek,
	.write = wifi_nrf_wlan_fmac_conf_write,
	.release = single_release
};

int wifi_nrf_wlan_fmac_dbgfs_conf_init(struct dentry *root,
				       struct wifi_nrf_ctx_lnx *rpu_ctx_lnx)
{
	int ret = 0;

	if ((!root) || (!rpu_ctx_lnx)) {
		pr_err("%s: Invalid parameters\n", __func__);
		ret = -EINVAL;
		goto fail;
	}

	rpu_ctx_lnx->dbgfs_wlan_conf_root = debugfs_create_file(
		"conf", 0644, root, rpu_ctx_lnx, &fops_wlan_conf);

	if (!rpu_ctx_lnx->dbgfs_wlan_conf_root) {
		pr_err("%s: Failed to create debugfs entry\n", __func__);
		ret = -ENOMEM;
		goto fail;
	}

	goto out;

fail:
	wifi_nrf_wlan_fmac_dbgfs_conf_deinit(rpu_ctx_lnx);
out:
	return ret;
}

void wifi_nrf_wlan_fmac_dbgfs_conf_deinit(struct wifi_nrf_ctx_lnx *rpu_ctx_lnx)
{
	if (rpu_ctx_lnx->dbgfs_wlan_conf_root)
		debugfs_remove(rpu_ctx_lnx->dbgfs_wlan_conf_root);

	rpu_ctx_lnx->dbgfs_wlan_conf_root = NULL;
}
