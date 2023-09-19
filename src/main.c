/*
 * Copyright (c) 2023 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: GPL-2.0
 */

#include <linux/etherdevice.h>
#include <linux/firmware.h>
#include <linux/rtnetlink.h>
#include "main.h"
#include "fmac_dbgfs_if.h"
#include "pal.h"

#include "util.h"
#include "fmac_util.h"
#include "fmac_api.h"
#include "fmac_main.h"
#include "net_stack.h"
#include "cfg80211_if.h"
#include "rpu_fw_patches.h"

#ifndef CONFIG_NRF700X_RADIO_TEST
char *base_mac_addr = "0019F5331179";
module_param(base_mac_addr, charp, 0000);
MODULE_PARM_DESC(base_mac_addr, "Configure the WiFi base MAC address");

char *rf_params = NULL;
module_param(rf_params, charp, 0000);
MODULE_PARM_DESC(rf_params, "Configure the RF parameters");

#ifdef CONFIG_WIFI_NRF_LOW_POWER
char *sleep_type = NULL;
module_param(sleep_type, charp, 0000);
MODULE_PARM_DESC(sleep_type, "Configure the sleep type parameter");
#endif

#endif /* !CONFIG_NRF700X_RADIO_TEST */

unsigned int phy_calib = NRF_WIFI_DEF_PHY_CALIB;

module_param(phy_calib, uint, 0000);
MODULE_PARM_DESC(phy_calib,
		 "Configure the bitmap of the PHY calibrations required");

unsigned char aggregation = 1;
unsigned char wmm = 1;
unsigned char max_num_tx_agg_sessions = 4;
unsigned char max_num_rx_agg_sessions = 8;
unsigned char reorder_buf_size = 64;
unsigned char max_rxampdu_size = MAX_RX_AMPDU_SIZE_64KB;

unsigned char max_tx_aggregation = 4;

unsigned int rx1_num_bufs = 21;
unsigned int rx2_num_bufs = 21;
unsigned int rx3_num_bufs = 21;

unsigned int rx1_buf_sz = 1600;
unsigned int rx2_buf_sz = 1600;
unsigned int rx3_buf_sz = 1600;

unsigned char rate_protection_type = 0;

module_param(aggregation, byte, 0000);
module_param(wmm, byte, 0000);
module_param(max_num_tx_agg_sessions, byte, 0000);
module_param(max_num_rx_agg_sessions, byte, 0000);
module_param(max_tx_aggregation, byte, 0000);
module_param(reorder_buf_size, byte, 0000);
module_param(max_rxampdu_size, byte, 0000);

module_param(rx1_num_bufs, int, 0000);
module_param(rx2_num_bufs, int, 0000);
module_param(rx3_num_bufs, int, 0000);

module_param(rx1_buf_sz, int, 0000);
module_param(rx2_buf_sz, int, 0000);
module_param(rx3_buf_sz, int, 0000);

module_param(rate_protection_type, byte, 0000);

MODULE_PARM_DESC(aggregation, "Enable (1) / disable (0) AMPDU aggregation");
MODULE_PARM_DESC(wmm, "Enable (1) / Disable (0) WMM");
MODULE_PARM_DESC(max_num_tx_agg_sessions, "Max Tx AMPDU sessions");
MODULE_PARM_DESC(max_num_rx_agg_sessions, "Max Rx AMPDU sessions");
MODULE_PARM_DESC(reorder_buf_size, "Reorder Buffer size (1 to 64)");
MODULE_PARM_DESC(max_rxampdu_size,
		 "Rx AMPDU size (0 to 3 for 8K, 16K, 32K, 64K)");

MODULE_PARM_DESC(rx1_num_bufs, "Rx1 queue size");
MODULE_PARM_DESC(rx2_num_bufs, "Rx2 queue size");
MODULE_PARM_DESC(rx3_num_bufs, "Rx3 queue size");

MODULE_PARM_DESC(rx1_buf_sz, "Rx1 pkt size");
MODULE_PARM_DESC(rx2_buf_sz, "Rx2 pkt size");
MODULE_PARM_DESC(rx3_buf_sz, "Rx3 pkt size");

MODULE_PARM_DESC(rate_protection_type, "0 (NONE), 1 (RTS/CTS), 2 (CTS2SELF)");

struct wifi_nrf_drv_priv_lnx rpu_drv_priv;

#ifndef CONFIG_NRF700X_RADIO_TEST
struct wifi_nrf_fmac_vif_ctx_lnx *
wifi_nrf_wlan_fmac_add_vif(struct wifi_nrf_ctx_lnx *rpu_ctx_lnx,
			   const char *name, char *mac_addr,
			   enum nl80211_iftype if_type)
{
	struct wifi_nrf_fmac_vif_ctx_lnx *vif_ctx_lnx = NULL;
	struct wireless_dev *wdev = NULL;

	/* Create a cfg80211 VIF */
	wdev = kzalloc(sizeof(struct wireless_dev), GFP_KERNEL);

	if (!wdev) {
		pr_err("%s: Unable to allocate memory for wdev\n", __func__);
		goto out;
	}

	wdev->wiphy = rpu_ctx_lnx->wiphy;
	wdev->iftype = if_type;

	/* Create the interface and register it to the netdev stack */
	vif_ctx_lnx =
		wifi_nrf_netdev_add_vif(rpu_ctx_lnx, name, wdev, mac_addr);

