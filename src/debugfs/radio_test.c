/*
 * Copyright (c) 2023 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: GPL-2.0
 */

#include <linux/debugfs.h>
#include <linux/fs.h>
#include <linux/etherdevice.h>

#include "fmac_api.h"
#include "fmac_dbgfs_if.h"

#define MAX_CONF_BUF_SIZE 500
#define MAX_ERR_STR_SIZE 80

struct nrf_wifi_ctx_lnx *ctx;

static __always_inline unsigned char
param_get_val(unsigned char *buf, unsigned char *str, unsigned long *val)
{
	unsigned char *temp = NULL;

	if (strstr(buf, str)) {
		temp = strstr(buf, "=") + 1;

		if (!kstrtoul(temp, 0, val)) {
			return 1;
		} else {
			return 0;
		}
	} else {
		return 0;
	}
}

static __always_inline unsigned char
param_get_sval(unsigned char *buf, unsigned char *str, long *sval)
{
	unsigned char *temp = NULL;

	if (strstr(buf, str)) {
		temp = strstr(buf, "=") + 1;

		if (!kstrtol(temp, 0, sval)) {
			return 1;
		} else {
			return 0;
		}
	} else {
		return 0;
	}
}

static __always_inline unsigned char param_get_match(unsigned char *buf,
						     unsigned char *str)
{
	if (strstr(buf, str))
		return 1;
	else
		return 0;
}

static int nrf_wifi_wlan_fmac_conf_disp(struct seq_file *m, void *v)
{
	struct rpu_conf_params *conf_params = NULL;

	conf_params = &ctx->conf_params;

	seq_puts(m, "************* Configured Parameters ***********\n");

#ifndef CONFIG_NRF700X_RADIO_TEST
	seq_printf(m, "uapsd_queue = %d\n", conf_params->uapsd_queue);
	seq_printf(m, "power_save = %s\n",
		   conf_params->power_save ? "ON" : "OFF");
	seq_printf(m, "he_gi = %d\n", conf_params->he_gi);
	seq_printf(m, "rts_threshold = %d\n", conf_params->rts_threshold);
#else
	seq_printf(m, "phy_calib_rxdc = %d\n",
		   (conf_params->phy_calib & NRF_WIFI_PHY_CALIB_FLAG_RXDC) ? 1 :
									     0);
	seq_printf(m, "phy_calib_txdc = %d\n",
		   (conf_params->phy_calib & NRF_WIFI_PHY_CALIB_FLAG_TXDC) ? 1 :
									     0);
	seq_printf(m, "phy_calib_txpow = %d\n",
		   (conf_params->phy_calib & NRF_WIFI_PHY_CALIB_FLAG_TXPOW) ?
			   1 :
			   0);
	seq_printf(m, "phy_calib_rxiq = %d\n",
		   (conf_params->phy_calib & NRF_WIFI_PHY_CALIB_FLAG_RXIQ) ? 1 :
									     0);
	seq_printf(m, "phy_calib_txiq = %d\n",
		   (conf_params->phy_calib & NRF_WIFI_PHY_CALIB_FLAG_TXIQ) ? 1 :
									     0);

	seq_printf(m, "he_ltf = %d\n", conf_params->he_ltf);
	seq_printf(m, "he_gi = %d\n", conf_params->he_gi);
	seq_printf(m, "tx_pkt_tput_mode = %d\n", conf_params->tx_pkt_tput_mode);
	seq_printf(m, "tx_pkt_sgi = %d\n", conf_params->tx_pkt_sgi);
	seq_printf(m, "tx_pkt_preamble = %d\n", conf_params->tx_pkt_preamble);
	seq_printf(m, "tx_pkt_mcs = %d\n", conf_params->tx_pkt_mcs);
	if (conf_params->tx_pkt_rate == 5)
		seq_printf(m, "tx_pkt_rate = 5.5\n");
	else
		seq_printf(m, "tx_pkt_rate = %d\n", conf_params->tx_pkt_rate);
	seq_printf(m, "tx_pkt_gap = %d\n", conf_params->tx_pkt_gap_us);
	seq_printf(m, "tx_pkt_num = %d\n", conf_params->tx_pkt_num);
	seq_printf(m, "tx_pkt_len = %d\n", conf_params->tx_pkt_len);
	seq_printf(m, "tx_power = %d\n", conf_params->tx_power);
	seq_printf(m, "ru_tone = %d\n", conf_params->ru_tone);
	seq_printf(m, "ru_index = %d\n", conf_params->ru_index);
	seq_printf(m, "rx_capture_length = %d\n", conf_params->capture_length);
	seq_printf(m, "rx_lna_gain = %d\n", conf_params->lna_gain);
	seq_printf(m, "rx_bb_gain = %d\n", conf_params->bb_gain);
	seq_printf(m, "tx_tone_freq = %d\n", conf_params->tx_tone_freq);
	seq_printf(m, "xo_val = %d\n",
		   conf_params->rf_params[NRF_WIFI_RF_PARAMS_OFF_CALIB_X0]);
	seq_printf(m, "init = %d\n", conf_params->chan.primary_num);
	seq_printf(m, "tx = %d\n", conf_params->tx);
	seq_printf(m, "rx = %d\n", conf_params->rx);
	seq_printf(m, "tx_pkt_cw = %d\n", conf_params->tx_pkt_cw);
	seq_printf(m, "reg_domain = %c%c\n", conf_params->country_code[0],
		   conf_params->country_code[1]);
	seq_printf(m, "bypass_reg_domain = %d\n",
		   conf_params->bypass_regulatory);
#endif
	return 0;
}

#ifdef CONFIG_NRF700X_RADIO_TEST

static bool check_valid_chan_2g(unsigned char chan_num)
{
	if ((chan_num >= 1) && (chan_num <= 14)) {
		return true;
	}

	return false;
}

static bool check_valid_chan_5g(unsigned char chan_num)
{
	if ((chan_num == 32) || (chan_num == 36) || (chan_num == 40) ||
	    (chan_num == 44) || (chan_num == 48) || (chan_num == 52) ||
	    (chan_num == 56) || (chan_num == 60) || (chan_num == 64) ||
	    (chan_num == 68) || (chan_num == 96) || (chan_num == 100) ||
	    (chan_num == 104) || (chan_num == 108) || (chan_num == 112) ||
	    (chan_num == 116) || (chan_num == 120) || (chan_num == 124) ||
	    (chan_num == 128) || (chan_num == 132) || (chan_num == 136) ||
	    (chan_num == 140) || (chan_num == 144) || (chan_num == 149) ||
	    (chan_num == 153) || (chan_num == 157) || (chan_num == 159) ||
	    (chan_num == 161) || (chan_num == 163) || (chan_num == 165) ||
	    (chan_num == 167) || (chan_num == 169) || (chan_num == 171) ||
	    (chan_num == 173) || (chan_num == 175) || (chan_num == 177)) {
		return true;
	}

	return false;
}

static bool check_valid_channel(unsigned char chan_num)
{
	bool ret = false;

	ret = check_valid_chan_2g(chan_num);

	if (ret) {
		goto out;
	}

	ret = check_valid_chan_5g(chan_num);

out:
	return ret;
}

enum nrf_wifi_status
nrf_wifi_radio_test_conf_init(struct rpu_conf_params *conf_params)
{
	enum nrf_wifi_status status = NRF_WIFI_STATUS_FAIL;
	unsigned char country_code[NRF_WIFI_COUNTRY_CODE_LEN] = { 0 };
	struct nrf_wifi_tx_pwr_ceil_params tx_pwr_ceil_params;

	/* Check and save regulatory country code currently set */
	if (strlen(conf_params->country_code)) {
		memcpy(country_code, conf_params->country_code,
		       NRF_WIFI_COUNTRY_CODE_LEN);
	}

	memset(conf_params, 0, sizeof(*conf_params));

	/* Initialize values which are other than 0 */
	conf_params->op_mode = RPU_OP_MODE_RADIO_TEST;

	status = nrf_wifi_fmac_rf_params_get(
		ctx->rpu_ctx, conf_params->rf_params, &tx_pwr_ceil_params);
	if (status != NRF_WIFI_STATUS_SUCCESS) {
		goto out;
	}

	conf_params->tx_pkt_nss = 1;
	conf_params->tx_pkt_gap_us = 0;

	conf_params->tx_power = MAX_TX_PWR_SYS_TEST;

	conf_params->chan.primary_num = 1;
	conf_params->tx_mode = 1;
	conf_params->tx_pkt_num = -1;
	conf_params->tx_pkt_len = 1400;
	conf_params->tx_pkt_preamble = 1;
	conf_params->tx_pkt_rate = 6;
	conf_params->he_ltf = 2;
	conf_params->he_gi = 2;
	conf_params->aux_adc_input_chain_id = 1;
	conf_params->ru_tone = 26;
	conf_params->ru_index = 1;
	conf_params->tx_pkt_cw = 15;
	conf_params->phy_calib = NRF_WIFI_DEF_PHY_CALIB;

	/* Store back the currently set country code */
	if (strlen(country_code)) {
		memcpy(conf_params->country_code, country_code,
		       NRF_WIFI_COUNTRY_CODE_LEN);
	} else {
		memcpy(conf_params->country_code, "00",
		       NRF_WIFI_COUNTRY_CODE_LEN);
	}
out:
	return status;
}

