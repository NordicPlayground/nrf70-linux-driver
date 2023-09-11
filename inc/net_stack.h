/*
 * Copyright (c) 2023 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: GPL-2.0
 */

#ifndef __NET_STACK_H__
#define __NET_STACK_H__

#include "main.h"
#include "fmac_main.h"

#ifndef CONFIG_NRF700X_RADIO_TEST

struct wifi_nrf_fmac_vif_ctx_lnx *
wifi_nrf_netdev_add_vif(struct wifi_nrf_ctx_lnx *rpu_ctx_lnx,
			const char *if_name, struct wireless_dev *wdev,
			char *mac_addr);

void wifi_nrf_netdev_del_vif(struct net_device *netdev);

void wifi_nrf_netdev_frame_rx_callbk_fn(void *vif_ctx, void *frm);

enum wifi_nrf_status wifi_nrf_netdev_if_state_chg_callbk_fn(
	void *vif_ctx, enum wifi_nrf_fmac_if_carr_state if_state);
#endif /* !CONFIG_NRF700X_RADIO_TEST */
#endif /* __NET_STACK_H__ */