	if (!vif_ctx_lnx) {
		pr_err("%s: Failed to add interface to netdev stack\n",
		       __func__);
		kfree(wdev);
		goto out;
	}

	vif_ctx_lnx->wdev = wdev;

out:
	return vif_ctx_lnx;
}

void wifi_nrf_wlan_fmac_del_vif(struct wifi_nrf_fmac_vif_ctx_lnx *vif_ctx_lnx)
{
	struct net_device *netdev = NULL;
	struct wireless_dev *wdev = NULL;

	netdev = vif_ctx_lnx->netdev;

	/* Unregister the default interface from the netdev stack */
	wifi_nrf_netdev_del_vif(netdev);

	wdev = vif_ctx_lnx->wdev;
	kfree(wdev);
}
#endif /* !CONFIG_NRF700X_RADIO_TEST */

static char *
wifi_nrf_fmac_fw_loc_get_lnx(struct wifi_nrf_fmac_dev_ctx *fmac_dev_ctx,
			     enum wifi_nrf_fw_type fw_type,
			     enum wifi_nrf_fw_subtype fw_subtype)
{
	char *fw_loc = NULL;

	fw_loc = pal_ops_get_fw_loc(fmac_dev_ctx->fpriv->opriv, fw_type,
				    fw_subtype);

	return fw_loc;
}

enum wifi_nrf_status
wifi_nrf_fmac_fw_get_lnx(struct wifi_nrf_fmac_dev_ctx *fmac_dev_ctx,
			 struct device *dev, enum wifi_nrf_fw_type fw_type,
			 enum wifi_nrf_fw_subtype fw_subtype,
			 struct wifi_nrf_fw_info *fw_info)
{
	enum wifi_nrf_status status = WIFI_NRF_STATUS_SUCCESS;
	const struct firmware *fw = NULL;
	char *fw_loc = NULL;
	const unsigned char *fw_data = NULL;
	unsigned int fw_data_size = 0;
	int ret = -1;

	if (fw_type == WIFI_NRF_FW_TYPE_LMAC_PATCH) {
		if (fw_subtype == WIFI_NRF_FW_SUBTYPE_PRI) {
			fw_data = wifi_nrf_lmac_patch_pri_bimg;
			fw_data_size = sizeof(wifi_nrf_lmac_patch_pri_bimg);
		} else if (fw_subtype == WIFI_NRF_FW_SUBTYPE_SEC) {
			fw_data = wifi_nrf_lmac_patch_sec_bin;
			fw_data_size = sizeof(wifi_nrf_lmac_patch_sec_bin);
		} else {
			pr_err("%s: Invalid fw_subtype (%d)\n", __func__,
			       fw_subtype);
		}
	} else if (fw_type == WIFI_NRF_FW_TYPE_UMAC_PATCH) {
		if (fw_subtype == WIFI_NRF_FW_SUBTYPE_PRI) {
			fw_data = wifi_nrf_umac_patch_pri_bimg;
			fw_data_size = sizeof(wifi_nrf_umac_patch_pri_bimg);
		} else if (fw_subtype == WIFI_NRF_FW_SUBTYPE_SEC) {
			fw_data = wifi_nrf_umac_patch_sec_bin;
			fw_data_size = sizeof(wifi_nrf_umac_patch_sec_bin);
		} else {
			pr_err("%s: Invalid fw_subtype (%d)\n", __func__,
			       fw_subtype);
		}
	} else {
		fw_loc = wifi_nrf_fmac_fw_loc_get_lnx(fmac_dev_ctx, fw_type,
						      fw_subtype);

		if (!fw_loc) {
			/* Nothing to be downloaded */
			goto out;
		}

		ret = request_firmware(&fw, fw_loc, dev);

		if (ret) {
			pr_err("Failed to get %s, Error = %d\n", fw_loc, ret);
			fw = NULL;

			/* It is possible that FW patches are not present, so not being
			 * able to get them is not a failure case */
			if ((fw_type != WIFI_NRF_FW_TYPE_LMAC_PATCH) &&
			    (fw_type != WIFI_NRF_FW_TYPE_UMAC_PATCH))
				status = WIFI_NRF_STATUS_FAIL;
			goto out;
		}

		fw_data = fw->data;
		fw_data_size = fw->size;
	}

	fw_info->data = kzalloc(fw_data_size, GFP_KERNEL);

	if (!fw_info->data) {
		pr_err("%s: Unable to allocate memory\n", __func__);
		status = WIFI_NRF_STATUS_FAIL;
		goto out;
	}

	memcpy(fw_info->data, fw_data, fw_data_size);

	fw_info->size = fw_data_size;

out:
	if (fw)
		release_firmware(fw);

	return status;
}

void wifi_nrf_wlan_fw_rel(struct wifi_nrf_fw_info *fw_info)
{
	if (fw_info->data)
		kfree(fw_info->data);

	fw_info->data = 0;
}

enum wifi_nrf_status
wifi_nrf_wlan_fmac_fw_load(struct wifi_nrf_ctx_lnx *rpu_ctx_lnx,
			   struct device *dev)
{
	struct wifi_nrf_fmac_fw_info fw_info;
	enum wifi_nrf_status status = WIFI_NRF_STATUS_FAIL;

	memset(&fw_info, 0, sizeof(fw_info));

	/* Get the LMAC patches */
	status = wifi_nrf_fmac_fw_get_lnx(rpu_ctx_lnx->rpu_ctx, dev,
					  WIFI_NRF_FW_TYPE_LMAC_PATCH,
					  WIFI_NRF_FW_SUBTYPE_PRI,
					  &fw_info.lmac_patch_pri);