bool check_test_in_prog(char *err_str)
{
	if (ctx->conf_params.rx) {
		snprintf(err_str, MAX_ERR_STR_SIZE, "Disable RX\n");
		return false;
	}

	if (ctx->conf_params.tx) {
		snprintf(err_str, MAX_ERR_STR_SIZE, "Disable TX\n");
		return false;
	}

	return true;
}

static bool check_valid_data_rate(unsigned char tput_mode, unsigned int nss,
				  int dr, char *err_str)
{
	bool is_mcs = dr & 0x80;
	bool ret = false;

	if (dr == -1)
		return true;

	if (is_mcs) {
		dr = dr & 0x7F;

		if (tput_mode & RPU_TPUT_MODE_HT) {
			if (nss == 2) {
				if ((dr >= 8) && (dr <= 15)) {
					ret = true;
				} else {
					snprintf(err_str, MAX_ERR_STR_SIZE,
						 "Invalid MIMO HT MCS: %d\n",
						 dr);
				}
			}

			if (nss == 1) {
				if ((dr >= 0) && (dr <= 7)) {
					ret = true;
				} else {
					snprintf(err_str, MAX_ERR_STR_SIZE,
						 "Invalid SISO HT MCS: %d\n",
						 dr);
				}
			}

		} else if (tput_mode == RPU_TPUT_MODE_VHT) {
			if ((dr >= 0 && dr <= 9)) {
				ret = true;
			} else {
				snprintf(err_str, MAX_ERR_STR_SIZE,
					 "Invalid VHT MCS value: %d\n", dr);
			}
		} else if (tput_mode == RPU_TPUT_MODE_HE_SU) {
			if ((dr >= 0 && dr <= 7)) {
				ret = true;
			} else {
				snprintf(err_str, MAX_ERR_STR_SIZE,
					 "Invalid HE_SU MCS value: %d\n", dr);
			}
		} else if (tput_mode == RPU_TPUT_MODE_HE_ER_SU) {
			if ((dr >= 0 && dr <= 2)) {
				ret = true;
			} else {
				snprintf(err_str, MAX_ERR_STR_SIZE,
					 "Invalid HE_ER_SU MCS value: %d\n",
					 dr);
			}
		} else {
			snprintf(err_str, MAX_ERR_STR_SIZE,
				 "%s: Invalid throughput mode: %d\n", __func__,
				 dr);
		}
	} else {
		if (tput_mode != RPU_TPUT_MODE_LEGACY) {
			ret = false;
			snprintf(err_str, MAX_ERR_STR_SIZE,
				 "Invalid rate_flags for legacy: %d\n", dr);
		}

		if ((dr == 1) || (dr == 2) || (dr == 55) || (dr == 11) ||
		    (dr == 6) || (dr == 9) || (dr == 12) || (dr == 18) ||
		    (dr == 24) || (dr == 36) || (dr == 48) || (dr == 54) ||
		    (dr == -1)) {
			ret = true;
		} else {
			snprintf(err_str, MAX_ERR_STR_SIZE,
				 "Invalid Legacy Rate value: %d\n", dr);
		}
	}

	return ret;
}

static int nrf_wifi_radio_test_set_phy_calib_rxdc(struct nrf_wifi_ctx_lnx *ctx,
						  unsigned long val,
						  char *err_str)
{
	int err_val = -ENOEXEC;

	if (val > 1) {
		snprintf(err_str, MAX_ERR_STR_SIZE, "Invalid value %ld\n", val);
		err_val = -EINVAL;
		goto error;
	}

	if (!check_test_in_prog(err_str)) {
		err_val = -EFAULT;
		goto error;
	}

	if (val)
		ctx->conf_params.phy_calib = (ctx->conf_params.phy_calib |
					      NRF_WIFI_PHY_CALIB_FLAG_RXDC);
	else
		ctx->conf_params.phy_calib = (ctx->conf_params.phy_calib &
					      ~(NRF_WIFI_PHY_CALIB_FLAG_RXDC));
	return 0;
error:
	return err_val;
}

static int nrf_wifi_radio_test_set_phy_calib_txdc(struct nrf_wifi_ctx_lnx *ctx,
						  unsigned long val,
						  char *err_str)
{
	int err_val = -ENOEXEC;

	if (val > 1) {
		snprintf(err_str, MAX_ERR_STR_SIZE, "Invalid value %ld\n", val);
		err_val = -EINVAL;
		goto error;
	}

	if (!check_test_in_prog(err_str)) {
		err_val = -EFAULT;
		goto error;
	}

	if (val)
		ctx->conf_params.phy_calib = (ctx->conf_params.phy_calib |
					      NRF_WIFI_PHY_CALIB_FLAG_TXDC);
	else
		ctx->conf_params.phy_calib = (ctx->conf_params.phy_calib &
					      ~(NRF_WIFI_PHY_CALIB_FLAG_TXDC));
	return 0;
error:
	return err_val;
}

static int nrf_wifi_radio_test_set_phy_calib_txpow(struct nrf_wifi_ctx_lnx *ctx,
						   unsigned long val,
						   char *err_str)
{
	int err_val = -ENOEXEC;

	if (val > 1) {
		snprintf(err_str, MAX_ERR_STR_SIZE, "Invalid value %ld\n", val);
		err_val = -EINVAL;
		goto error;
	}

	if (!check_test_in_prog(err_str)) {
		err_val = -EFAULT;
		goto error;
	}

	if (val)
		ctx->conf_params.phy_calib = (ctx->conf_params.phy_calib |
					      NRF_WIFI_PHY_CALIB_FLAG_TXPOW);
	else
		ctx->conf_params.phy_calib = (ctx->conf_params.phy_calib &
					      ~(NRF_WIFI_PHY_CALIB_FLAG_TXPOW));
	return 0;
error:
	return err_val;
}

static int nrf_wifi_radio_test_set_phy_calib_rxiq(struct nrf_wifi_ctx_lnx *ctx,
						  unsigned long val,
						  char *err_str)
{
	int err_val = -ENOEXEC;

	if (val > 1) {
		snprintf(err_str, MAX_ERR_STR_SIZE, "Invalid value %ld\n", val);
		err_val = -EINVAL;
		goto error;
	}

	if (!check_test_in_prog(err_str)) {
		err_val = -EFAULT;
		goto error;
	}

	if (val)
		ctx->conf_params.phy_calib = (ctx->conf_params.phy_calib |
					      NRF_WIFI_PHY_CALIB_FLAG_RXIQ);
	else
		ctx->conf_params.phy_calib = (ctx->conf_params.phy_calib &
					      ~(NRF_WIFI_PHY_CALIB_FLAG_RXIQ));
	return 0;
error:
	return err_val;
}

static int nrf_wifi_radio_test_set_phy_calib_txiq(struct nrf_wifi_ctx_lnx *ctx,
						  unsigned long val,
						  char *err_str)
{
	int err_val = -ENOEXEC;

	if (val > 1) {
		snprintf(err_str, MAX_ERR_STR_SIZE, "Invalid value %ld\n", val);
		err_val = -EINVAL;
		goto error;
	}

	if (!check_test_in_prog(err_str)) {
		err_val = -EFAULT;
		goto error;
	}

	if (val)
		ctx->conf_params.phy_calib = (ctx->conf_params.phy_calib |
					      NRF_WIFI_PHY_CALIB_FLAG_TXIQ);
	else
		ctx->conf_params.phy_calib = (ctx->conf_params.phy_calib &
					      ~(NRF_WIFI_PHY_CALIB_FLAG_TXIQ));

	return 0;
error:
	return err_val;
}

static int nrf_wifi_radio_test_set_he_ltf(struct nrf_wifi_ctx_lnx *ctx,
					  unsigned long val, char *err_str)
{
	int err_val = -ENOEXEC;

	if ((val < 0) || (val > 2)) {
		snprintf(err_str, MAX_ERR_STR_SIZE, "Invalid value %lu\n", val);
		err_val = -EINVAL;
		goto error;
	}

	ctx->conf_params.he_ltf = val;

	return 0;
error:
	return err_val;
}

