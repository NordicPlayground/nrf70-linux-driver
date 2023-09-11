/*
 * Copyright (c) 2023 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: GPL-2.0
 */

#include "fmac_dbgfs_if.h"
extern struct wifi_nrf_drv_priv_lnx rpu_drv_priv;

int wifi_nrf_dbgfs_init(void)
{
	int status = -1;

	rpu_drv_priv.dbgfs_root = debugfs_create_dir("nrf", NULL);

	if (!rpu_drv_priv.dbgfs_root)
		goto out;

	status = 0;
out:
	if (status)
		wifi_nrf_dbgfs_deinit();

	return status;
}

void wifi_nrf_dbgfs_deinit(void)
{
	if (rpu_drv_priv.dbgfs_root)
		debugfs_remove_recursive(rpu_drv_priv.dbgfs_root);

	rpu_drv_priv.dbgfs_root = NULL;
}

int wifi_nrf_wlan_fmac_dbgfs_init(struct wifi_nrf_ctx_lnx *rpu_ctx_lnx)
{
	enum wifi_nrf_status status = WIFI_NRF_STATUS_FAIL;

	rpu_ctx_lnx->dbgfs_wlan_root =
		debugfs_create_dir("wifi", rpu_drv_priv.dbgfs_root);

	if (!rpu_ctx_lnx->dbgfs_wlan_root)
		goto out;

	status = wifi_nrf_wlan_fmac_dbgfs_twt_init(rpu_ctx_lnx->dbgfs_wlan_root,
						   rpu_ctx_lnx);

	if (status != WIFI_NRF_STATUS_SUCCESS)
		goto out;

	status = wifi_nrf_wlan_fmac_dbgfs_stats_init(
		rpu_ctx_lnx->dbgfs_wlan_root, rpu_ctx_lnx);

	if (status != WIFI_NRF_STATUS_SUCCESS)
		goto out;

	status = wifi_nrf_wlan_fmac_dbgfs_ver_init(rpu_ctx_lnx->dbgfs_wlan_root,
						   rpu_ctx_lnx);

	if (status != WIFI_NRF_STATUS_SUCCESS)
		goto out;

out:
	if (status != WIFI_NRF_STATUS_SUCCESS)
		wifi_nrf_wlan_fmac_dbgfs_deinit(rpu_ctx_lnx);

	return status;
}

void wifi_nrf_wlan_fmac_dbgfs_deinit(struct wifi_nrf_ctx_lnx *rpu_ctx_lnx)
{
	if (rpu_ctx_lnx->dbgfs_wlan_root)
		debugfs_remove_recursive(rpu_ctx_lnx->dbgfs_wlan_root);

	rpu_ctx_lnx->dbgfs_wlan_root = NULL;
}