	if (status != WIFI_NRF_STATUS_SUCCESS) {
		pr_err("%s: Unable to get LMAC BIMG patch\n", __func__);
		goto out;
	}

	status = wifi_nrf_fmac_fw_get_lnx(rpu_ctx_lnx->rpu_ctx, dev,
					  WIFI_NRF_FW_TYPE_LMAC_PATCH,
					  WIFI_NRF_FW_SUBTYPE_SEC,
					  &fw_info.lmac_patch_sec);

	if (status != WIFI_NRF_STATUS_SUCCESS) {
		pr_err("%s: Unable to get LMAC BIN patch\n", __func__);
		goto out;
	}

	/* Get the UMAC patches */
	status = wifi_nrf_fmac_fw_get_lnx(rpu_ctx_lnx->rpu_ctx, dev,
					  WIFI_NRF_FW_TYPE_UMAC_PATCH,
					  WIFI_NRF_FW_SUBTYPE_PRI,
					  &fw_info.umac_patch_pri);

	if (status != WIFI_NRF_STATUS_SUCCESS) {
		pr_err("%s: Unable to get UMAC BIMG patch\n", __func__);
		goto out;
	}

	status = wifi_nrf_fmac_fw_get_lnx(rpu_ctx_lnx->rpu_ctx, dev,
					  WIFI_NRF_FW_TYPE_UMAC_PATCH,
					  WIFI_NRF_FW_SUBTYPE_SEC,
					  &fw_info.umac_patch_sec);

	if (status != WIFI_NRF_STATUS_SUCCESS) {
		pr_err("%s: Unable to get UMAC BIN patch\n", __func__);
		goto out;
	}

	/* Load the FW's to the RPU */
	status = wifi_nrf_fmac_fw_load(rpu_ctx_lnx->rpu_ctx, &fw_info);

	if (status != WIFI_NRF_STATUS_SUCCESS) {
		pr_err("%s: wifi_nrf_fmac_fw_load failed\n", __func__);
		goto out;
	}

	status = WIFI_NRF_STATUS_SUCCESS;

out:
	wifi_nrf_wlan_fw_rel(&fw_info.lmac_patch_pri);
	wifi_nrf_wlan_fw_rel(&fw_info.lmac_patch_sec);
	wifi_nrf_wlan_fw_rel(&fw_info.umac_patch_pri);
	wifi_nrf_wlan_fw_rel(&fw_info.umac_patch_sec);

	return status;
}

struct wifi_nrf_ctx_lnx *wifi_nrf_fmac_dev_add_lnx(void)
{
	enum wifi_nrf_status status = WIFI_NRF_STATUS_FAIL;
	struct wifi_nrf_ctx_lnx *rpu_ctx_lnx = NULL;
	void *rpu_ctx = NULL;
	struct device *dev = NULL;
	struct wiphy *wiphy = NULL;
	unsigned char i = 0;

	while ((!rpu_drv_priv.drv_init) && (i < 5)) {
		pr_debug("%s: Driver not yet initialized, waiting %d\n",
			 __func__, i);
		i++;
		msleep(10);
	}

	if (i == 5) {
		pr_err("%s: Driver init failed\n", __func__);
		goto out;
	}

	/* Register the RPU to the cfg80211 stack */
	wiphy = cfg80211_if_init();

	if (!wiphy) {
		pr_err("%s: cfg80211_if_init failed\n", __func__);
		goto out;
	}

	rpu_ctx_lnx = wiphy_priv(wiphy);

	rpu_ctx_lnx->wiphy = wiphy;
	dev = &wiphy->dev;

	INIT_LIST_HEAD(&rpu_ctx_lnx->cookie_list);

	rpu_ctx = wifi_nrf_fmac_dev_add(rpu_drv_priv.fmac_priv, rpu_ctx_lnx);

	if (!rpu_ctx) {
		pr_err("%s: wifi_nrf_fmac_dev_add failed\n", __func__);
		cfg80211_if_deinit(wiphy);
		rpu_ctx_lnx = NULL;
		goto out;
	}

	rpu_ctx_lnx->rpu_ctx = rpu_ctx;

	/* Load the firmware to the RPU */
	status = wifi_nrf_wlan_fmac_fw_load(rpu_ctx_lnx, dev);

	if (status != WIFI_NRF_STATUS_SUCCESS) {
		pr_err("%s: FW is not booted up\n", __func__);
		cfg80211_if_deinit(wiphy);
		rpu_ctx_lnx = NULL;
		goto out;
	}

out:

	return rpu_ctx_lnx;
}

void wifi_nrf_fmac_dev_rem_lnx(struct wifi_nrf_ctx_lnx *rpu_ctx_lnx)
{
	void *fmac_dev_ctx = rpu_ctx_lnx->rpu_ctx;
	cfg80211_if_deinit(rpu_ctx_lnx->wiphy);
	wifi_nrf_fmac_dev_rem(fmac_dev_ctx);
}

