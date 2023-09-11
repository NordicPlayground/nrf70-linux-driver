/*
 * Copyright (c) 2023 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: GPL-2.0
 */

#ifndef __DBGFS_IF_H__
#define __DBGFS_IF_H__

#include "main.h"

#include "fmac_main.h"

int wifi_nrf_dbgfs_init(void);
void wifi_nrf_dbgfs_deinit(void);
int wifi_nrf_wlan_fmac_dbgfs_init(struct wifi_nrf_ctx_lnx *rpu_ctx_lnx);
void wifi_nrf_wlan_fmac_dbgfs_deinit(struct wifi_nrf_ctx_lnx *rpu_ctx_lnx);
int wifi_nrf_wlan_fmac_dbgfs_stats_init(struct dentry *root,
					struct wifi_nrf_ctx_lnx *rpu_ctx_lnx);
void wifi_nrf_wlan_fmac_dbgfs_stats_deinit(struct wifi_nrf_ctx_lnx *rpu_ctx_lnx);
int wifi_nrf_wlan_fmac_dbgfs_ver_init(struct dentry *root,
				      struct wifi_nrf_ctx_lnx *rpu_ctx_lnx);
void wifi_nrf_wlan_fmac_dbgfs_ver_deinit(void);
int wifi_nrf_wlan_fmac_dbgfs_twt_init(struct dentry *root,
				      struct wifi_nrf_ctx_lnx *rpu_ctx_lnx);
void wifi_nrf_wlan_fmac_dbgfs_twt_deinit(struct wifi_nrf_ctx_lnx *rpu_ctx_lnx);

#endif /* __DBGFS_IF_H__ */
