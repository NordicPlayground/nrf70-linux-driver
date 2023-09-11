/*
 * Copyright (c) 2023 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: GPL-2.0
 */

#ifndef __MAIN_H__
#define __MAIN_H__

#include <linux/debugfs.h>
#include "rpu_if.h"
#ifdef DEBUG_MODE_SUPPORT
#include "host_rpu_umac_if.h"
#endif

#include <net/cfg80211.h>
#include "fmac_structs.h"

#define NRF_WIFI_FMAC_DRV_VER "1.0.0.0"

enum connect_status {
	DISCONNECTED,
	CONNECTED,
};

struct twt_params {
	unsigned char twt_flow_id;
	unsigned char neg_type;
	int setup_cmd;
	unsigned char ap_trigger_frame;
	unsigned char is_implicit;
	unsigned char twt_flow_type;
	unsigned char twt_target_wake_interval_exponent;
	unsigned short twt_target_wake_interval_mantissa;
	unsigned long long target_wake_time;
	unsigned short nominal_min_twt_wake_duration;
};

struct rpu_twt_params {
	//unsigned char target_wake_time[8]; //should it be changed to unsigned long long
	enum connect_status status;
	struct twt_params twt_cmd;
	struct twt_params twt_event;
	int twt_event_info_avail;
	char twt_setup_cmd[250];
	int teardown_reason;
	int teardown_event_cnt;
};

struct wifi_nrf_ctx_lnx {
	void *rpu_ctx;
	struct wifi_nrf_fmac_vif_ctx_lnx *def_vif_ctx;
	struct wiphy *wiphy;

	struct dentry *dbgfs_rpu_root;
	struct dentry *dbgfs_wlan_root;
	struct dentry *dbgfs_wlan_stats_root;
#ifdef DEBUG_MODE_SUPPORT
	struct wifi_nrf_umac_set_beacon_info info;
	struct rpu_btcoex btcoex;
#endif
	struct list_head cookie_list;
	struct dentry *dbgfs_wifi_nrf_twt_root;
	struct rpu_twt_params twt_params;
};

struct wifi_nrf_drv_priv_lnx {
	struct dentry *dbgfs_root;
	struct dentry *dbgfs_ver_root;
	struct wifi_nrf_fmac_priv *fmac_priv;
	bool drv_init;
};

struct wifi_nrf_ctx_lnx *wifi_nrf_fmac_dev_add_lnx(void);
void wifi_nrf_fmac_dev_rem_lnx(struct wifi_nrf_ctx_lnx *rpu_ctx_lnx);
enum wifi_nrf_status
wifi_nrf_fmac_dev_init_lnx(struct wifi_nrf_ctx_lnx *rpu_ctx_lnx);
void wifi_nrf_fmac_dev_deinit_lnx(struct wifi_nrf_ctx_lnx *rpu_ctx_lnx);
#endif /* __MAIN_H__ */