enum wifi_nrf_status
wifi_nrf_fmac_dev_init_lnx(struct wifi_nrf_ctx_lnx *rpu_ctx_lnx)
{
	enum wifi_nrf_status status = WIFI_NRF_STATUS_FAIL;
	unsigned char base_mac_addr[NRF_WIFI_ETH_ADDR_LEN];
#ifndef CONFIG_NRF700X_RADIO_TEST
	struct nrf_wifi_umac_add_vif_info add_vif_info;
#endif /* !CONFIG_NRF700X_RADIO_TEST */
	struct wifi_nrf_fmac_vif_ctx_lnx *vif_ctx_lnx = NULL;

#if defined(CONFIG_BOARD_NRF7001)
	enum op_band op_band = BAND_24G;
#else /* CONFIG_BOARD_NRF7001 */
	enum op_band op_band = BAND_ALL;
#endif /* CONFIG_BOARD_NRF7001 */

#ifdef CONFIG_WIFI_NRF_LOW_POWER
	int sleep_type = -1;
#ifndef CONFIG_NRF700X_RADIO_TEST
	sleep_type = HW_SLEEP_ENABLE;
#else
	sleep_type = SLEEP_DISABLE;
#endif /* CONFIG_NRF700X_RADIO_TEST */
#endif /* CONFIG_NRF_WIFI_LOW_POWER */

	unsigned int fw_ver = 0;
	struct nrf_wifi_tx_pwr_ctrl_params tx_pwr_ctrl_params;

#ifndef CONFIG_NRF700X_RADIO_TEST
	status = wifi_nrf_fmac_otp_mac_addr_get(rpu_ctx_lnx->rpu_ctx, 0,

						base_mac_addr);

	if (status != WIFI_NRF_STATUS_SUCCESS) {
		pr_err("%s: Fetching of MAC address from OTP failed\n",
		       __func__);
		goto out;
	}

	/* Create a default interface and register it to the netdev stack.
	 * We need to take a rtnl_lock since the netdev stack expects it. In the
	 * regular case this lock is taken by the cfg80211, but in the case of
	 * the default interface we need to take it since we are initiating the
	 * creation of the interface */
	rtnl_lock();

	vif_ctx_lnx = wifi_nrf_wlan_fmac_add_vif(
		rpu_ctx_lnx, "nrf_wifi", base_mac_addr, NL80211_IFTYPE_STATION);

	rtnl_unlock();

	if (!vif_ctx_lnx) {
		pr_err("%s: Unable to register default interface to stack\n",
		       __func__);
		goto out;
	}

	memset(&add_vif_info, 0, sizeof(add_vif_info));

	add_vif_info.iftype = NL80211_IFTYPE_STATION;

	memcpy(add_vif_info.ifacename, "wlan0", strlen("wlan0"));

	ether_addr_copy(add_vif_info.mac_addr, base_mac_addr);

	vif_ctx_lnx->if_idx = wifi_nrf_fmac_add_vif(rpu_ctx_lnx->rpu_ctx,
						    vif_ctx_lnx, &add_vif_info);

	if (vif_ctx_lnx->if_idx != 0) {
		pr_err("%s: FMAC returned non 0 index for default interface\n",
		       __func__);
		goto out;
	}

	rpu_ctx_lnx->def_vif_ctx = vif_ctx_lnx;

#endif /* !CONFIG_NRF700X_RADIO_TEST */

	status = wifi_nrf_wlan_fmac_dbgfs_init(rpu_ctx_lnx);

	if (status != WIFI_NRF_STATUS_SUCCESS) {
		pr_err("%s: Failed to create wlan entry in DebugFS\n",
		       __func__);
		goto out;
	}

	status = wifi_nrf_fmac_ver_get(rpu_ctx_lnx->rpu_ctx, &fw_ver);

	if (status != WIFI_NRF_STATUS_SUCCESS) {
		pr_err("%s: wifi_nrf_fmac_ver_get failed\n", __func__);
		goto out;
	}

	pr_info("Firmware (v%d.%d.%d.%d) booted successfully\n",
		NRF_WIFI_UMAC_VER(fw_ver), NRF_WIFI_UMAC_VER_MAJ(fw_ver),
		NRF_WIFI_UMAC_VER_MIN(fw_ver), NRF_WIFI_UMAC_VER_EXTRA(fw_ver));

	tx_pwr_ctrl_params.ant_gain_2g = CONFIG_NRF700X_ANT_GAIN_2G;
	tx_pwr_ctrl_params.ant_gain_5g_band1 = CONFIG_NRF700X_ANT_GAIN_5G_BAND1;
	tx_pwr_ctrl_params.ant_gain_5g_band2 = CONFIG_NRF700X_ANT_GAIN_5G_BAND2;
	tx_pwr_ctrl_params.ant_gain_5g_band3 = CONFIG_NRF700X_ANT_GAIN_5G_BAND3;
	tx_pwr_ctrl_params.band_edge_2g_lo =
		CONFIG_NRF700X_BAND_2G_LOWER_EDGE_BACKOFF;
	tx_pwr_ctrl_params.band_edge_2g_hi =
		CONFIG_NRF700X_BAND_2G_UPPER_EDGE_BACKOFF;
	tx_pwr_ctrl_params.band_edge_5g_unii_1_lo =
		CONFIG_NRF700X_BAND_UNII_1_LOWER_EDGE_BACKOFF;
	tx_pwr_ctrl_params.band_edge_5g_unii_1_hi =
		CONFIG_NRF700X_BAND_UNII_1_UPPER_EDGE_BACKOFF;
	tx_pwr_ctrl_params.band_edge_5g_unii_2a_lo =
		CONFIG_NRF700X_BAND_UNII_2A_LOWER_EDGE_BACKOFF;
	tx_pwr_ctrl_params.band_edge_5g_unii_2a_hi =
		CONFIG_NRF700X_BAND_UNII_2A_UPPER_EDGE_BACKOFF;
	tx_pwr_ctrl_params.band_edge_5g_unii_2c_lo =
		CONFIG_NRF700X_BAND_UNII_2C_LOWER_EDGE_BACKOFF;
	tx_pwr_ctrl_params.band_edge_5g_unii_2c_hi =
		CONFIG_NRF700X_BAND_UNII_2C_UPPER_EDGE_BACKOFF;
	tx_pwr_ctrl_params.band_edge_5g_unii_3_lo =
		CONFIG_NRF700X_BAND_UNII_3_LOWER_EDGE_BACKOFF;
	tx_pwr_ctrl_params.band_edge_5g_unii_3_hi =
		CONFIG_NRF700X_BAND_UNII_3_UPPER_EDGE_BACKOFF;
	tx_pwr_ctrl_params.band_edge_5g_unii_4_lo =
		CONFIG_NRF700X_BAND_UNII_4_LOWER_EDGE_BACKOFF;
	tx_pwr_ctrl_params.band_edge_5g_unii_4_hi =
		CONFIG_NRF700X_BAND_UNII_4_UPPER_EDGE_BACKOFF;

	status = wifi_nrf_fmac_dev_init(rpu_ctx_lnx->rpu_ctx, NULL,
#ifdef CONFIG_WIFI_NRF_LOW_POWER
					sleep_type,
#endif /* CONFIG_NRF_WIFI_LOW_POWER */
					NRF_WIFI_DEF_PHY_CALIB, op_band,
					&tx_pwr_ctrl_params);

	if (status != WIFI_NRF_STATUS_SUCCESS) {
		pr_err("%s: wifi_nrf_fmac_dev_init failed\n", __func__);
		goto out;
	}
	status = wifi_nrf_fmac_set_vif_macaddr(
		rpu_ctx_lnx->rpu_ctx, vif_ctx_lnx->if_idx, base_mac_addr);
	if (status != WIFI_NRF_STATUS_SUCCESS) {
		pr_err("%s: MAC address change failed\n", __func__);
		goto out;
	}
out:
#ifndef CONFIG_NRF700X_RADIO_TEST
	if (status != WIFI_NRF_STATUS_SUCCESS) {
		if (vif_ctx_lnx) {
			/* Remove the default interface and unregister it from the netdev stack.
			 * We need to take a rtnl_lock since the netdev stack expects it. In the
			 * regular case this lock is taken by the cfg80211, but in the case of
			 * the default interface we need to take it since we are initiating the
			 * deletion of the interface */
			rtnl_lock();

			wifi_nrf_wlan_fmac_del_vif(vif_ctx_lnx);

			rtnl_unlock();
		}
	}
#endif /* !CONFIG_NRF700X_RADIO_TEST */

	return status;
}

