#ifndef CONFIG_NRF700X_RADIO_TEST
/*
 * Copyright (c) 2023 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: GPL-2.0
 */

int nrf_wifi_cfg80211_suspend(struct wiphy *wiphy, struct cfg80211_wowlan *wow);

int nrf_wifi_cfg80211_resume(struct wiphy *wiphy);

void nrf_wifi_cfg80211_set_wakeup(struct wiphy *wiphy, bool enabled);

int nrf_wifi_cfg80211_set_qos_map(struct wiphy *wiphy, struct net_device *dev,
				  struct cfg80211_qos_map *qos_map);

int nrf_wifi_cfg80211_set_power_mgmt(struct wiphy *wiphy,
				     struct net_device *dev, bool enabled,
				     int timeout);

int nrf_wifi_cfg80211_scan(struct wiphy *wiphy,
			   struct cfg80211_scan_request *req);

int nrf_wifi_cfg80211_auth(struct wiphy *wiphy, struct net_device *netdev,
			   struct cfg80211_auth_request *req);

int nrf_wifi_cfg80211_assoc(struct wiphy *wiphy, struct net_device *dev,
			    struct cfg80211_assoc_request *req);

int nrf_wifi_cfg80211_deauth(struct wiphy *wiphy, struct net_device *dev,
			     struct cfg80211_deauth_request *req);

int nrf_wifi_cfg80211_disassoc(struct wiphy *wiphy, struct net_device *dev,
			       struct cfg80211_disassoc_request *req);

int nrf_wifi_cfg80211_add_key(struct wiphy *wiphy, struct net_device *netdev,
			      u8 key_index, bool pairwise, const u8 *mac_addr,
			      struct key_params *params);

int nrf_wifi_cfg80211_del_key(struct wiphy *wiphy, struct net_device *dev,
			      u8 key_idx, bool pairwise, const u8 *mac_addr);

int nrf_wifi_cfg80211_set_def_key(struct wiphy *wiphy,
				  struct net_device *netdev, u8 key_index,
				  bool unicast, bool multicast);

int nrf_wifi_cfg80211_set_def_mgmt_key(struct wiphy *wiphy,
				       struct net_device *netdev, u8 key_index);

int nrf_wifi_cfg80211_get_tx_power(struct wiphy *wiphy,
				   struct wireless_dev *wdev, int *dbm);

int nrf_wifi_cfg80211_get_station(struct wiphy *wiphy, struct net_device *dev,
				  const u8 *mac, struct station_info *sinfo);

int nrf_wifi_cfg80211_get_channel(struct wiphy *wiphy,
				  struct wireless_dev *wdev,
				  struct cfg80211_chan_def *chandef);

int nrf_wifi_cfg80211_set_wiphy_params(struct wiphy *wiphy,
				       unsigned int changed);
#endif /* !CONFIG_NRF700X_RADIO_TEST */