static int nrf_wifi_radio_test_set_he_gi(struct nrf_wifi_ctx_lnx *ctx,
					 unsigned long val, char *err_str)
{
	int err_val = -ENOEXEC;

	if ((val < 0) || (val > 2)) {
		snprintf(err_str, MAX_ERR_STR_SIZE, "Invalid value %lu\n", val);
		err_val = -EINVAL;
		goto error;
	}
	ctx->conf_params.he_gi = val;

	return 0;
error:
	return err_val;
}

static int
nrf_wifi_radio_test_set_tx_pkt_tput_mode(struct nrf_wifi_ctx_lnx *ctx,
					 unsigned long val, char *err_str)
{
	int err_val = -ENOEXEC;

	if (val >= RPU_TPUT_MODE_MAX) {
		snprintf(err_str, MAX_ERR_STR_SIZE, "Invalid value %lu\n", val);
		err_val = -EINVAL;
		goto error;
	}

	if (!check_test_in_prog(err_str)) {
		err_val = -EFAULT;
		goto error;
	}

	ctx->conf_params.tx_pkt_tput_mode = val;

	return 0;
error:
	return err_val;
}

static int nrf_wifi_radio_test_set_tx_pkt_sgi(struct nrf_wifi_ctx_lnx *ctx,
					      unsigned long val, char *err_str)
{
	int err_val = -ENOEXEC;

	if ((val < 0) || (val > 1)) {
		snprintf(err_str, MAX_ERR_STR_SIZE, "Invalid value %lu\n", val);
		err_val = -EINVAL;
		goto error;
	}

	ctx->conf_params.tx_pkt_sgi = val;

	return 0;
error:
	return err_val;
}

static int nrf_wifi_radio_test_set_tx_pkt_preamble(struct nrf_wifi_ctx_lnx *ctx,
						   unsigned long val,
						   char *err_str)
{
	int err_val = -ENOEXEC;

	if (!check_test_in_prog(err_str)) {
		err_val = -EFAULT;
		goto error;
	}

	if (ctx->conf_params.tx_pkt_tput_mode == 0) {
		if ((val != RPU_PKT_PREAMBLE_SHORT) &&
		    (val != RPU_PKT_PREAMBLE_LONG)) {
			snprintf(err_str, MAX_ERR_STR_SIZE,
				 "Invalid value %lu\n", val);
			err_val = -EINVAL;
			goto error;
		}
	} else {
		if (val != RPU_PKT_PREAMBLE_MIXED) {
			snprintf(err_str, MAX_ERR_STR_SIZE,
				 "Invalid value %lu\n", val);
			err_val = -EINVAL;
			goto error;
		}
	}

	ctx->conf_params.tx_pkt_preamble = val;

	return 0;
error:
	return err_val;
}

static int nrf_wifi_radio_test_set_tx_pkt_mcs(struct nrf_wifi_ctx_lnx *ctx,
					      long val, char *err_str)
{
	int err_val = -ENOEXEC;

	if (!check_test_in_prog(err_str)) {
		err_val = -EFAULT;
		goto error;
	}
	if (ctx->conf_params.tx_pkt_rate != -1) {
		snprintf(err_str, MAX_ERR_STR_SIZE, "tx_pkt_rate is set\n");
		err_val = -EFAULT;
		goto error;
	}

	if (!(check_valid_data_rate(ctx->conf_params.tx_pkt_tput_mode,
				    ctx->conf_params.tx_pkt_nss, val | 0x80,
				    err_str))) {
		err_val = -EINVAL;
		goto error;
	}

	ctx->conf_params.tx_pkt_mcs = val;

	return 0;
error:
	return err_val;
}

static int nrf_wifi_radio_test_set_tx_pkt_rate(struct nrf_wifi_ctx_lnx *ctx,
					       long val, char *err_str)
{
	int err_val = -ENOEXEC;

	if (!check_test_in_prog(err_str)) {
		err_val = -EFAULT;
		goto error;
	}

	if (!(check_valid_data_rate(ctx->conf_params.tx_pkt_tput_mode,
				    ctx->conf_params.tx_pkt_nss, val,
				    err_str))) {
		err_val = -EINVAL;
		goto error;
	}

	ctx->conf_params.tx_pkt_rate = val;

	return 0;
error:
	return err_val;
}

static int nrf_wifi_radio_test_set_tx_pkt_gap(struct nrf_wifi_ctx_lnx *ctx,
					      unsigned long val, char *err_str)
{
	int err_val = -ENOEXEC;

	if (val > 200000) {
		snprintf(err_str, MAX_ERR_STR_SIZE, "Invalid value %lu\n", val);
		err_val = -EINVAL;
		goto error;
	}

	if (!check_test_in_prog(err_str)) {
		err_val = -EFAULT;
		goto error;
	}

	ctx->conf_params.tx_pkt_gap_us = val;

	return 0;
error:
	return err_val;
}

static int nrf_wifi_radio_test_set_tx_pkt_num(struct nrf_wifi_ctx_lnx *ctx,
					      long val, char *err_str)
{
	int err_val = -ENOEXEC;

	if ((val < -1) || (val == 0)) {
		snprintf(err_str, MAX_ERR_STR_SIZE, "Invalid value %lu\n", val);
		err_val = -EINVAL;
		goto error;
	}

	if (!check_test_in_prog(err_str)) {
		err_val = -EFAULT;
		goto error;
	}

	ctx->conf_params.tx_pkt_num = val;

	return 0;
error:
	return err_val;
}

static int nrf_wifi_radio_test_set_tx_pkt_len(struct nrf_wifi_ctx_lnx *ctx,
					      unsigned long val, char *err_str)
{
	int err_val = -ENOEXEC;

	if (val == 0) {
		snprintf(err_str, MAX_ERR_STR_SIZE, "Invalid value %lu\n", val);
		return -EINVAL;
		goto error;
	}

	if (ctx->conf_params.tx_pkt_tput_mode == RPU_TPUT_MODE_MAX) {
		snprintf(err_str, MAX_ERR_STR_SIZE,
			 "Throughput mode not set\n");
		err_val = -EINVAL;
		goto error;
	}

	if (ctx->conf_params.tx_pkt_tput_mode == RPU_TPUT_MODE_LEGACY) {
		if (val > 4000) {
			snprintf(
				err_str, MAX_ERR_STR_SIZE,
				"max 'tx_pkt_len' size for legacy is 4000 bytes\n");
			err_val = -ENOEXEC;
			goto error;
		}
	} else if (ctx->conf_params.tx_pkt_tput_mode == RPU_TPUT_MODE_HE_TB) {
		if (ctx->conf_params.ru_tone == 26) {
			if (val >= 350) {
				snprintf(
					err_str, MAX_ERR_STR_SIZE,
					"'tx_pkt_len' has to be less than 350 bytes\n");
				err_val = -ENOEXEC;
				goto error;
			}
		} else if (ctx->conf_params.ru_tone == 52) {
			if (val >= 800) {
				snprintf(
					err_str, MAX_ERR_STR_SIZE,
					"'tx_pkt_len' has to be less than 800 bytes\n");
				err_val = -ENOEXEC;
				goto error;
			}
		} else if (ctx->conf_params.ru_tone == 106) {
			if (val >= 1800) {
				snprintf(
					err_str, MAX_ERR_STR_SIZE,
					"'tx_pkt_len' has to be less than 1800 bytes\n");
				err_val = -ENOEXEC;
				goto error;
			}
		} else if (ctx->conf_params.ru_tone == 242) {
			if (val >= 4000) {
				snprintf(
					err_str, MAX_ERR_STR_SIZE,
					"'tx_pkt_len' has to be less than 4000 bytes\n");
				err_val = -ENOEXEC;
				goto error;
			}
		} else {
			snprintf(err_str, MAX_ERR_STR_SIZE,
				 "'ru_tone' not set\n");
			err_val = -ENOEXEC;
			goto error;
		}
	}

	if (!check_test_in_prog(err_str)) {
		err_val = -EFAULT;
		goto error;
	}

	ctx->conf_params.tx_pkt_len = val;

	return 0;
error:
	return err_val;
}

static int nrf_wifi_radio_test_set_tx_power(struct nrf_wifi_ctx_lnx *ctx,
					    unsigned long val, char *err_str)
{
	int err_val = -ENOEXEC;

	if (((val > MAX_TX_PWR_RADIO_TEST) && (val != MAX_TX_PWR_SYS_TEST))) {
		snprintf(err_str, MAX_ERR_STR_SIZE,
			 "Invalid TX power setting %lu\n", val);
		err_val = -EINVAL;
		goto error;
	}

	if (!check_test_in_prog(err_str)) {
		err_val = -EFAULT;
		goto error;
	}

	ctx->conf_params.tx_power = val;

	return 0;
error:
	return err_val;
}