void wifi_nrf_fmac_dev_deinit_lnx(struct wifi_nrf_ctx_lnx *rpu_ctx_lnx)
{
#ifndef CONFIG_NRF700X_RADIO_TEST
	struct wifi_nrf_fmac_dev_ctx *fmac_dev_ctx = NULL;
	struct wifi_nrf_fmac_dev_ctx_def *def_dev_ctx = NULL;
	struct wifi_nrf_fmac_vif_ctx_lnx *vif_ctx_lnx = NULL;
	unsigned char if_idx = MAX_NUM_VIFS;
	unsigned char i = 0;

	fmac_dev_ctx = rpu_ctx_lnx->rpu_ctx;
	def_dev_ctx = wifi_dev_priv(fmac_dev_ctx);

	/* Delete all other interfaces */
	for (i = 1; i < MAX_NUM_VIFS; i++) {
		if (def_dev_ctx->vif_ctx[i]) {
			vif_ctx_lnx =
				(struct wifi_nrf_fmac_vif_ctx_lnx
					 *)(def_dev_ctx->vif_ctx[i]->os_vif_ctx);
			if_idx = vif_ctx_lnx->if_idx;
			rtnl_lock();
			wifi_nrf_wlan_fmac_del_vif(vif_ctx_lnx);
			rtnl_unlock();
			wifi_nrf_fmac_del_vif(fmac_dev_ctx, if_idx);
			def_dev_ctx->vif_ctx[i] = NULL;
		}
	}

	/* Delete the default interface*/
	vif_ctx_lnx = rpu_ctx_lnx->def_vif_ctx;
	fmac_dev_ctx = rpu_ctx_lnx->rpu_ctx;
	if_idx = vif_ctx_lnx->if_idx;
#endif /* !CONFIG_NRF700X_RADIO_TEST */

	wifi_nrf_fmac_dev_deinit(rpu_ctx_lnx->rpu_ctx);

	wifi_nrf_wlan_fmac_dbgfs_deinit(rpu_ctx_lnx);

#ifndef CONFIG_NRF700X_RADIO_TEST
	/* Remove the default interface and unregister it from the netdev stack.
	 * We need to take a rtnl_lock since the netdev stack expects it. In the
	 * regular case this lock is taken by the cfg80211, but in the case of
	 * the default interface we need to take it since we are initiating the
	 * deletion of the interface */
	rtnl_lock();

	wifi_nrf_wlan_fmac_del_vif(vif_ctx_lnx);

	rtnl_unlock();

	wifi_nrf_fmac_del_vif(fmac_dev_ctx, if_idx);

	rpu_ctx_lnx->def_vif_ctx = NULL;
#endif /* !CONFIG_NRF700X_RADIO_TEST */
}

