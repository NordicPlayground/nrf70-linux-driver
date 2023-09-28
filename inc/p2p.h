/*
 * Copyright (c) 2023 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: GPL-2.0
 */

/*
 * @dev_addr: P2P device address.
 * @int_addr: P2P interface address.
 * @remain_on_channel: contains copy of struct used by cfg80211.
 * @remain_on_channel_cookie: cookie counter for remain on channel cmd
 */
struct p2p_info {
	u8 dev_addr[ETH_ALEN];
	u8 int_addr[ETH_ALEN];
	struct ieee80211_channel remain_on_channel;
	u64 remain_on_channel_cookie;
};

int nrf_wifi_cfg80211_start_p2p_dev(struct wiphy *wiphy,
				    struct wireless_dev *wdev);

void nrf_wifi_cfg80211_stop_p2p_dev(struct wiphy *wiphy,
				    struct wireless_dev *wdev);

int nrf_wifi_cfg80211_remain_on_channel(struct wiphy *wiphy,
					struct wireless_dev *wdev,
					struct ieee80211_channel *chan,
					unsigned int duration,
					unsigned long long *cookie);

int nrf_wifi_cfg80211_cancel_remain_on_channel(struct wiphy *wiphy,
					       struct wireless_dev *wdev,
					       unsigned long long cookie);

int p2p_event(struct nrf_wifi_ctx_lnx *rpu_ctx_lnx, void *network_dev,
	      void *params);