static int nrf_wifi_radio_test_set_ru_tone(struct nrf_wifi_ctx_lnx *ctx,
					   unsigned long val, char *err_str)
{
	int err_val = -ENOEXEC;

	if ((val != 26) && (val != 52) && (val != 106) && (val != 242)) {
		snprintf(err_str, MAX_ERR_STR_SIZE, "Invalid value %lu\n", val);
		err_val = -EINVAL;
		goto error;
	}

	if (!check_test_in_prog(err_str)) {
		err_val = -EFAULT;
		goto error;
	}

	ctx->conf_params.ru_tone = val;

	return 0;
error:
	return err_val;
}

static int nrf_wifi_radio_test_set_ru_index(struct nrf_wifi_ctx_lnx *ctx,
					    unsigned long val, char *err_str)
{
	int err_val = -ENOEXEC;

	if (ctx->conf_params.ru_tone == 26) {
		if ((val < 1) || (val > 9)) {
			snprintf(err_str, MAX_ERR_STR_SIZE,
				 "Invalid value %lu\n", val);
			err_val = -EINVAL;
			goto error;
		}
	} else if (ctx->conf_params.ru_tone == 52) {
		if ((val < 1) || (val > 4)) {
			snprintf(err_str, MAX_ERR_STR_SIZE,
				 "Invalid value %lu\n", val);
			err_val = -EINVAL;
			goto error;
		}
	} else if (ctx->conf_params.ru_tone == 106) {
		if ((val < 1) || (val > 2)) {
			snprintf(err_str, MAX_ERR_STR_SIZE,
				 "Invalid value %lu\n", val);
			err_val = -EINVAL;
			goto error;
		}
	} else if (ctx->conf_params.ru_tone == 242) {
		if ((val != 1)) {
			snprintf(err_str, MAX_ERR_STR_SIZE,
				 "Invalid value %lu\n", val);
			err_val = -EINVAL;
			goto error;
		}
	} else {
		snprintf(err_str, MAX_ERR_STR_SIZE, "RU tone not set\n");
		err_val = -EINVAL;
		goto error;
	}

	if (!check_test_in_prog(err_str)) {
		err_val = -EFAULT;
		goto error;
	}

	ctx->conf_params.ru_index = val;

	return 0;
error:
	return err_val;
}

static int
nrf_wifi_radio_test_set_rx_capture_length(struct nrf_wifi_ctx_lnx *ctx,
					  unsigned long val, char *err_str)
{
	int err_val = -ENOEXEC;

	if (val >= 16384) {
		snprintf(err_str, MAX_ERR_STR_SIZE,
			 "'capture_length' has to be less than 16384\n");
		err_val = -EINVAL;
		goto error;
	}

	if (!check_test_in_prog(err_str)) {
		err_val = -ENOEXEC;
		goto error;
	}

	ctx->conf_params.capture_length = val;

	return 0;
error:
	return err_val;
}

static int nrf_wifi_radio_test_set_rx_lna_gain(struct nrf_wifi_ctx_lnx *ctx,
					       unsigned long val, char *err_str)
{
	int err_val = -ENOEXEC;

	if ((val > 4) || (val < 0)) {
		snprintf(err_str, MAX_ERR_STR_SIZE,
			 "'lna_gain' has to be in between 0 to 4\n");
		err_val = -ENOEXEC;
		goto error;
	}

	if (!check_test_in_prog(err_str)) {
		err_val = -ENOEXEC;
		goto error;
	}

	ctx->conf_params.lna_gain = val;

	return 0;
error:
	return err_val;
}

static int nrf_wifi_radio_test_set_rx_bb_gain(struct nrf_wifi_ctx_lnx *ctx,
					      unsigned long val, char *err_str)
{
	int err_val = -ENOEXEC;

	if ((val > 31) || (val < 0)) {
		snprintf(err_str, MAX_ERR_STR_SIZE,
			 "'bb_gain' has to be in between 0 to 31\n");
		err_val = -ENOEXEC;
		goto error;
	}

	if (!check_test_in_prog(err_str)) {
		err_val = -ENOEXEC;
		goto error;
	}

	ctx->conf_params.bb_gain = val;

	return 0;
error:
	return err_val;
}

static int nrf_wifi_radio_test_set_tx_tone_freq(struct nrf_wifi_ctx_lnx *ctx,
						unsigned long val,
						char *err_str)
{
	int err_val = -ENOEXEC;

	if ((val > 10) || (val < -10)) {
		snprintf(err_str, MAX_ERR_STR_SIZE,
			 "'tx_tone_freq' has to be in between -10 to +10\n");
		err_val = -ENOEXEC;
		goto error;
	}

	if (!check_test_in_prog(err_str)) {
		err_val = -ENOEXEC;
		goto error;
	}

	ctx->conf_params.tx_tone_freq = val;

	return 0;
error:
	return err_val;
}

static int nrf_wifi_radio_set_dpd(struct nrf_wifi_ctx_lnx *ctx,
				  unsigned long val, char *err_str)
{
	enum nrf_wifi_status status = NRF_WIFI_STATUS_FAIL;
	int err_val = -ENOEXEC;

	if (val > 1) {
		snprintf(err_str, MAX_ERR_STR_SIZE, "Invalid value %lu\n", val);
		err_val = -EINVAL;
		goto error;
	}

	if (val == 1) {
		if (!check_test_in_prog(err_str)) {
			err_val = -ENOEXEC;
			goto error;
		}
	}

	ctx->rf_test_run = true;
	ctx->rf_test = NRF_WIFI_RF_TEST_DPD;

	status = nrf_wifi_fmac_rf_test_dpd(ctx->rpu_ctx, val);

	if (status != NRF_WIFI_STATUS_SUCCESS) {
		snprintf(err_str, MAX_ERR_STR_SIZE, "DPD programming failed\n");
		err_val = -ENOEXEC;
		goto error;
	}

	ctx->rf_test_run = false;
	ctx->rf_test = NRF_WIFI_RF_TEST_MAX;

	return 0;
error:
	return err_val;
}

static int nrf_wifi_radio_set_xo_val(struct nrf_wifi_ctx_lnx *ctx,
				     unsigned long val, char *err_str)
{
	enum nrf_wifi_status status = NRF_WIFI_STATUS_FAIL;
	int err_val = -ENOEXEC;

	if (val > 0x7f) {
		snprintf(err_str, MAX_ERR_STR_SIZE,
			 "Invalid value XO value should be <= 0x7f\n");
		err_val = -EINVAL;
		goto error;
	}

	if (val < 0) {
		snprintf(err_str, MAX_ERR_STR_SIZE,
			 "Invalid value XO value should be >= 0\n");
		err_val = -EINVAL;
		goto error;
	}

	if (val == 1) {
		if (!check_test_in_prog(err_str)) {
			err_val = -ENOEXEC;
			goto error;
		}
	}

	ctx->rf_test_run = true;
	ctx->rf_test = NRF_WIFI_RF_TEST_XO_CALIB;

	status = nrf_wifi_fmac_set_xo_val(ctx->rpu_ctx, (unsigned char)val);

	if (status != NRF_WIFI_STATUS_SUCCESS) {
		snprintf(err_str, MAX_ERR_STR_SIZE,
			 "XO value programming failed\n");
		err_val = -ENOEXEC;
		goto error;
	}

	ctx->conf_params.rf_params[NRF_WIFI_RF_PARAMS_OFF_CALIB_X0] = val;

	ctx->rf_test_run = false;
	ctx->rf_test = NRF_WIFI_RF_TEST_MAX;

	return 0;
error:
	return err_val;
}