void wifi_nrf_chnl_get_callbk_fn(void *os_vif_ctx,
				 struct nrf_wifi_umac_event_get_channel *info,
				 unsigned int event_len)
{
	struct wifi_nrf_fmac_vif_ctx_lnx *vif_ctx_lnx = NULL;

	vif_ctx_lnx = os_vif_ctx;

	vif_ctx_lnx->chan_def =
		kmalloc(sizeof(struct nrf_wifi_chan_definition), GFP_KERNEL);
	if (!vif_ctx_lnx->chan_def) {
		pr_err("%s: Unable to allocate memory for chan_def\n",
		       __func__);
		return;
	}

	memcpy(vif_ctx_lnx->chan_def, &info->chan_def,
	       sizeof(struct nrf_wifi_chan_definition));
}

void wifi_nrf_tx_pwr_get_callbk_fn(void *os_vif_ctx,
				   struct nrf_wifi_umac_event_get_tx_power *info,
				   unsigned int event_len)
{
	struct wifi_nrf_fmac_vif_ctx_lnx *vif_ctx_lnx = NULL;

	vif_ctx_lnx = os_vif_ctx;

	vif_ctx_lnx->tx_power = info->txpwr_level;
	vif_ctx_lnx->event_tx_power = 1;
}

void wifi_nrf_get_station_callbk_fn(void *os_vif_ctx,
				    struct nrf_wifi_umac_event_new_station *info,
				    unsigned int event_len)
{
	struct wifi_nrf_fmac_vif_ctx_lnx *vif_ctx_lnx = NULL;

	vif_ctx_lnx = os_vif_ctx;

	vif_ctx_lnx->station_info =
		kmalloc(sizeof(struct nrf_wifi_sta_info), GFP_KERNEL);
	if (!vif_ctx_lnx->station_info) {
		pr_err("%s: Unable to allocate memory for station_info\n",
		       __func__);
		return;
	}

	memcpy(vif_ctx_lnx->station_info, &info->sta_info,
	       sizeof(struct nrf_wifi_sta_info));
}

void wifi_nrf_disp_scan_res_callbk_fn(
	void *os_vif_ctx,
	struct nrf_wifi_umac_event_new_scan_display_results *scan_res,
	unsigned int event_len, bool more_res)
{
}

#ifndef CONFIG_NRF700X_RADIO_TEST
void wifi_nrf_cookie_rsp_callbk_fn(
	void *os_vif_ctx, struct nrf_wifi_umac_event_cookie_rsp *cookie_rsp,
	unsigned int event_len)
{
	struct wifi_nrf_fmac_vif_ctx_lnx *vif_ctx_lnx = NULL;
	struct wifi_nrf_ctx_lnx *rpu_ctx_lnx = NULL;
	struct cookie_info *cookie_info = NULL;

	vif_ctx_lnx = os_vif_ctx;
	rpu_ctx_lnx = vif_ctx_lnx->rpu_ctx;

	cookie_info = kzalloc(sizeof(*cookie_info), GFP_KERNEL);

	if (!cookie_info) {
		pr_err("%s: Unable to allocate memory for cookie_info\n",
		       __func__);
		return;
	}

	cookie_info->host_cookie = cookie_rsp->host_cookie;
	cookie_info->rpu_cookie = cookie_rsp->cookie;

	list_add(&cookie_info->list, &rpu_ctx_lnx->cookie_list);
}
#endif /* !CONFIG_NRF700X_RADIO_TEST */

#ifdef CONFIG_NRF700X_STA_MODE
static void wifi_nrf_process_rssi_from_rx(void *os_vif_ctx, signed short signal)
{
	struct wifi_nrf_fmac_vif_ctx_lnx *vif_ctx_lnx = NULL;
	struct wifi_nrf_ctx_lnx *rpu_ctx_lnx = NULL;
	struct wifi_nrf_fmac_dev_ctx *fmac_dev_ctx = NULL;
	vif_ctx_lnx = os_vif_ctx;
	if (!vif_ctx_lnx) {
		pr_err("%s: vif_ctx_lnx is NULL\n", __func__);
		return;
	}
	rpu_ctx_lnx = vif_ctx_lnx->rpu_ctx;
	if (!rpu_ctx_lnx) {
		pr_err("%s: rpu_ctx_lnx is NULL\n", __func__);
		return;
	}
	fmac_dev_ctx = rpu_ctx_lnx->rpu_ctx;
	vif_ctx_lnx->rssi = MBM_TO_DBM(signal);
	vif_ctx_lnx->rssi_record_timestamp_us =
		wifi_nrf_osal_time_get_curr_us(fmac_dev_ctx->fpriv->opriv);
}
#endif /* CONFIG_NRF700X_STA_MODE */

void wifi_nrf_set_if_callbk_fn(
	void *os_vif_ctx,
	struct nrf_wifi_umac_event_set_interface *set_if_event,
	unsigned int event_len)
{
	struct wifi_nrf_fmac_vif_ctx_lnx *vif_ctx_lnx = NULL;

	vif_ctx_lnx = os_vif_ctx;

	vif_ctx_lnx->event_set_if = 1;
	vif_ctx_lnx->status_set_if = set_if_event->return_value;
}

