/*
 * Copyright (c) 2023 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: GPL-2.0
 */

#ifndef __FMAC_MAIN_H__
#define __FMAC_MAIN_H__

#include <net/cfg80211.h>
#include "fmac_structs.h"
#include "sta.h"
#include "ap.h"
#include "p2p.h"
#include "host_rpu_umac_if.h"
#ifdef RPU_MODE_EXPLORER
#include "driver_linux.h"
#endif /* RPU_MODE_EXPLORER */

struct wifi_nrf_fmac_vif_ctx_lnx {
	struct wifi_nrf_ctx_lnx *rpu_ctx;
	struct net_device *netdev;
	struct wireless_dev *wdev;
	struct cfg80211_bss *bss;
	struct cfg80211_scan_request *wifi_nrf_scan_req;

	unsigned char if_idx;

	/* event responses */
	struct nrf_wifi_sta_info *station_info;
	struct nrf_wifi_chan_definition *chan_def;
	int tx_power;
	int event_tx_power;
	int event_set_if;
	int status_set_if;
	struct p2p_info p2p;
	unsigned long rssi_record_timestamp_us;
	signed short rssi;
#ifdef CONFIG_NRF700X_DATA_TX
	void *data_txq;
	struct work_struct ws_data_tx;
	struct work_struct ws_queue_monitor;
	unsigned long long num_tx_pkt;
#endif
};

struct wifi_nrf_fmac_vif_ctx_lnx *
wifi_nrf_wlan_fmac_add_vif(struct wifi_nrf_ctx_lnx *rpu_ctx_lnx,
			   const char *name, char *mac_addr,
			   enum nl80211_iftype if_type);
void wifi_nrf_wlan_fmac_del_vif(struct wifi_nrf_fmac_vif_ctx_lnx *vif_ctx_lnx);
#endif /* __FMAC_MAIN_H__ */
