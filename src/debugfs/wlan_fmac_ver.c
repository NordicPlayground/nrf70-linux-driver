/*
 * Copyright (c) 2023 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: GPL-2.0
 */

#include <linux/debugfs.h>

#include "fmac_api.h"
#include "fmac_dbgfs_if.h"

extern struct nrf_wifi_drv_priv_lnx rpu_drv_priv;

static int nrf_wifi_wlan_fmac_dbgfs_ver_show(struct seq_file *m, void *v)
{
	enum nrf_wifi_status status = NRF_WIFI_STATUS_FAIL;
	struct nrf_wifi_ctx_lnx *rpu_ctx_lnx = NULL;
	struct nrf_wifi_fmac_dev_ctx *fmac_dev_ctx = NULL;
	unsigned int fw_ver;

	rpu_ctx_lnx = (struct nrf_wifi_ctx_lnx *)m->private;

	fmac_dev_ctx = rpu_ctx_lnx->rpu_ctx;

	status = nrf_wifi_fmac_ver_get(fmac_dev_ctx, &fw_ver);

	seq_printf(m, "Driver : %s\n", NRF_WIFI_FMAC_DRV_VER);

	seq_printf(m, "UMAC : %d.%d.%d.%d\n", NRF_WIFI_UMAC_VER(fw_ver),
		   NRF_WIFI_UMAC_VER_MAJ(fw_ver), NRF_WIFI_UMAC_VER_MIN(fw_ver),
		   NRF_WIFI_UMAC_VER_EXTRA(fw_ver));
	return NRF_WIFI_STATUS_SUCCESS;
}

static int open_ver(struct inode *inode, struct file *file)
{
	struct nrf_wifi_ctx_lnx *rpu_ctx_lnx =
		(struct nrf_wifi_ctx_lnx *)inode->i_private;

	return single_open(file, nrf_wifi_wlan_fmac_dbgfs_ver_show,
			   rpu_ctx_lnx);
}

static const struct file_operations fops_ver = { .open = open_ver,
						 .read = seq_read,
						 .llseek = seq_lseek,
						 .write = NULL,
						 .release = single_release };

int nrf_wifi_wlan_fmac_dbgfs_ver_init(struct dentry *root,
				      struct nrf_wifi_ctx_lnx *rpu_ctx_lnx)
{
	int ret = 0;

	if ((!root) || (!rpu_ctx_lnx)) {
		pr_err("%s: Invalid parameters\n", __func__);
		ret = -EINVAL;
		goto fail;
	}

	rpu_drv_priv.dbgfs_ver_root = debugfs_create_file(
		"version", 0444, root, rpu_ctx_lnx, &fops_ver);

	if (!rpu_drv_priv.dbgfs_ver_root) {
		pr_err("%s: Failed to create debugfs entry\n", __func__);
		ret = -ENOMEM;
		goto fail;
	}

	goto out;

fail:
	nrf_wifi_wlan_fmac_dbgfs_ver_deinit();

out:
	return ret;
}

void nrf_wifi_wlan_fmac_dbgfs_ver_deinit(void)
{
	if (rpu_drv_priv.dbgfs_ver_root)
		debugfs_remove(rpu_drv_priv.dbgfs_ver_root);

	rpu_drv_priv.dbgfs_ver_root = NULL;
}
