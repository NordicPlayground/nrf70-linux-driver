/*
 * Copyright (c) 2023 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: GPL-2.0
 */

#ifndef __CFG80211_IF_H__
#define __CFG80211_IF_H__

#include <net/cfg80211.h>
#include "main.h"
#include "fmac_main.h"

struct wiphy *cfg80211_if_init(void);
void cfg80211_if_deinit(struct wiphy *wiphy);

void nrf_wifi_cfg80211_scan_start_callbk_fn(
	void *os_vif_ctx,
	struct nrf_wifi_umac_event_trigger_scan *scan_start_event,
	unsigned int event_len);
void nrf_wifi_cfg80211_scan_res_callbk_fn(
	void *os_vif_ctx, struct nrf_wifi_umac_event_new_scan_results *scan_res,
	unsigned int event_len, bool more_res);
void nrf_wifi_cfg80211_scan_done_callbk_fn(
	void *os_vif_ctx,
	struct nrf_wifi_umac_event_trigger_scan *scan_done_event,
	unsigned int event_len);
void nrf_wifi_cfg80211_scan_abort_callbk_fn(
	void *os_vif_ctx,
	struct nrf_wifi_umac_event_trigger_scan *scan_done_event,
	unsigned int event_len);
void nrf_wifi_cfg80211_auth_resp_callbk_fn(
	void *os_vif_ctx, struct nrf_wifi_umac_event_mlme *auth_resp_event,
	unsigned int event_len);
void nrf_wifi_cfg80211_assoc_resp_callbk_fn(
	void *os_vif_ctx, struct nrf_wifi_umac_event_mlme *assoc_resp_event,
	unsigned int event_len);
void nrf_wifi_cfg80211_deauth_callbk_fn(
	void *os_vif_ctx, struct nrf_wifi_umac_event_mlme *deauth_event,
	unsigned int event_len);
void nrf_wifi_cfg80211_disassoc_callbk_fn(
	void *os_vif_ctx, struct nrf_wifi_umac_event_mlme *disassoc_event,
	unsigned int event_len);
void nrf_wifi_cfg80211_mgmt_rx_callbk_fn(
	void *os_vif_ctx, struct nrf_wifi_umac_event_mlme *mgmt_rx_event,
	unsigned int event_len);
void nrf_wifi_cfg80211_tx_status_callbk_fn(
	void *os_vif_ctx, struct nrf_wifi_umac_event_mlme *mgmt_rx_event,
	unsigned int event_len);
void nrf_wifi_cfg80211_unprot_mlme_mgmt_rx_callbk_fn(
	void *os_vif_ctx, struct nrf_wifi_umac_event_mlme *unprot_mlme_event,
	unsigned int event_len);
void nrf_wifi_cfg80211_roc_callbk_fn(
	void *os_vif_ctx, struct nrf_wifi_event_remain_on_channel *roc_event,
	unsigned int event_len);
void nrf_wifi_cfg80211_roc_cancel_callbk_fn(
	void *os_vif_ctx,
	struct nrf_wifi_event_remain_on_channel *roc_cancel_event,
	unsigned int event_len);
void nrf_wifi_cfg80211_rx_bcn_prb_rsp_callbk_fn(void *os_vif_ctx, void *frm,
						unsigned short frequency,
						short signal);
#endif /* __CFG80211_IF_H__ */
