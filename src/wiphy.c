/*
 * Copyright (c) 2023 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: GPL-2.0
 */

#include <net/cfg80211.h>

#define CHAN5G(_freq, _idx, _flags)                                     \
	{                                                               \
		.band = NL80211_BAND_5GHZ, .center_freq = (_freq),      \
		.hw_value = (_idx), .max_power = 20, .flags = (_flags), \
	}

#define CHAN2G(_freq, _idx)                                        \
	{                                                          \
		.band = NL80211_BAND_2GHZ, .center_freq = (_freq), \
		.hw_value = (_idx), .max_power = 20,               \
	}

/* There isn't a lot of sense in it, but you can transmit anything you like */
const struct ieee80211_txrx_stypes
ieee80211_default_mgmt_stypes[NUM_NL80211_IFTYPES] = {
	[NL80211_IFTYPE_ADHOC] = {
		.tx = 0xffff,
		.rx = BIT(IEEE80211_STYPE_ACTION >> 4) |
			BIT(IEEE80211_STYPE_AUTH >> 4) |
			BIT(IEEE80211_STYPE_DEAUTH >> 4) |
			BIT(IEEE80211_STYPE_PROBE_REQ >> 4),
	},
	[NL80211_IFTYPE_STATION] = {
		.tx = 0xffff,
		.rx = BIT(IEEE80211_STYPE_ACTION >> 4) |
			BIT(IEEE80211_STYPE_PROBE_REQ >> 4),
	},
	[NL80211_IFTYPE_AP] = {
		.tx = 0xffff,
		.rx = BIT(IEEE80211_STYPE_ASSOC_REQ >> 4) |
			BIT(IEEE80211_STYPE_REASSOC_REQ >> 4) |
			BIT(IEEE80211_STYPE_PROBE_REQ >> 4) |
			BIT(IEEE80211_STYPE_DISASSOC >> 4) |
			BIT(IEEE80211_STYPE_AUTH >> 4) |
			BIT(IEEE80211_STYPE_DEAUTH >> 4) |
			BIT(IEEE80211_STYPE_ACTION >> 4),
	},
	[NL80211_IFTYPE_AP_VLAN] = {
		/* copy AP */
		.tx = 0xffff,
		.rx = BIT(IEEE80211_STYPE_ASSOC_REQ >> 4) |
			BIT(IEEE80211_STYPE_REASSOC_REQ >> 4) |
			BIT(IEEE80211_STYPE_PROBE_REQ >> 4) |
			BIT(IEEE80211_STYPE_DISASSOC >> 4) |
			BIT(IEEE80211_STYPE_AUTH >> 4) |
			BIT(IEEE80211_STYPE_DEAUTH >> 4) |
			BIT(IEEE80211_STYPE_ACTION >> 4),
	},
	[NL80211_IFTYPE_P2P_CLIENT] = {
		.tx = 0xffff,
		.rx = BIT(IEEE80211_STYPE_ACTION >> 4) |
			BIT(IEEE80211_STYPE_PROBE_REQ >> 4),
	},
	[NL80211_IFTYPE_P2P_GO] = {
		.tx = 0xffff,
		.rx = BIT(IEEE80211_STYPE_ASSOC_REQ >> 4) |
			BIT(IEEE80211_STYPE_REASSOC_REQ >> 4) |
			BIT(IEEE80211_STYPE_PROBE_REQ >> 4) |
			BIT(IEEE80211_STYPE_DISASSOC >> 4) |
			BIT(IEEE80211_STYPE_AUTH >> 4) |
			BIT(IEEE80211_STYPE_DEAUTH >> 4) |
			BIT(IEEE80211_STYPE_ACTION >> 4),
	},
	[NL80211_IFTYPE_MESH_POINT] = {
		.tx = 0xffff,
		.rx = BIT(IEEE80211_STYPE_ACTION >> 4) |
			BIT(IEEE80211_STYPE_AUTH >> 4) |
			BIT(IEEE80211_STYPE_DEAUTH >> 4),
	},
	[NL80211_IFTYPE_P2P_DEVICE] = {
		.tx = 0xffff,
		.rx = BIT(IEEE80211_STYPE_ACTION >> 4) |
			BIT(IEEE80211_STYPE_PROBE_REQ >> 4),
	},
};

