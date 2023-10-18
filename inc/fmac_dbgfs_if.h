/*
 * Copyright (c) 2023 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: GPL-2.0
 */

#ifndef __DBGFS_IF_H__
#define __DBGFS_IF_H__

#include "main.h"

#include "fmac_main.h"

#define MAX_CONF_BUF_SIZE 500
#define MAX_ERR_STR_SIZE 80

int nrf_wifi_dbgfs_init(void);
void nrf_wifi_dbgfs_deinit(void);
int nrf_wifi_wlan_fmac_dbgfs_init(struct nrf_wifi_ctx_lnx *rpu_ctx_lnx);
void nrf_wifi_wlan_fmac_dbgfs_deinit(struct nrf_wifi_ctx_lnx *rpu_ctx_lnx);
int nrf_wifi_wlan_fmac_dbgfs_stats_init(struct dentry *root,
					struct nrf_wifi_ctx_lnx *rpu_ctx_lnx);
void nrf_wifi_wlan_fmac_dbgfs_stats_deinit(struct nrf_wifi_ctx_lnx *rpu_ctx_lnx);
int nrf_wifi_wlan_fmac_dbgfs_ver_init(struct dentry *root,
				      struct nrf_wifi_ctx_lnx *rpu_ctx_lnx);
void nrf_wifi_wlan_fmac_dbgfs_ver_deinit(void);
int nrf_wifi_wlan_fmac_dbgfs_twt_init(struct dentry *root,
				      struct nrf_wifi_ctx_lnx *rpu_ctx_lnx);
void nrf_wifi_wlan_fmac_dbgfs_twt_deinit(struct nrf_wifi_ctx_lnx *rpu_ctx_lnx);
int nrf_wifi_wlan_fmac_dbgfs_conf_init(struct dentry *root,
				       struct nrf_wifi_ctx_lnx *rpu_ctx_lnx);
void nrf_wifi_wlan_fmac_dbgfs_conf_deinit(struct nrf_wifi_ctx_lnx *rpu_ctx_lnx);
#ifdef CONFIG_NRF700X_RADIO_TEST
int nrf_wifi_wlan_fmac_dbgfs_radio_test_init(
	struct nrf_wifi_ctx_lnx *rpu_ctx_lnx);
void nrf_wifi_wlan_fmac_dbgfs_radio_test_deinit(
	struct nrf_wifi_ctx_lnx *rpu_ctx_lnx);
int nrf_wifi_wlan_fmac_dbgfs_radio_test_conf_init(
	struct dentry *root, struct nrf_wifi_ctx_lnx *rpu_ctx_lnx);
#endif /* CONFIG_NRF700X_RADIO_TEST */
#endif /* __DBGFS_IF_H__ */