static int nrf_wifi_radio_test_init(struct nrf_wifi_ctx_lnx *ctx,
				    unsigned long val, char *err_str)
{
	enum nrf_wifi_status status = NRF_WIFI_STATUS_FAIL;
	int err_val = -ENOEXEC;

	if (!(check_valid_channel(val))) {
		snprintf(err_str, MAX_ERR_STR_SIZE, "Invalid value %lu\n", val);
		err_val = -EINVAL;
		goto error;
	}

	if (ctx->conf_params.rx) {
		ctx->conf_params.rx = 0;

		status = nrf_wifi_fmac_radio_test_prog_rx(ctx->rpu_ctx,
							  &ctx->conf_params);

		if (status != NRF_WIFI_STATUS_SUCCESS) {
			snprintf(err_str, MAX_ERR_STR_SIZE,
				 "Disabling RX failed\n");
			err_val = -ENOEXEC;
			goto error;
		}
	}

	if (ctx->conf_params.tx) {
		ctx->conf_params.tx = 0;

		status = nrf_wifi_fmac_radio_test_prog_tx(ctx->rpu_ctx,
							  &ctx->conf_params);

		if (status != NRF_WIFI_STATUS_SUCCESS) {
			snprintf(err_str, MAX_ERR_STR_SIZE,
				 "Disabling TX failed\n");
			return -ENOEXEC;
			goto error;
		}
	}

	if (ctx->rf_test_run) {
		if (ctx->rf_test != NRF_WIFI_RF_TEST_TX_TONE) {
			snprintf(err_str, MAX_ERR_STR_SIZE,
				 "Unexpected: RF Test (%d) running\n",
				 ctx->rf_test);

			err_val = -ENOEXEC;
			goto error;
		}

		status = nrf_wifi_fmac_rf_test_tx_tone(
			ctx->rpu_ctx, 0, ctx->conf_params.tx_tone_freq,
			ctx->conf_params.tx_power);

		if (status != NRF_WIFI_STATUS_SUCCESS) {
			snprintf(err_str, MAX_ERR_STR_SIZE,
				 "Disabling TX tone test failed\n");
			err_val = -ENOEXEC;
			goto error;
		}

		ctx->rf_test_run = false;
		ctx->rf_test = NRF_WIFI_RF_TEST_MAX;
	}

	status = nrf_wifi_radio_test_conf_init(&ctx->conf_params);

	if (status != NRF_WIFI_STATUS_SUCCESS) {
		snprintf(err_str, MAX_ERR_STR_SIZE,
			 "Configuration init failed\n");
		err_val = -ENOEXEC;
		goto error;
	}

	ctx->conf_params.chan.primary_num = val;

	status = nrf_wifi_fmac_radio_test_init(ctx->rpu_ctx, &ctx->conf_params);

	if (status != NRF_WIFI_STATUS_SUCCESS) {
		snprintf(err_str, MAX_ERR_STR_SIZE,
			 "Programming init failed\n");
		err_val = -ENOEXEC;
		goto error;
	}

	return 0;
error:
	return err_val;
}

static int check_channel_settings(struct nrf_wifi_ctx_lnx *ctx,
				  unsigned char tput_mode,
				  struct chan_params *chan, char *err_str)
{
	int err_val = -ENOEXEC;

	if (tput_mode == RPU_TPUT_MODE_LEGACY) {
		if (chan->bw != RPU_CH_BW_20) {
			snprintf(
				err_str, MAX_ERR_STR_SIZE,
				"Invalid bandwidth setting for legacy channel = %d\n",
				chan->primary_num);
			err_val = -ENOEXEC;
			goto error;
		}
	} else if (tput_mode == RPU_TPUT_MODE_HT) {
		if (chan->bw != RPU_CH_BW_20) {
			snprintf(
				err_str, MAX_ERR_STR_SIZE,
				"Invalid bandwidth setting for HT channel = %d\n",
				chan->primary_num);
			err_val = -ENOEXEC;
			goto error;
		}
	} else if (tput_mode == RPU_TPUT_MODE_VHT) {
		if ((chan->primary_num >= 1) && (chan->primary_num <= 14)) {
			snprintf(err_str, MAX_ERR_STR_SIZE,
				 "VHT setting not allowed in 2.4GHz band\n");
			err_val = -ENOEXEC;
			goto error;
		}

		if (chan->bw != RPU_CH_BW_20) {
			snprintf(
				err_str, MAX_ERR_STR_SIZE,
				"Invalid bandwidth setting for VHT channel = %d\n",
				chan->primary_num);
			err_val = -ENOEXEC;
			goto error;
		}
	} else if ((tput_mode == RPU_TPUT_MODE_HE_SU) ||
		   (tput_mode == RPU_TPUT_MODE_HE_TB) ||
		   (tput_mode == RPU_TPUT_MODE_HE_ER_SU)) {
		if (chan->bw != RPU_CH_BW_20) {
			snprintf(
				err_str, MAX_ERR_STR_SIZE,
				"Invalid bandwidth setting for HE channel = %d\n",
				chan->primary_num);
			err_val = -ENOEXEC;
			goto error;
		}
	} else {
		snprintf(err_str, MAX_ERR_STR_SIZE,
			 "Invalid throughput mode = %d\n", tput_mode);
		err_val = -ENOEXEC;
		goto error;
	}

	return 0;
error:
	return err_val;
}

void nrf_wifi_radio_test_get_max_tx_power_params(void)
{
	/*Max TX power is represented in 0.25dB resolution
	 *So,multiply 4 to MAX_TX_PWR_RADIO_TEST and
	 *configure the RF params corresponding to Max TX power
	 */
	ctx->conf_params.rf_params[NRF_WIFI_RF_PARAMS_OFF_CALIB_PWR2G] =
		(MAX_TX_PWR_RADIO_TEST << 2);

	ctx->conf_params.rf_params[NRF_WIFI_RF_PARAMS_OFF_CALIB_PWR2GM0M7] =
		(MAX_TX_PWR_RADIO_TEST << 2);

	ctx->conf_params.rf_params[NRF_WIFI_RF_PARAMS_OFF_CALIB_PWR2GM0M7 + 1] =
		(MAX_TX_PWR_RADIO_TEST << 2);

	ctx->conf_params.rf_params[NRF_WIFI_RF_PARAMS_OFF_CALIB_PWR5GM7] =
		(MAX_TX_PWR_RADIO_TEST << 2);

	ctx->conf_params.rf_params[NRF_WIFI_RF_PARAMS_OFF_CALIB_PWR5GM7 + 1] =
		(MAX_TX_PWR_RADIO_TEST << 2);

	ctx->conf_params.rf_params[NRF_WIFI_RF_PARAMS_OFF_CALIB_PWR5GM7 + 2] =
		(MAX_TX_PWR_RADIO_TEST << 2);

	ctx->conf_params.rf_params[NRF_WIFI_RF_PARAMS_OFF_CALIB_PWR5GM0] =
		(MAX_TX_PWR_RADIO_TEST << 2);

	ctx->conf_params.rf_params[NRF_WIFI_RF_PARAMS_OFF_CALIB_PWR5GM0 + 1] =
		(MAX_TX_PWR_RADIO_TEST << 2);

	ctx->conf_params.rf_params[NRF_WIFI_RF_PARAMS_OFF_CALIB_PWR5GM0 + 2] =
		(MAX_TX_PWR_RADIO_TEST << 2);
}

static int nrf_wifi_radio_test_set_tx(struct nrf_wifi_ctx_lnx *ctx,
				      unsigned long val, char *err_str)
{
	enum nrf_wifi_status status = NRF_WIFI_STATUS_FAIL;
	int err_val = -ENOEXEC;

	if (val > 1) {
		snprintf(err_str, MAX_ERR_STR_SIZE, "Invalid value %lu\n", val);
		err_val = -EINVAL;
		goto error;
	}

	if (val == 1) {
		if (!check_test_in_prog(err_str)) {
			err_val = -ENOEXEC;
			goto error;
		}

		if (check_channel_settings(
			    ctx, ctx->conf_params.tx_pkt_tput_mode,
			    &ctx->conf_params.chan, err_str) != 0) {
			snprintf(err_str, MAX_ERR_STR_SIZE,
				 "Invalid channel settings\n");
			err_val = -ENOEXEC;
			goto error;
		}

		/*Max TX power values differ based on the test being performed.
		 *For TX EVM Vs Power, Max TX power required is
		 *"MAX_TX_PWR_RADIO_TEST" (24dB) whereas for testing the
		 *Max TX power for which both EVM and spectrum mask are passing
		 *for specific band and MCS/rate, TX power values will be read from
		 *RF params string
		 */
		if (ctx->conf_params.tx_power != MAX_TX_PWR_SYS_TEST) {
			nrf_wifi_radio_test_get_max_tx_power_params();
		}
	}

	ctx->conf_params.tx = val;

	status = nrf_wifi_fmac_radio_test_prog_tx(ctx->rpu_ctx,
						  &ctx->conf_params);

	if (status != NRF_WIFI_STATUS_SUCCESS) {
		snprintf(err_str, MAX_ERR_STR_SIZE, "Programming TX failed\n");
		err_val = -ENOEXEC;
		goto error;
	}

	return 0;
error:
	return err_val;
}

