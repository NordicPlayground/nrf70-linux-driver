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

extern struct nrf_wifi_drv_priv_lnx rpu_drv_priv;
struct nrf_wifi_ctx_lnx *ctx;

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

static __always_inline unsigned char
param_get_sval(unsigned char *buf, unsigned char *str, long *sval)
{
	unsigned char *temp = NULL;

	if (strstr(buf, str)) {
		temp = strstr(buf, "=") + 1;

		if (!kstrtol(temp, 0, sval)) {
			return 1;
		} else {
			return 0;
		}
	} else {
		return 0;
	}
}

static __always_inline unsigned char param_get_match(unsigned char *buf,
						     unsigned char *str)
{
	if (strstr(buf, str))
		return 1;
	else
		return 0;
}

static int nrf_wifi_wlan_fmac_conf_disp(struct seq_file *m, void *v)
{
	struct rpu_conf_params *conf_params = NULL;

	conf_params = &ctx->conf_params;

	seq_puts(m, "************* Configured Parameters ***********\n");

	seq_printf(m, "uapsd_queue = %d\n", conf_params->uapsd_queue);
	seq_printf(m, "power_save = %s\n",
		   conf_params->power_save ? "ON" : "OFF");
	seq_printf(m, "he_gi = %d\n", conf_params->he_gi);
	seq_printf(m, "rts_threshold = %d\n", conf_params->rts_threshold);

	return 0;
}

static ssize_t nrf_wifi_wlan_fmac_conf_write(struct file *file,
					     const char __user *in_buf,
					     size_t count, loff_t *ppos)
{
	char *conf_buf = NULL;
	unsigned long val = 0;
	char err_str[MAX_ERR_STR_SIZE];
	ssize_t err_val = count;
	struct nrf_wifi_ctx_lnx *ctx = NULL;
	enum nrf_wifi_status status = NRF_WIFI_STATUS_FAIL;

	ctx = (struct nrf_wifi_ctx_lnx *)file->f_inode->i_private;

	if (count >= MAX_CONF_BUF_SIZE) {
		snprintf(err_str, MAX_ERR_STR_SIZE,
			 "Size of input buffer cannot be more than %d chars\n",
			 MAX_CONF_BUF_SIZE);

		err_val = -EFAULT;
		goto error;
	}

	conf_buf = kzalloc(MAX_CONF_BUF_SIZE, GFP_KERNEL);

	if (!conf_buf) {
		snprintf(err_str, MAX_ERR_STR_SIZE,
			 "Not enough memory available\n");

		err_val = -EFAULT;
		goto error;
	}

	if (copy_from_user(conf_buf, in_buf, count)) {
		snprintf(err_str, MAX_ERR_STR_SIZE,
			 "Copy from input buffer failed\n");

		err_val = -EFAULT;
		goto error;
	}

	conf_buf[count - 1] = '\0';

	if (param_get_val(conf_buf, "uapsd_queue=", &val)) {
		if ((val < 0) || (val > 15)) {
			snprintf(err_str, MAX_ERR_STR_SIZE,
				 "Invalid value %lu\n", val);
			err_val = -EINVAL;
			goto error;
		}

		if (ctx->conf_params.uapsd_queue == val)
			goto out;

		status = nrf_wifi_fmac_set_uapsd_queue(ctx->rpu_ctx, 0, val);

		if (status != NRF_WIFI_STATUS_SUCCESS) {
			snprintf(err_str, MAX_ERR_STR_SIZE,
				 "Programming uapsd queue failed\n");
			goto error;
		}
		ctx->conf_params.uapsd_queue = val;
		goto error;
	} else if (param_get_val(conf_buf, "power_save=", &val)) {
		if ((val != 0) && (val != 1)) {
			snprintf(err_str, MAX_ERR_STR_SIZE,
				 "Invalid value %lu\n", val);
			err_val = -EINVAL;
			goto error;
		}

		if (ctx->conf_params.power_save == val)
			goto out;

		status = nrf_wifi_fmac_set_power_save(ctx->rpu_ctx, 0, val);

		if (status != NRF_WIFI_STATUS_SUCCESS) {
			snprintf(err_str, MAX_ERR_STR_SIZE,
				 "Programming power save failed\n");
			goto error;
		}
		ctx->conf_params.power_save = val;
	} else if (param_get_val(conf_buf, "he_gi=", &val)) {
		if ((val < 0) || (val > 2)) {
			snprintf(err_str, MAX_ERR_STR_SIZE,
				 "Invalid value %lu\n", val);
			err_val = -EINVAL;
			goto error;
		}

		if (ctx->conf_params.he_gi == val)
			goto out;
		ctx->conf_params.he_gi = val;
	} else if (param_get_val(conf_buf, "rts_threshold=", &val)) {
		struct nrf_wifi_umac_set_wiphy_info *wiphy_info = NULL;

		if (val <= 0) {
			snprintf(err_str, MAX_ERR_STR_SIZE,
				 "Invalid value %lu\n", val);
			err_val = -EINVAL;
			goto error;
		}

		if (ctx->conf_params.rts_threshold == val)
			goto out;

		wiphy_info = kzalloc(sizeof(*wiphy_info), GFP_KERNEL);

		if (!wiphy_info) {
			snprintf(err_str, MAX_ERR_STR_SIZE,
				 "Unable to allocate memory\n");
			goto error;
		}
		wiphy_info->rts_threshold = val;

		status = nrf_wifi_fmac_set_wiphy_params(ctx->rpu_ctx, 0,
							wiphy_info);

		if (status != NRF_WIFI_STATUS_SUCCESS) {
			snprintf(err_str, MAX_ERR_STR_SIZE,
				 "Programming rts_threshold failed\n");
			goto error;
		}
		if (wiphy_info)
			kfree(wiphy_info);

		ctx->conf_params.rts_threshold = val;
	} else {
		snprintf(err_str, MAX_ERR_STR_SIZE,
			 "Invalid parameter name: %s\n", conf_buf);
		err_val = -EFAULT;
		goto error;
	}

	goto out;

error:
	pr_err("Error condition: %s\n", err_str);
out:
	kfree(conf_buf);

	return err_val;
}