void wifi_nrf_twt_config_callbk_fn(
	void *os_vif_ctx, struct nrf_wifi_umac_cmd_config_twt *twt_cfg_event,
	unsigned int event_len)
{
	struct wifi_nrf_ctx_lnx *rpu_ctx_lnx = NULL;
	struct wifi_nrf_fmac_vif_ctx_lnx *vif_ctx_lnx = NULL;

	vif_ctx_lnx = os_vif_ctx;
	rpu_ctx_lnx = vif_ctx_lnx->rpu_ctx;

	rpu_ctx_lnx->twt_params.twt_event.twt_flow_id =
		twt_cfg_event->info.twt_flow_id;
	rpu_ctx_lnx->twt_params.twt_event.neg_type =
		twt_cfg_event->info.neg_type;
	rpu_ctx_lnx->twt_params.twt_event.setup_cmd =
		twt_cfg_event->info.setup_cmd;
	rpu_ctx_lnx->twt_params.twt_event.ap_trigger_frame =
		twt_cfg_event->info.ap_trigger_frame;
	rpu_ctx_lnx->twt_params.twt_event.is_implicit =
		twt_cfg_event->info.is_implicit;
	rpu_ctx_lnx->twt_params.twt_event.twt_flow_type =
		twt_cfg_event->info.twt_flow_type;
	rpu_ctx_lnx->twt_params.twt_event.twt_target_wake_interval_exponent =
		twt_cfg_event->info.twt_target_wake_interval_exponent;
	rpu_ctx_lnx->twt_params.twt_event.twt_target_wake_interval_mantissa =
		twt_cfg_event->info.twt_target_wake_interval_mantissa;
	//TODO
	//twt_setup_cmd->info.target_wake_time =
	//    rpu_ctx_lnx->twt_params.target_wake_time;
	rpu_ctx_lnx->twt_params.twt_event.nominal_min_twt_wake_duration =
		twt_cfg_event->info.nominal_min_twt_wake_duration;
	rpu_ctx_lnx->twt_params.twt_event_info_avail = 1;
}

void wifi_nrf_twt_teardown_callbk_fn(
	void *os_vif_ctx,
	struct nrf_wifi_umac_cmd_teardown_twt *twt_teardown_event,
	unsigned int event_len)
{
	struct wifi_nrf_ctx_lnx *rpu_ctx_lnx = NULL;
	struct wifi_nrf_fmac_vif_ctx_lnx *vif_ctx_lnx = NULL;

	vif_ctx_lnx = os_vif_ctx;
	rpu_ctx_lnx = vif_ctx_lnx->rpu_ctx;

	rpu_ctx_lnx->twt_params.twt_event.twt_flow_id =
		twt_teardown_event->info.twt_flow_id;

	rpu_ctx_lnx->twt_params.teardown_reason =
		twt_teardown_event->info.reason_code;
}

#ifdef CONFIG_WIFI_NRF_LOW_POWER
void wifi_nrf_twt_sleep_callbk_fn(
	void *os_vif_ctx, struct nrf_wifi_umac_event_twt_sleep *twt_sleep_evnt,
	unsigned int event_len)
{
	struct wifi_nrf_fmac_vif_ctx_lnx *vif_ctx_lnx = NULL;

	vif_ctx_lnx = os_vif_ctx;

	switch (twt_sleep_evnt->info.type) {
	case TWT_BLOCK_TX:
		netif_stop_queue(vif_ctx_lnx->netdev);
		break;
	case TWT_UNBLOCK_TX:
		netif_wake_queue(vif_ctx_lnx->netdev);
		break;
	default:
		break;
	}
}
#endif /* CONFIG_NRF_WIFI_LOW_POWER */

#ifndef CONFIG_NRF700X_RADIO_TEST
#endif /* !CONFIG_NRF700X_RADIO_TEST */