static int nrf_wifi_radio_test_set_rx(struct nrf_wifi_ctx_lnx *ctx,
				      unsigned long val, char *err_str)
{
	enum nrf_wifi_status status = NRF_WIFI_STATUS_FAIL;
	int err_val = -ENOEXEC;

	if (val > 1) {
		snprintf(err_str, MAX_ERR_STR_SIZE, "Invalid value %lu\n", val);
		err_val = -EINVAL;
		goto error;
	}

	if (val == 1) {
		if (!check_test_in_prog(err_str)) {
			err_val = -ENOEXEC;
			goto error;
		}
	}

	ctx->conf_params.rx = val;

	status = nrf_wifi_fmac_radio_test_prog_rx(ctx->rpu_ctx,
						  &ctx->conf_params);

	if (status != NRF_WIFI_STATUS_SUCCESS) {
		snprintf(err_str, MAX_ERR_STR_SIZE, "Programming RX failed\n");
		err_val = -ENOEXEC;
		goto error;
	}

	return 0;
error:
	return err_val;
}

static int nrf_wifi_radio_test_rx_cap(struct nrf_wifi_ctx_lnx *ctx,
				      unsigned long rx_cap_type, char *err_str)
{
	enum nrf_wifi_status status = NRF_WIFI_STATUS_FAIL;
	int err_val = -ENOEXEC;

	if ((rx_cap_type != NRF_WIFI_RF_TEST_RX_ADC_CAP) &&
	    (rx_cap_type != NRF_WIFI_RF_TEST_RX_STAT_PKT_CAP) &&
	    (rx_cap_type != NRF_WIFI_RF_TEST_RX_DYN_PKT_CAP)) {
		snprintf(err_str, MAX_ERR_STR_SIZE, "Invalid value %lu\n",
			 rx_cap_type);
		err_val = -EINVAL;
		goto error;
	}

	if (!ctx->conf_params.capture_length) {
		snprintf(err_str, MAX_ERR_STR_SIZE,
			 "%s: Invalid rx_capture_length %d\n", __func__,
			 ctx->conf_params.capture_length);
		err_val = -EINVAL;
		goto error;
	}

	if (!check_test_in_prog(err_str)) {
		err_val = -ENOEXEC;
		goto error;
	}

	if (ctx->rx_cap_buf)
		kfree(ctx->rx_cap_buf);

	ctx->rx_cap_buf =
		kzalloc((ctx->conf_params.capture_length * 3), sizeof(char));

	if (!ctx->rx_cap_buf) {
		pr_err("%s: Unable to allocate (%d) bytes for RX capture\n",
		       __func__, (ctx->conf_params.capture_length * 3));
		status = NRF_WIFI_STATUS_FAIL;
		goto error;
	}

	ctx->rf_test_run = true;
	ctx->rf_test = NRF_WIFI_RF_TEST_RX_ADC_CAP;

	status = nrf_wifi_fmac_rf_test_rx_cap(ctx->rpu_ctx, rx_cap_type,
					      ctx->rx_cap_buf,
					      ctx->conf_params.capture_length,
					      ctx->conf_params.lna_gain,
					      ctx->conf_params.bb_gain);

	if (status != NRF_WIFI_STATUS_SUCCESS) {
		pr_err("%s RX ADC capture programming failed\n", __func__);
		goto error;
	}

	ctx->rf_test_run = false;
	ctx->rf_test = NRF_WIFI_RF_TEST_MAX;

	return 0;
error:
	if (ctx->rx_cap_buf)
		kfree(ctx->rx_cap_buf);

	return err_val;
}

static int nrf_wifi_radio_test_tx_tone(struct nrf_wifi_ctx_lnx *ctx,
				       unsigned long val, char *err_str)
{
	enum nrf_wifi_status status = NRF_WIFI_STATUS_FAIL;
	int err_val = -ENOEXEC;

	if (val > 1) {
		snprintf(err_str, MAX_ERR_STR_SIZE, "Invalid value %lu\n", val);
		err_val = -EINVAL;
		goto error;
	}

	if (val == 1) {
		if (!check_test_in_prog(err_str)) {
			err_val = -ENOEXEC;
			goto error;
		}
	}

	status = nrf_wifi_fmac_rf_test_tx_tone(ctx->rpu_ctx, (unsigned char)val,
					       ctx->conf_params.tx_tone_freq,
					       ctx->conf_params.tx_power);

	if (status != NRF_WIFI_STATUS_SUCCESS) {
		snprintf(err_str, MAX_ERR_STR_SIZE,
			 "TX tone programming failed\n");
		err_val = -ENOEXEC;
		goto error;
	}

	if (val == 1) {
		ctx->rf_test_run = true;
		ctx->rf_test = NRF_WIFI_RF_TEST_TX_TONE;
	} else {
		ctx->rf_test_run = false;
		ctx->rf_test = NRF_WIFI_RF_TEST_MAX;
	}

	return 0;
error:
	return err_val;
}

static int nrf_wifi_radio_get_temperature(struct nrf_wifi_ctx_lnx *ctx,
					  unsigned long val, char *err_str)
{
	enum nrf_wifi_status status = NRF_WIFI_STATUS_FAIL;
	int err_val = -ENOEXEC;

	if (val > 1) {
		snprintf(err_str, MAX_ERR_STR_SIZE, "Invalid value %lu\n", val);
		err_val = -EINVAL;
		goto error;
	}

	if (val == 1) {
		if (!check_test_in_prog(err_str)) {
			err_val = -ENOEXEC;
			goto error;
		}
	}

	ctx->rf_test_run = true;
	ctx->rf_test = NRF_WIFI_RF_TEST_GET_TEMPERATURE;

	status = nrf_wifi_fmac_rf_get_temp(ctx->rpu_ctx);

	if (status != NRF_WIFI_STATUS_SUCCESS) {
		snprintf(err_str, MAX_ERR_STR_SIZE, "DPD programming failed\n");
		goto error;
	}

	ctx->rf_test_run = false;
	ctx->rf_test = NRF_WIFI_RF_TEST_MAX;

	return 0;
error:
	return err_val;
}

static int nrf_wifi_radio_get_rf_rssi(struct nrf_wifi_ctx_lnx *ctx,
				      unsigned long val, char *err_str)
{
	enum nrf_wifi_status status = NRF_WIFI_STATUS_FAIL;
	int err_val = -ENOEXEC;

	if (val > 1) {
		snprintf(err_str, MAX_ERR_STR_SIZE, "Invalid value %lu\n", val);
		err_val = -EINVAL;
		goto error;
	}

	if (val == 1) {
		if (!check_test_in_prog(err_str)) {
			err_val = -ENOEXEC;
			goto error;
		}
	}

	ctx->rf_test_run = true;
	ctx->rf_test = NRF_WIFI_RF_TEST_RF_RSSI;

	status = nrf_wifi_fmac_rf_get_rf_rssi(ctx->rpu_ctx);

	if (status != NRF_WIFI_STATUS_SUCCESS) {
		snprintf(err_str, MAX_ERR_STR_SIZE, "RF RSSI get failed\n");
		err_val = -EINVAL;
		goto error;
	}

	ctx->rf_test_run = false;
	ctx->rf_test = NRF_WIFI_RF_TEST_MAX;

	return 0;
error:
	return err_val;
}

static int nrf_wifi_radio_comp_opt_xo_val(struct nrf_wifi_ctx_lnx *ctx,
					  unsigned long val, char *err_str)
{
	enum nrf_wifi_status status = NRF_WIFI_STATUS_FAIL;

	int err_val = -ENOEXEC;

	if (!check_test_in_prog(err_str)) {
		err_val = -ENOEXEC;
		goto error;
	}

	ctx->rf_test_run = true;
	ctx->rf_test = NRF_WIFI_RF_TEST_XO_TUNE;

	status = nrf_wifi_fmac_rf_test_compute_xo(ctx->rpu_ctx);

	if (status != NRF_WIFI_STATUS_SUCCESS) {
		snprintf(err_str, MAX_ERR_STR_SIZE,
			 "XO value computation failed\n");
		err_val = -EINVAL;
		goto error;
	}

	ctx->rf_test_run = false;
	ctx->rf_test = NRF_WIFI_RF_TEST_MAX;

	return 0;
error:
	return err_val;
}

#ifdef notyet
static int nrf_wifi_radio_test_get_stats(struct nrf_wifi_ctx_lnx *ctx,
					 unsigned long val, char *err_str)
{
	enum nrf_wifi_status status = NRF_WIFI_STATUS_FAIL;
	struct rpu_op_stats stats;
	int err_val = -ENOEXEC;

	memset(&stats, 0, sizeof(stats));

	status = nrf_wifi_fmac_stats_get(ctx->rpu_ctx, ctx->conf_params.op_mode,
					 &stats);

	if (status != NRF_WIFI_STATUS_SUCCESS) {
		snprintf(err_str, MAX_ERR_STR_SIZE,
			 "nrf_wifi_fmac_stats_get failed\n");
		err_val = -ENOEXEC;
		goto error;
	}

	return 0;
error:
	return err_val;
}
#endif