static int nrf_wifi_wlan_fmac_conf_open(struct inode *inode, struct file *file)
{
	ctx = (struct nrf_wifi_ctx_lnx *)inode->i_private;

	return single_open(file, nrf_wifi_wlan_fmac_conf_disp, ctx);
}

static const struct file_operations fops_wlan_conf = {
	.open = nrf_wifi_wlan_fmac_conf_open,
	.read = seq_read,
	.llseek = seq_lseek,
	.write = nrf_wifi_wlan_fmac_conf_write,
	.release = single_release
};

void nrf_wifi_wlan_fmac_dbgfs_conf_deinit(struct nrf_wifi_ctx_lnx *ctx)
{
	if (ctx->dbgfs_wlan_conf_root)
		debugfs_remove(ctx->dbgfs_wlan_conf_root);

	ctx->dbgfs_wlan_conf_root = NULL;
}

int nrf_wifi_wlan_fmac_dbgfs_conf_init(struct dentry *root,
				       struct nrf_wifi_ctx_lnx *ctx)
{
	int ret = 0;

	if ((!root) || (!ctx)) {
		pr_err("%s: Invalid parameters\n", __func__);
		ret = -EINVAL;
		goto fail;
	}

	ctx->dbgfs_wlan_conf_root =
		debugfs_create_file("conf", 0644, root, ctx, &fops_wlan_conf);

	if (!ctx->dbgfs_wlan_conf_root) {
		pr_err("%s: Failed to create debugfs entry\n", __func__);
		ret = -ENOMEM;
		goto fail;
	}

	goto out;

fail:
	nrf_wifi_wlan_fmac_dbgfs_conf_deinit(ctx);
out:
	return ret;
}

int nrf_wifi_wlan_fmac_dbgfs_init(struct nrf_wifi_ctx_lnx *rpu_ctx_lnx)
{
	enum nrf_wifi_status status = NRF_WIFI_STATUS_FAIL;

	rpu_ctx_lnx->dbgfs_wlan_root =
		debugfs_create_dir("wifi", rpu_drv_priv.dbgfs_root);

	if (!rpu_ctx_lnx->dbgfs_wlan_root)
		goto out;

	status = nrf_wifi_wlan_fmac_dbgfs_conf_init(
		rpu_ctx_lnx->dbgfs_wlan_root, rpu_ctx_lnx);

	if (status != NRF_WIFI_STATUS_SUCCESS)
		goto out;

	status = nrf_wifi_wlan_fmac_dbgfs_twt_init(rpu_ctx_lnx->dbgfs_wlan_root,
						   rpu_ctx_lnx);

	if (status != NRF_WIFI_STATUS_SUCCESS)
		goto out;

	status = nrf_wifi_wlan_fmac_dbgfs_stats_init(
		rpu_ctx_lnx->dbgfs_wlan_root, rpu_ctx_lnx);

	if (status != NRF_WIFI_STATUS_SUCCESS)
		goto out;

	status = nrf_wifi_wlan_fmac_dbgfs_ver_init(rpu_ctx_lnx->dbgfs_wlan_root,
						   rpu_ctx_lnx);

	if (status != NRF_WIFI_STATUS_SUCCESS)
		goto out;

out:
	if (status != NRF_WIFI_STATUS_SUCCESS)
		nrf_wifi_wlan_fmac_dbgfs_deinit(rpu_ctx_lnx);

	return status;
}

void nrf_wifi_wlan_fmac_dbgfs_deinit(struct nrf_wifi_ctx_lnx *rpu_ctx_lnx)
{
	if (rpu_ctx_lnx->dbgfs_wlan_root)
		debugfs_remove_recursive(rpu_ctx_lnx->dbgfs_wlan_root);

	rpu_ctx_lnx->dbgfs_wlan_root = NULL;
}