struct ieee80211_channel ofdm_chantable[] = {
	CHAN5G(5180, 14, 0), /* Channel 36 */
	CHAN5G(5200, 15, 0), /* Channel 40 */
	CHAN5G(5220, 16, 0), /* Channel 44 */
	CHAN5G(5240, 17, 0), /* Channel 48 */
	CHAN5G(5260, 18, IEEE80211_CHAN_RADAR), /* Channel 52 */
	CHAN5G(5280, 19, IEEE80211_CHAN_RADAR), /* Channel 56 */
	CHAN5G(5300, 20, IEEE80211_CHAN_RADAR), /* Channel 60 */
	CHAN5G(5320, 21, IEEE80211_CHAN_RADAR), /* Channel 64 */
	CHAN5G(5500, 22, IEEE80211_CHAN_RADAR), /* Channel 100 */
	CHAN5G(5520, 23, IEEE80211_CHAN_RADAR), /* Channel 104 */
	CHAN5G(5540, 24, IEEE80211_CHAN_RADAR), /* Channel 108 */
	CHAN5G(5560, 25, IEEE80211_CHAN_RADAR), /* Channel 112 */
	CHAN5G(5580, 26, IEEE80211_CHAN_RADAR), /* Channel 116 */
	CHAN5G(5600, 27, IEEE80211_CHAN_RADAR), /* Channel 120 */
	CHAN5G(5620, 28, IEEE80211_CHAN_RADAR), /* Channel 124 */
	CHAN5G(5640, 29, IEEE80211_CHAN_RADAR), /* Channel 128 */
	CHAN5G(5660, 30, IEEE80211_CHAN_RADAR), /* Channel 132 */
	CHAN5G(5680, 31, IEEE80211_CHAN_RADAR), /* Channel 136 */
	CHAN5G(5700, 32, IEEE80211_CHAN_RADAR), /* Channel 140 */
	CHAN5G(5720, 33, IEEE80211_CHAN_RADAR), /* Channel 144 */
	CHAN5G(5745, 34, 0), /* Channel 149 */
	CHAN5G(5765, 35, 0), /* Channel 153 */
	CHAN5G(5785, 36, 0), /* Channel 157 */
	CHAN5G(5805, 37, 0), /* Channel 161 */
	CHAN5G(5825, 38, 0), /* Channel 165 */
	CHAN5G(5845, 39, 0), /* Channel 169 */
	CHAN5G(5865, 40, 0), /* Channel 173 */
	CHAN5G(5885, 41, 0), /* Channel 177 */
};

struct ieee80211_channel dsss_chantable[] = {
	CHAN2G(2412, 0), /* Channel 1 */
	CHAN2G(2417, 1), /* Channel 2 */
	CHAN2G(2422, 2), /* Channel 3 */
	CHAN2G(2427, 3), /* Channel 4 */
	CHAN2G(2432, 4), /* Channel 5 */
	CHAN2G(2437, 5), /* Channel 6 */
	CHAN2G(2442, 6), /* Channel 7 */
	CHAN2G(2447, 7), /* Channel 8 */
	CHAN2G(2452, 8), /* Channel 9 */
	CHAN2G(2457, 9), /* Channel 10 */
	CHAN2G(2462, 10), /* Channel 11 */
	CHAN2G(2467, 11), /* Channel 12 */
	CHAN2G(2472, 12), /* Channel 13 */
	CHAN2G(2484, 13), /* Channel 14 */
};

struct ieee80211_rate dsss_rates[] = {
	{ .bitrate = 10, .hw_value = 2 },
	{ .bitrate = 20, .hw_value = 4, .flags = IEEE80211_RATE_SHORT_PREAMBLE },
	{ .bitrate = 55,
	  .hw_value = 11,
	  .flags = IEEE80211_RATE_SHORT_PREAMBLE },
	{ .bitrate = 110,
	  .hw_value = 22,
	  .flags = IEEE80211_RATE_SHORT_PREAMBLE },
	{ .bitrate = 60, .hw_value = 12 },
	{ .bitrate = 90, .hw_value = 18 },
	{ .bitrate = 120, .hw_value = 24 },
	{ .bitrate = 180, .hw_value = 36 },
	{ .bitrate = 240, .hw_value = 48 },
	{ .bitrate = 360, .hw_value = 72 },
	{ .bitrate = 480, .hw_value = 96 },
	{ .bitrate = 540, .hw_value = 108 }
};

static struct ieee80211_rate ofdm_rates[] = {
	{ .bitrate = 60, .hw_value = 12 },  { .bitrate = 90, .hw_value = 18 },
	{ .bitrate = 120, .hw_value = 24 }, { .bitrate = 180, .hw_value = 36 },
	{ .bitrate = 240, .hw_value = 48 }, { .bitrate = 360, .hw_value = 72 },
	{ .bitrate = 480, .hw_value = 96 }, { .bitrate = 540, .hw_value = 108 }
};

struct ieee80211_supported_band band_5ghz = {
	.channels = ofdm_chantable,
	.n_channels = ARRAY_SIZE(ofdm_chantable),
	.band = NL80211_BAND_5GHZ,
	.bitrates = ofdm_rates,
	.n_bitrates = ARRAY_SIZE(ofdm_rates),
};
struct ieee80211_supported_band band_2ghz = {
	.channels = dsss_chantable,
	.n_channels = ARRAY_SIZE(dsss_chantable),
	.band = NL80211_BAND_2GHZ,
	.bitrates = dsss_rates,
	.n_bitrates = ARRAY_SIZE(dsss_rates),
	.ht_cap.ht_supported = 1,
	/*.vht_cap.vht_supported = 1,*/
};