static int nrf_wifi_radio_test_set_tx_pkt_cw(struct nrf_wifi_ctx_lnx *ctx,
					     long val, char *err_str)
{
	int err_val = -ENOEXEC;

	if (!((val == 0) || (val == 3) || (val == 7) || (val == 15) ||
	      (val == 31) || (val == 63) || (val == 127) || (val == 255) ||
	      (val == 511) || (val == 1023))) {
		snprintf(err_str, MAX_ERR_STR_SIZE, "Invalid value %lu\n", val);
		err_val = -EINVAL;
		goto error;
	}

	if (!check_test_in_prog(err_str)) {
		err_val = -ENOEXEC;
		goto error;
	}

	ctx->conf_params.tx_pkt_cw = val;

	return 0;
error:
	return err_val;
}

static int nrf_wifi_radio_test_set_reg_domain(struct nrf_wifi_ctx_lnx *ctx,
					      const char *reg, char *err_str)
{
	enum nrf_wifi_status status = NRF_WIFI_STATUS_FAIL;
	int err_val = -ENOEXEC;
	struct nrf_wifi_fmac_reg_info reg_domain_info = { 0 };

	if (strlen(reg) != 2) {
		snprintf(
			err_str, MAX_ERR_STR_SIZE,
			"Invalid reg domain: Length should be two letters/digits\n");
		goto error;
	}

	/* Two letter country code with special case of 00 for WORLD */
	if (((reg[0] < 'A' || reg[0] > 'Z') ||
	     (reg[1] < 'A' || reg[1] > 'Z')) &&
	    (reg[0] != '0' || reg[1] != '0')) {
		snprintf(err_str, MAX_ERR_STR_SIZE, "Invalid reg domain %c%c\n",
			 reg[1], reg[0]);
		err_val = -ENOEXEC;
		goto error;
	}

	ctx->conf_params.country_code[0] = reg[0];
	ctx->conf_params.country_code[1] = reg[1];

	if (!check_test_in_prog(err_str)) {
		err_val = -ENOEXEC;
		goto error;
	}

	memcpy(reg_domain_info.alpha2, ctx->conf_params.country_code,
	       NRF_WIFI_COUNTRY_CODE_LEN);

	status = nrf_wifi_fmac_set_reg(ctx->rpu_ctx, &reg_domain_info);

	if (status != NRF_WIFI_STATUS_SUCCESS) {
		snprintf(err_str, MAX_ERR_STR_SIZE,
			 "Regulatory programming failed\n");
		err_val = -ENOEXEC;
		goto error;
	}

	return 0;
error:
	return err_val;
}

static int nrf_wifi_radio_test_set_bypass_reg(struct nrf_wifi_ctx_lnx *ctx,
					      unsigned long val, char *err_str)
{
	int err_val = -1;

	if (val > 1) {
		snprintf(err_str, MAX_ERR_STR_SIZE, "Invalid value %lu\n", val);
		err_val = -ENOEXEC;
		goto error;
	}

	if (!check_test_in_prog(err_str)) {
		err_val = -ENOEXEC;
		goto error;
	}

	ctx->conf_params.bypass_regulatory = val;

	return 0;
error:
	return err_val;
}
#endif

void nrf_wifi_lnx_wlan_fmac_conf_init(struct rpu_conf_params *conf_params)
{
	memset(conf_params, 0, sizeof(*conf_params));

	conf_params->he_gi = -1;
	conf_params->power_save = 0;
}