int __init wifi_nrf_init_lnx(void)
{
	int ret = -ENOMEM;
#ifndef CONFIG_NRF700X_RADIO_TEST
	struct wifi_nrf_fmac_callbk_fns callbk_fns;
	struct nrf_wifi_data_config_params data_config;
	struct rx_buf_pool_params rx_buf_pools[MAX_NUM_OF_RX_QUEUES];
#endif /* !CONFIG_NRF700X_RADIO_TEST */

	ret = wifi_nrf_dbgfs_init();

	if (ret) {
		pr_err("%s: Failed to create root entry in DebugFS\n",
		       __func__);
		goto out;
	}

#ifndef CONFIG_NRF700X_RADIO_TEST
	if (rf_params) {
		if (strlen(rf_params) != (NRF_WIFI_RF_PARAMS_SIZE * 2)) {
			pr_err("%s: Invalid length of rf_params. Should consist of %d hex characters\n",
			       __func__, (NRF_WIFI_RF_PARAMS_SIZE * 2));

			goto out;
		}
	}

	data_config.aggregation = aggregation;
	data_config.wmm = wmm;
	data_config.max_num_tx_agg_sessions = max_num_tx_agg_sessions;
	data_config.max_num_rx_agg_sessions = max_num_rx_agg_sessions;
	data_config.max_tx_aggregation = max_tx_aggregation;
	data_config.reorder_buf_size = reorder_buf_size;
	data_config.max_rxampdu_size = max_rxampdu_size;
	data_config.rate_protection_type = rate_protection_type;

	rx_buf_pools[0].num_bufs = rx1_num_bufs;
	rx_buf_pools[1].num_bufs = rx2_num_bufs;
	rx_buf_pools[2].num_bufs = rx3_num_bufs;

	rx_buf_pools[0].buf_sz = rx1_buf_sz;
	rx_buf_pools[1].buf_sz = rx2_buf_sz;
	rx_buf_pools[2].buf_sz = rx3_buf_sz;
#endif /* !CONFIG_NRF700X_RADIO_TEST */

	rpu_drv_priv.drv_init = false;

#ifndef CONFIG_NRF700X_RADIO_TEST
	memset(&callbk_fns, 0, sizeof(callbk_fns));

	callbk_fns.if_carr_state_chg_callbk_fn =
		&wifi_nrf_netdev_if_state_chg_callbk_fn;
	callbk_fns.rx_frm_callbk_fn = &wifi_nrf_netdev_frame_rx_callbk_fn;
	callbk_fns.get_station_callbk_fn = &wifi_nrf_get_station_callbk_fn;
	callbk_fns.disp_scan_res_callbk_fn = &wifi_nrf_disp_scan_res_callbk_fn;
#ifdef CONFIG_WIFI_MGMT_RAW_SCAN_RESULTS
	callbk_fns.rx_bcn_prb_resp_callbk_fn =
		&nrf_wifi_cfg80211_rx_bcn_prb_rsp_callbk_fn;
#endif /* CONFIG_WIFI_MGMT_RAW_SCAN_RESULTS */
	callbk_fns.set_if_callbk_fn = &wifi_nrf_set_if_callbk_fn;
	callbk_fns.chnl_get_callbk_fn = &wifi_nrf_chnl_get_callbk_fn;
	callbk_fns.tx_pwr_get_callbk_fn = &wifi_nrf_tx_pwr_get_callbk_fn;
	callbk_fns.cookie_rsp_callbk_fn = &wifi_nrf_cookie_rsp_callbk_fn;
	callbk_fns.process_rssi_from_rx = &wifi_nrf_process_rssi_from_rx;
	callbk_fns.scan_start_callbk_fn =
		&wifi_nrf_cfg80211_scan_start_callbk_fn;
	callbk_fns.scan_res_callbk_fn = &wifi_nrf_cfg80211_scan_res_callbk_fn;
	callbk_fns.scan_done_callbk_fn = &wifi_nrf_cfg80211_scan_done_callbk_fn;
	callbk_fns.scan_abort_callbk_fn =
		&wifi_nrf_cfg80211_scan_abort_callbk_fn;
	callbk_fns.auth_resp_callbk_fn = &wifi_nrf_cfg80211_auth_resp_callbk_fn;
	callbk_fns.assoc_resp_callbk_fn =
		&wifi_nrf_cfg80211_assoc_resp_callbk_fn;
	callbk_fns.deauth_callbk_fn = &wifi_nrf_cfg80211_deauth_callbk_fn;
	callbk_fns.disassoc_callbk_fn = &wifi_nrf_cfg80211_disassoc_callbk_fn;
	callbk_fns.mgmt_rx_callbk_fn = &wifi_nrf_cfg80211_mgmt_rx_callbk_fn;
	callbk_fns.unprot_mlme_mgmt_rx_callbk_fn =
		&wifi_nrf_cfg80211_unprot_mlme_mgmt_rx_callbk_fn;
	callbk_fns.roc_callbk_fn = &wifi_nrf_cfg80211_roc_callbk_fn;
	callbk_fns.roc_cancel_callbk_fn =
		&wifi_nrf_cfg80211_roc_cancel_callbk_fn;
	callbk_fns.tx_status_callbk_fn = &wifi_nrf_cfg80211_tx_status_callbk_fn;

	callbk_fns.twt_config_callbk_fn = &wifi_nrf_twt_config_callbk_fn;
	callbk_fns.twt_teardown_callbk_fn = &wifi_nrf_twt_teardown_callbk_fn;
#ifdef CONFIG_WIFI_NRF_LOW_POWER
	callbk_fns.twt_sleep_callbk_fn = &wifi_nrf_twt_sleep_callbk_fn;
#endif
#endif /* !CONFIG_NRF700X_RADIO_TEST */

	rpu_drv_priv.fmac_priv = wifi_nrf_fmac_init(
#ifndef CONFIG_NRF700X_RADIO_TEST
		&data_config, rx_buf_pools, &callbk_fns
#endif /* !CONFIG_NRF700X_RADIO_TEST */
	);

	if (rpu_drv_priv.fmac_priv == NULL) {
		pr_err("%s: wifi_nrf_fmac_init failed\n", __func__);
		goto out;
	}

	rpu_drv_priv.drv_init = true;
#ifndef CONFIG_NRF700X_RADIO_TEST
#endif /* !CONFIG_NRF700X_RADIO_TEST */

	ret = 0;
out:
	return ret;
}

void __exit wifi_nrf_deinit_lnx(void)
{
#ifndef CONFIG_NRF700X_RADIO_TEST
#endif /* !CONFIG_NRF700X_RADIO_TEST */
	wifi_nrf_fmac_deinit(rpu_drv_priv.fmac_priv);
	wifi_nrf_dbgfs_deinit();
}

MODULE_LICENSE("GPL");
MODULE_AUTHOR("Imagination Technologies");
MODULE_DESCRIPTION("FullMAC Driver for nRF WLAN Solution");
MODULE_VERSION(NRF_WIFI_FMAC_DRV_VER);

module_init(wifi_nrf_init_lnx);
module_exit(wifi_nrf_deinit_lnx);
