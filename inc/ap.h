#ifndef CONFIG_NRF700X_RADIO_TEST
/*
 * Copyright (c) 2023 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: GPL-2.0
 */

struct nrf_wifi_cfg80211_mgmt_registration {
	struct list_head list;
	struct wireless_dev *wdev;
	u32 nlportid;
	int match_len;
	__le16 frame_type;
	u8 match[];
};

struct cookie_info {
	struct list_head list;
	unsigned long long host_cookie;
	unsigned long long rpu_cookie;
};

int ap_event_cookie(struct nrf_wifi_ctx_lnx *rpu_ctx_lnx, int event_num,
		    void *event_info);

int ap_event_rx_mgmt(void *dev, int event_num, void *event_info);

int ap_event_tx_status(struct nrf_wifi_ctx_lnx *rpu_ctx_lnx, void *dev,
		       int event_num, void *event_info);

int ap_event_set_interface(void *dev, int event_num, void *event_info);

int nrf_wifi_cfg80211_chg_bcn(struct wiphy *wiphy, struct net_device *netdev,
			      struct cfg80211_beacon_data *params);

int nrf_wifi_cfg80211_chg_bss(struct wiphy *wiphy, struct net_device *netdev,
			      struct bss_parameters *params);

int nrf_wifi_cfg80211_start_ap(struct wiphy *wiphy, struct net_device *netdev,
			       struct cfg80211_ap_settings *params);

int nrf_wifi_cfg80211_stop_ap(struct wiphy *wiphy, struct net_device *netdev);

int nrf_wifi_cfg80211_chg_vif(struct wiphy *wiphy, struct net_device *netdev,
			      enum nl80211_iftype type,
			      struct vif_params *params);

int nrf_wifi_cfg80211_mgmt_tx(struct wiphy *wiphy, struct wireless_dev *wdev,
			      struct cfg80211_mgmt_tx_params *params,
			      u64 *cookie);

int nrf_wifi_cfg80211_del_sta(struct wiphy *wiphy, struct net_device *netdev,
			      struct station_del_parameters *params);

int nrf_wifi_cfg80211_add_sta(struct wiphy *wiphy, struct net_device *netdev,
			      const u8 *mac, struct station_parameters *params);

int nrf_wifi_cfg80211_chg_sta(struct wiphy *wiphy, struct net_device *netdev,
			      const u8 *mac, struct station_parameters *params);

int nrf_wifi_cfg80211_set_txq_params(struct wiphy *wiphy,
				     struct net_device *netdev,
				     struct ieee80211_txq_params *params);

void nrf_wifi_cfg80211_mgmt_frame_reg(struct wiphy *wiphy,
				      struct wireless_dev *wdev,
				      struct mgmt_frame_regs *upd);

int nrf_wifi_cfg80211_probe_client(struct wiphy *wiphy,
				   struct net_device *netdev, const u8 *peer,
				   u64 *cookie);
struct wireless_dev *nrf_wifi_cfg80211_add_vif(struct wiphy *wiphy,
					       const char *name,
					       unsigned char name_assign_type,
					       enum nl80211_iftype type,
					       struct vif_params *params);

int nrf_wifi_cfg80211_del_vif(struct wiphy *wiphy, struct wireless_dev *wdev);
#endif /* !CONFIG_NRF700X_RADIO_TEST */