static ssize_t nrf_wifi_wlan_fmac_conf_write(struct file *file,
					     const char __user *in_buf,
					     size_t count, loff_t *ppos)
{
	char *conf_buf = NULL;
	unsigned long val = 0;
#ifdef CONFIG_NRF700X_RADIO_TEST
	long sval = 0;
#endif
	char err_str[MAX_ERR_STR_SIZE];
	ssize_t err_val = count;
	struct nrf_wifi_ctx_lnx *ctx = NULL;
#ifndef CONFIG_NRF700X_RADIO_TEST
	enum nrf_wifi_status status = NRF_WIFI_STATUS_FAIL;
#endif

	ctx = (struct nrf_wifi_ctx_lnx *)file->f_inode->i_private;

	if (count >= MAX_CONF_BUF_SIZE) {
		snprintf(err_str, MAX_ERR_STR_SIZE,
			 "Size of input buffer cannot be more than %d chars\n",
			 MAX_CONF_BUF_SIZE);

		err_val = -EFAULT;
		goto error;
	}

	conf_buf = kzalloc(MAX_CONF_BUF_SIZE, GFP_KERNEL);

	if (!conf_buf) {
		snprintf(err_str, MAX_ERR_STR_SIZE,
			 "Not enough memory available\n");

		err_val = -EFAULT;
		goto error;
	}

	if (copy_from_user(conf_buf, in_buf, count)) {
		snprintf(err_str, MAX_ERR_STR_SIZE,
			 "Copy from input buffer failed\n");

		err_val = -EFAULT;
		goto error;
	}

	conf_buf[count - 1] = '\0';

#ifndef CONFIG_NRF700X_RADIO_TEST
	if (param_get_val(conf_buf, "uapsd_queue=", &val)) {
		if ((val < 0) || (val > 15)) {
			snprintf(err_str, MAX_ERR_STR_SIZE,
				 "Invalid value %lu\n", val);
			err_val = -EINVAL;
			goto error;
		}

		if (ctx->conf_params.uapsd_queue == val)
			goto out;

		status = nrf_wifi_fmac_set_uapsd_queue(ctx->rpu_ctx, 0, val);

		if (status != NRF_WIFI_STATUS_SUCCESS) {
			snprintf(err_str, MAX_ERR_STR_SIZE,
				 "Programming uapsd queue failed\n");
			goto error;
		}
		ctx->conf_params.uapsd_queue = val;
		goto error;
	} else if (param_get_val(conf_buf, "power_save=", &val)) {
		if ((val != 0) && (val != 1)) {
			snprintf(err_str, MAX_ERR_STR_SIZE,
				 "Invalid value %lu\n", val);
			err_val = -EINVAL;
			goto error;
		}

		if (ctx->conf_params.power_save == val)
			goto out;

		status = nrf_wifi_fmac_set_power_save(ctx->rpu_ctx, 0, val);

		if (status != NRF_WIFI_STATUS_SUCCESS) {
			snprintf(err_str, MAX_ERR_STR_SIZE,
				 "Programming power save failed\n");
			goto error;
		}
		ctx->conf_params.power_save = val;
	} else if (param_get_val(conf_buf, "he_gi=", &val)) {
		if ((val < 0) || (val > 2)) {
			snprintf(err_str, MAX_ERR_STR_SIZE,
				 "Invalid value %lu\n", val);
			err_val = -EINVAL;
			goto error;
		}

		if (ctx->conf_params.he_gi == val)
			goto out;
		ctx->conf_params.he_gi = val;
	} else if (param_get_val(conf_buf, "rts_threshold=", &val)) {
		struct nrf_wifi_umac_set_wiphy_info *wiphy_info = NULL;

		if (val <= 0) {
			snprintf(err_str, MAX_ERR_STR_SIZE,
				 "Invalid value %lu\n", val);
			err_val = -EINVAL;
			goto error;
		}

		if (ctx->conf_params.rts_threshold == val)
			goto out;

		wiphy_info = kzalloc(sizeof(*wiphy_info), GFP_KERNEL);

		if (!wiphy_info) {
			snprintf(err_str, MAX_ERR_STR_SIZE,
				 "Unable to allocate memory\n");
			goto error;
		}
		wiphy_info->rts_threshold = val;

		status = nrf_wifi_fmac_set_wiphy_params(ctx->rpu_ctx, 0,
							wiphy_info);

		if (status != NRF_WIFI_STATUS_SUCCESS) {
			snprintf(err_str, MAX_ERR_STR_SIZE,
				 "Programming rts_threshold failed\n");
			goto error;
		}
		if (wiphy_info)
			kfree(wiphy_info);

		ctx->conf_params.rts_threshold = val;
#else
	if (param_get_match(conf_buf, "set_defaults")) {
		nrf_wifi_radio_test_conf_init(&ctx->conf_params);
	} else if (param_get_val(conf_buf, "phy_calib_rxdc=", &val)) {
		if (nrf_wifi_radio_test_set_phy_calib_rxdc(ctx, val, err_str))
			goto error;
	} else if (param_get_val(conf_buf, "phy_calib_txdc=", &val)) {
		if (nrf_wifi_radio_test_set_phy_calib_txdc(ctx, val, err_str))
			goto error;
	} else if (param_get_val(conf_buf, "phy_calib_txpow=", &val)) {
		if (nrf_wifi_radio_test_set_phy_calib_txpow(ctx, val, err_str))
			goto error;
	} else if (param_get_val(conf_buf, "phy_calib_rxiq=", &val)) {
		if (nrf_wifi_radio_test_set_phy_calib_rxiq(ctx, val, err_str))
			goto error;
	} else if (param_get_val(conf_buf, "phy_calib_txiq=", &val)) {
		if (nrf_wifi_radio_test_set_phy_calib_txiq(ctx, val, err_str))
			goto error;
	} else if (param_get_val(conf_buf, "he_ltf=", &val)) {
		if (nrf_wifi_radio_test_set_he_ltf(ctx, val, err_str))
			goto error;
	} else if (param_get_val(conf_buf, "he_gi=", &val)) {
		if (nrf_wifi_radio_test_set_he_gi(ctx, val, err_str))
			goto error;
	} else if (param_get_val(conf_buf, "tx_pkt_tput_mode=", &val)) {
		if (nrf_wifi_radio_test_set_tx_pkt_tput_mode(ctx, val, err_str))
			goto error;
	} else if (param_get_val(conf_buf, "tx_pkt_sgi=", &val)) {
		if (nrf_wifi_radio_test_set_tx_pkt_sgi(ctx, val, err_str))
			goto error;
	} else if (param_get_val(conf_buf, "tx_pkt_preamble=", &val)) {
		if (nrf_wifi_radio_test_set_tx_pkt_preamble(ctx, val, err_str))
			goto error;
	} else if (param_get_sval(conf_buf, "tx_pkt_mcs=", &sval)) {
		if (nrf_wifi_radio_test_set_tx_pkt_mcs(ctx, sval, err_str))
			goto error;
	} else if (param_get_sval(conf_buf, "tx_pkt_rate=", &sval)) {
		if (nrf_wifi_radio_test_set_tx_pkt_rate(ctx, sval, err_str))
			goto error;
	} else if (param_get_val(conf_buf, "tx_pkt_gap=", &val)) {
		if (nrf_wifi_radio_test_set_tx_pkt_gap(ctx, val, err_str))
			goto error;
	} else if (param_get_sval(conf_buf, "tx_pkt_num=", &sval)) {
		if (nrf_wifi_radio_test_set_tx_pkt_num(ctx, sval, err_str))
			goto error;
	} else if (param_get_val(conf_buf, "tx_pkt_len=", &val)) {
		if (nrf_wifi_radio_test_set_tx_pkt_len(ctx, val, err_str))
			goto error;
	} else if (param_get_val(conf_buf, "tx_pkt_len=", &val)) {
		if (nrf_wifi_radio_test_set_tx_pkt_len(ctx, val, err_str))
			goto error;
	} else if (param_get_val(conf_buf, "tx_power=", &val)) {
		if (nrf_wifi_radio_test_set_tx_power(ctx, val, err_str))
			goto error;
	} else if (param_get_val(conf_buf, "ru_tone=", &val)) {
		if (nrf_wifi_radio_test_set_ru_tone(ctx, val, err_str))
			goto error;
	} else if (param_get_val(conf_buf, "ru_index=", &val)) {
		if (nrf_wifi_radio_test_set_ru_index(ctx, val, err_str))
			goto error;
	} else if (param_get_val(conf_buf, "rx_capture_length=", &val)) {
		if (nrf_wifi_radio_test_set_rx_capture_length(ctx, val,
							      err_str))
			goto error;
	} else if (param_get_val(conf_buf, "rx_lna_gain=", &val)) {
		if (nrf_wifi_radio_test_set_rx_lna_gain(ctx, val, err_str))
			goto error;
	} else if (param_get_val(conf_buf, "rx_bb_gain=", &val)) {
		if (nrf_wifi_radio_test_set_rx_bb_gain(ctx, val, err_str))
			goto error;
	} else if (param_get_val(conf_buf, "tx_tone_freq=", &val)) {
		if (nrf_wifi_radio_test_set_tx_tone_freq(ctx, val, err_str))
			goto error;
	} else if (param_get_val(conf_buf, "dpd=", &val)) {
		if (nrf_wifi_radio_set_dpd(ctx, val, err_str))
			goto error;
	} else if (param_get_val(conf_buf, "set_xo_val=", &val)) {
		if (nrf_wifi_radio_set_xo_val(ctx, val, err_str))
			goto error;
	} else if (param_get_val(conf_buf, "init=", &val)) {
		if (nrf_wifi_radio_test_init(ctx, val, err_str))
			goto error;
	} else if (param_get_val(conf_buf, "tx=", &val)) {
		if (nrf_wifi_radio_test_set_tx(ctx, val, err_str))
			goto error;
	} else if (param_get_val(conf_buf, "rx=", &val)) {
		if (nrf_wifi_radio_test_set_rx(ctx, val, err_str))
			goto error;
	} else if (param_get_val(conf_buf, "rx_cap=", &val)) {
		if (nrf_wifi_radio_test_rx_cap(ctx, val, err_str))
			goto error;
	} else if (param_get_val(conf_buf, "tx_tone=", &val)) {
		if (nrf_wifi_radio_test_tx_tone(ctx, val, err_str))
			goto error;
	} else if (param_get_val(conf_buf, "get_temperature=", &val)) {
		if (nrf_wifi_radio_get_temperature(ctx, val, err_str))
			goto error;
	} else if (param_get_val(conf_buf, "get_rf_rssi=", &val)) {
		if (nrf_wifi_radio_get_rf_rssi(ctx, val, err_str))
			goto error;
	} else if (param_get_val(conf_buf, "compute_optimal_xo_val=", &val)) {
		if (nrf_wifi_radio_comp_opt_xo_val(ctx, val, err_str))
			goto error;
#ifdef notyet
	} else if (param_get_val(conf_buf, "get_stats=", &val)) {
		if (nrf_wifi_radio_test_get_stats(ctx, val, err_str))
			goto error;
#endif
	} else if (param_get_val(conf_buf, "tx_pkt_cw=", &val)) {
		if (nrf_wifi_radio_test_set_tx_pkt_cw(ctx, val, err_str))
			goto error;
	} else if (param_get_match(conf_buf, "reg_domain=")) {
		if (strstr(conf_buf, "bypass")) {
			if (param_get_val(conf_buf,
					  "bypass_reg_domain=", &val)) {
				if (nrf_wifi_radio_test_set_bypass_reg(ctx, val,
								       err_str))
					goto error;
			} else {
				snprintf(err_str, MAX_ERR_STR_SIZE,
					 "Invalid parameter value: %s\n",
					 strstr(conf_buf, "=") + 1);
				goto error;
			}
		} else {
			if (nrf_wifi_radio_test_set_reg_domain(
				    ctx, strstr(conf_buf, "=") + 1, err_str))
				goto error;
		}
#endif
		/* CONFIG_NRF700X_RADIO_TEST */
	} else {
		snprintf(err_str, MAX_ERR_STR_SIZE,
			 "Invalid parameter name: %s\n", conf_buf);
		err_val = -EFAULT;
		goto error;
	}

	goto out;

error:
	pr_err("Error condition: %s\n", err_str);
out:
	kfree(conf_buf);

	return err_val;
}

static int nrf_wifi_wlan_fmac_conf_open(struct inode *inode, struct file *file)
{
	ctx = (struct nrf_wifi_ctx_lnx *)inode->i_private;

	return single_open(file, nrf_wifi_wlan_fmac_conf_disp, ctx);
}

static const struct file_operations fops_wlan_conf = {
	.open = nrf_wifi_wlan_fmac_conf_open,
	.read = seq_read,
	.llseek = seq_lseek,
	.write = nrf_wifi_wlan_fmac_conf_write,
	.release = single_release
};

int nrf_wifi_wlan_fmac_dbgfs_conf_init(struct dentry *root,
				       struct nrf_wifi_ctx_lnx *ctx)
{
	int ret = 0;

	if ((!root) || (!ctx)) {
		pr_err("%s: Invalid parameters\n", __func__);
		ret = -EINVAL;
		goto fail;
	}

	ctx->dbgfs_wlan_conf_root =
		debugfs_create_file("conf", 0644, root, ctx, &fops_wlan_conf);

	if (!ctx->dbgfs_wlan_conf_root) {
		pr_err("%s: Failed to create debugfs entry\n", __func__);
		ret = -ENOMEM;
		goto fail;
	}

	goto out;

fail:
	nrf_wifi_wlan_fmac_dbgfs_conf_deinit(ctx);
out:
	return ret;
}

void nrf_wifi_wlan_fmac_dbgfs_conf_deinit(struct nrf_wifi_ctx_lnx *ctx)
{
	if (ctx->dbgfs_wlan_conf_root)
		debugfs_remove(ctx->dbgfs_wlan_conf_root);

	ctx->dbgfs_wlan_conf_root = NULL;
}
