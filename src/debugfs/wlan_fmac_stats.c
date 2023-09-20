/*
 * Copyright (c) 2023 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: GPL-2.0
 */

#include <linux/debugfs.h>

#include "fmac_api.h"
#include "fmac_dbgfs_if.h"

#ifndef CONFIG_NRF700X_RADIO_TEST
static void wifi_nrf_wlan_fmac_dbgfs_stats_show_host(
	struct seq_file *m, struct wifi_nrf_fmac_dev_ctx *fmac_dev_ctx,
	struct rpu_host_stats *stats)
{
#ifdef DEBUG_MODE_SUPPORT
	int i = 0;
	unsigned int cnt = 0;
#endif /* DEBUG_MODE_SUPPORT */

	seq_puts(m, "************* HOST STATS ***********\n");
	seq_printf(m, "total_tx_pkts =%llu\n", stats->total_tx_pkts);
	seq_printf(m, "total_tx_done_pkts = %llu\n", stats->total_tx_done_pkts);
	seq_printf(m, "total_rx_pkts = %llu\n", stats->total_rx_pkts);
#ifdef DEBUG_MODE_SUPPORT

	for (i = 0; i < fmac_dev_ctx->fpriv->data_config.max_tx_aggregation;
	     i++) {
		cnt = stats->tx_coalesce_frames[i];
		if (cnt != 0)
			seq_printf(m, "tx_coalesece_frames[%d] = %d\n", (i + 1),
				   cnt);
	}

	seq_puts(m, "\n\n");

	for (i = 0; i < fmac_dev_ctx->fpriv->data_config.max_tx_aggregation;
	     i++) {
		cnt = stats->tx_done_coalesce_frames[i];
		if (cnt != 0)
			seq_printf(m, "tx_done_coalesece_frames[%d] = %d\n",
				   (i + 1), cnt);
	}
	seq_printf(m, "total_isr_cnts = %u\n", stats->num_isrs);
	seq_printf(m, "total_event_pop_cnt = %u\n", stats->num_events);
	seq_printf(m, "total_event_resubmit_cnt_true = %u\n",
		   stats->num_events_resubmit);
	seq_printf(m, "total_event_resubmit_cnt_false = %u\n",
		   (stats->num_events - stats->num_events_resubmit));

#endif /* DEBUG_MODE_SUPPORT */
}

static void
wifi_nrf_wlan_fmac_dbgfs_stats_show_umac(struct seq_file *m,
					 struct rpu_umac_stats *stats)
{
	seq_puts(m, "**************** UMAC STATS ****************\n");
	seq_printf(m, "tx_cmd = %d\n", stats->tx_dbg_params.tx_cmd);

	seq_printf(m, "tx_non_coalesce_pkts_rcvd_from_host = %d\n",
		   stats->tx_dbg_params.tx_non_coalesce_pkts_rcvd_from_host);

	seq_printf(m, "tx_coalesce_pkts_rcvd_from_host = %d\n",
		   stats->tx_dbg_params.tx_coalesce_pkts_rcvd_from_host);

	seq_printf(m, "tx_coalesce_pkts_rcvd_from_host = %d\n",
		   stats->tx_dbg_params.tx_max_coalesce_pkts_rcvd_from_host);

	seq_printf(m, "tx_cmds_max_used = %d\n",
		   stats->tx_dbg_params.tx_cmds_max_used);

	seq_printf(m, "tx_cmds_currently_in_use = %d\n",
		   stats->tx_dbg_params.tx_cmds_currently_in_use);

	seq_printf(m, "tx_done_events_send_to_host = %d\n",
		   stats->tx_dbg_params.tx_done_events_send_to_host);

	seq_printf(m, "tx_done_success_pkts_to_host = %d\n",
		   stats->tx_dbg_params.tx_done_success_pkts_to_host);

	seq_printf(m, "tx_done_failure_pkts_to_host = %d\n",
		   stats->tx_dbg_params.tx_done_failure_pkts_to_host);

	seq_printf(
		m, "tx_cmds_with_crypto_pkts_rcvd_from_host = %d\n",
		stats->tx_dbg_params.tx_cmds_with_crypto_pkts_rcvd_from_host);

	seq_printf(m, "tx_cmds_with_non_cryptot_pkts_rcvd_from_host = %d\n",
		   stats->tx_dbg_params
			   .tx_cmds_with_non_crypto_pkts_rcvd_from_host);

	seq_printf(
		m, "tx_cmds_with_broadcast_pkts_rcvd_from_host = %d\n",
		stats->tx_dbg_params.tx_cmds_with_broadcast_pkts_rcvd_from_host);

	seq_printf(
		m, "tx_cmds_with_multicast_pkts_rcvd_from_host = %d\n",
		stats->tx_dbg_params.tx_cmds_with_multicast_pkts_rcvd_from_host);

	seq_printf(
		m, "tx_cmds_with_unicast_pkts_rcvd_from_host = %d\n",
		stats->tx_dbg_params.tx_cmds_with_unicast_pkts_rcvd_from_host);

	seq_printf(m, "xmit = %d\n", stats->tx_dbg_params.xmit);

	seq_printf(m, "send_addba_req = %d\n",
		   stats->tx_dbg_params.send_addba_req);

	seq_printf(m, "addba_resp = %d\n", stats->tx_dbg_params.addba_resp);

	seq_printf(m, "softmac_tx = %d\n", stats->tx_dbg_params.softmac_tx);

	seq_printf(m, "internal_pkts = %d\n",
		   stats->tx_dbg_params.internal_pkts);

	seq_printf(m, "external_pkts = %d\n",
		   stats->tx_dbg_params.external_pkts);

	seq_printf(m, "tx_cmds_to_lmac = %d\n",
		   stats->tx_dbg_params.tx_cmds_to_lmac);

	seq_printf(m, "tx_dones_from_lmac = %d\n",
		   stats->tx_dbg_params.tx_dones_from_lmac);

	seq_printf(m, "total_cmds_to_lmac = %d\n",
		   stats->tx_dbg_params.total_cmds_to_lmac);

	seq_printf(m, "tx_packet_data_count = %d\n",
		   stats->tx_dbg_params.tx_packet_data_count);

	seq_printf(m, "tx_packet_mgmt_count = %d\n",
		   stats->tx_dbg_params.tx_packet_mgmt_count);

	seq_printf(m, "tx_packet_beacon_count = %d\n",
		   stats->tx_dbg_params.tx_packet_beacon_count);

	seq_printf(m, "tx_packet_probe_req_count = %d\n",
		   stats->tx_dbg_params.tx_packet_probe_req_count);

	seq_printf(m, "tx_packet_auth_count = %d\n",
		   stats->tx_dbg_params.tx_packet_auth_count);

	seq_printf(m, "tx_packet_deauth_count = %d\n",
		   stats->tx_dbg_params.tx_packet_deauth_count);

	seq_printf(m, "tx_packet_assoc_req_count = %d\n",
		   stats->tx_dbg_params.tx_packet_assoc_req_count);

	seq_printf(m, "tx_packet_disassoc_count = %d\n",
		   stats->tx_dbg_params.tx_packet_disassoc_count);

	seq_printf(m, "tx_packet_action_count = %d\n",
		   stats->tx_dbg_params.tx_packet_action_count);

	seq_printf(m, "tx_packet_other_mgmt_count = %d\n",
		   stats->tx_dbg_params.tx_packet_other_mgmt_count);

	seq_printf(m, "tx_packet_non_mgmt_data_count = %d\n",
		   stats->tx_dbg_params.tx_packet_non_mgmt_data_count);

	seq_printf(m, "lmac_events = %d\n", stats->rx_dbg_params.lmac_events);

	seq_printf(m, "rx_events = %d\n", stats->rx_dbg_params.rx_events);

	seq_printf(m, "rx_coalised_events = %d\n",
		   stats->rx_dbg_params.rx_coalesce_events);

	seq_printf(m, "total_rx_pkts_from_lmac = %d\n",
		   stats->rx_dbg_params.total_rx_pkts_from_lmac);

	seq_printf(m, "max_refill_gap = %d\n",
		   stats->rx_dbg_params.max_refill_gap);

	seq_printf(m, "current_refill_gap = %d\n",
		   stats->rx_dbg_params.current_refill_gap);

	seq_printf(m, "out_oforedr_mpdus = %d\n",
		   stats->rx_dbg_params.out_of_order_mpdus);

	seq_printf(m, "reoredr_free_mpdus = %d\n",
		   stats->rx_dbg_params.reorder_free_mpdus);

	seq_printf(m, "umac_consumed_pkts = %d\n",
		   stats->rx_dbg_params.umac_consumed_pkts);

	seq_printf(m, "host_consumed_pkts = %d\n",
		   stats->rx_dbg_params.host_consumed_pkts);

	seq_printf(m, "rx_mbox_post = %d\n", stats->rx_dbg_params.rx_mbox_post);

	seq_printf(m, "rx_mbox_receive = %d\n",
		   stats->rx_dbg_params.rx_mbox_receive);

	seq_printf(m, "reordering_ampdu = %d\n",
		   stats->rx_dbg_params.reordering_ampdu);

	seq_printf(m, "timer_mbox_post = %d\n",
		   stats->rx_dbg_params.timer_mbox_post);

	seq_printf(m, "timer_mbox_rcv = %d\n",
		   stats->rx_dbg_params.timer_mbox_rcv);

	seq_printf(m, "work_mbox_post = %d\n",
		   stats->rx_dbg_params.work_mbox_post);

	seq_printf(m, "work_mbox_rcv = %d\n",
		   stats->rx_dbg_params.work_mbox_rcv);

	seq_printf(m, "tasklet_mbox_post = %d\n",
		   stats->rx_dbg_params.tasklet_mbox_post);

	seq_printf(m, "tasklet_mbox_rcv = %d\n",
		   stats->rx_dbg_params.tasklet_mbox_rcv);

	seq_printf(m, "userspace_offload_frames = %d\n",
		   stats->rx_dbg_params.userspace_offload_frames);

	seq_printf(m, "alloc_buf_fail = %d\n",
		   stats->rx_dbg_params.alloc_buf_fail);

	seq_printf(m, "rx_packet_total_count = %d\n",
		   stats->rx_dbg_params.rx_packet_total_count);

	seq_printf(m, "rx_packet_data_count = %d\n",
		   stats->rx_dbg_params.rx_packet_data_count);

	seq_printf(m, "rx_packet_qos_data_count = %d\n",
		   stats->rx_dbg_params.rx_packet_qos_data_count);

	seq_printf(m, "rx_packet_protected_data_count = %d\n",
		   stats->rx_dbg_params.rx_packet_protected_data_count);

	seq_printf(m, "rx_packet_mgmt_count = %d\n",
		   stats->rx_dbg_params.rx_packet_mgmt_count);

	seq_printf(m, "rx_packet_beacon_count = %d\n",
		   stats->rx_dbg_params.rx_packet_beacon_count);

	seq_printf(m, "rx_packet_probe_resp_count = %d\n",
		   stats->rx_dbg_params.rx_packet_probe_resp_count);

	seq_printf(m, "rx_packet_auth_count = %d\n",
		   stats->rx_dbg_params.rx_packet_auth_count);

	seq_printf(m, "rx_packet_deauth_count = %d\n",
		   stats->rx_dbg_params.rx_packet_deauth_count);

	seq_printf(m, "rx_packet_assoc_resp_count = %d\n",
		   stats->rx_dbg_params.rx_packet_assoc_resp_count);

	seq_printf(m, "rx_packet_disassoc_count = %d\n",
		   stats->rx_dbg_params.rx_packet_disassoc_count);

	seq_printf(m, "rx_packet_action_count = %d\n",
		   stats->rx_dbg_params.rx_packet_action_count);

	seq_printf(m, "rx_packet_probe_req_count = %d\n",
		   stats->rx_dbg_params.rx_packet_probe_req_count);

	seq_printf(m, "rx_packet_other_mgmt_count = %d\n",
		   stats->rx_dbg_params.rx_packet_other_mgmt_count);

	seq_printf(m, "max_coalised_pkts = %d\n",
		   stats->rx_dbg_params.max_coalesce_pkts);

	seq_printf(m, "null_skb_pointer_from_lmac = %d\n",
		   stats->rx_dbg_params.null_skb_pointer_from_lmac);

	seq_printf(m, "unexpected_mgmt_pk = %d\n",
		   stats->rx_dbg_params.unexpected_mgmt_pkt);

	seq_printf(m, "cmd_init = %d\n", stats->cmd_evnt_dbg_params.cmd_init);

	seq_printf(m, "event_init_done = %d\n",
		   stats->cmd_evnt_dbg_params.event_init_done);

	seq_printf(m, "cmd_get_stats = %d\n",
		   stats->cmd_evnt_dbg_params.cmd_get_stats);

	seq_printf(m, "event_ps_state = %d\n",
		   stats->cmd_evnt_dbg_params.event_ps_state);

	seq_printf(m, "cmd_trigger_scan = %d\n",
		   stats->cmd_evnt_dbg_params.cmd_trigger_scan);

	seq_printf(m, "event_scan_done = %d\n",
		   stats->cmd_evnt_dbg_params.event_scan_done);

	seq_printf(m, "cmd_get_scan = %d\n",
		   stats->cmd_evnt_dbg_params.cmd_get_scan);

	seq_printf(m, "cmd_auth = %d\n", stats->cmd_evnt_dbg_params.cmd_auth);

	seq_printf(m, "cmd_assoc = %d\n", stats->cmd_evnt_dbg_params.cmd_assoc);

	seq_printf(m, "cmd_connect = %d\n",
		   stats->cmd_evnt_dbg_params.cmd_connect);

	seq_printf(m, "cmd_deauth = %d\n",
		   stats->cmd_evnt_dbg_params.cmd_deauth);

	seq_printf(m, "cmd_register_frame = %d\n",
		   stats->cmd_evnt_dbg_params.cmd_register_frame);

	seq_printf(m, "cmd_frame = %d\n", stats->cmd_evnt_dbg_params.cmd_frame);

	seq_printf(m, "cmd_del_key = %d\n",
		   stats->cmd_evnt_dbg_params.cmd_del_key);

	seq_printf(m, "cmd_new_key = %d\n",
		   stats->cmd_evnt_dbg_params.cmd_new_key);

	seq_printf(m, "cmd_set_key = %d\n",
		   stats->cmd_evnt_dbg_params.cmd_set_key);

	seq_printf(m, "cmd_set_station = %d\n",
		   stats->cmd_evnt_dbg_params.cmd_set_station);

	seq_printf(m, "cmd_del_station = %d\n",
		   stats->cmd_evnt_dbg_params.cmd_del_station);

	seq_printf(m, "cmd_new_interface = %d\n",
		   stats->cmd_evnt_dbg_params.cmd_new_interface);

	seq_printf(m, "cmd_set_interface = %d\n",
		   stats->cmd_evnt_dbg_params.cmd_set_interface);

	seq_printf(m, "cmd_set_ifflags = %d\n",
		   stats->cmd_evnt_dbg_params.cmd_set_ifflags);

	seq_printf(m, "cmd_set_ifflags_done = %d\n",
		   stats->cmd_evnt_dbg_params.cmd_set_ifflags_done);

	seq_printf(m, "cmd_set_bss = %d\n",
		   stats->cmd_evnt_dbg_params.cmd_set_bss);

	seq_printf(m, "cmd_set_wiphy = %d\n",
		   stats->cmd_evnt_dbg_params.cmd_set_wiphy);

	seq_printf(m, "cmd_start_ap = %d\n",
		   stats->cmd_evnt_dbg_params.cmd_start_ap);

	seq_printf(m, "cmd_rf_test = %d\n",
		   stats->cmd_evnt_dbg_params.cmd_rf_test);

	seq_printf(m, "LMAC_CMD_PS = %d\n",
		   stats->cmd_evnt_dbg_params.LMAC_CMD_PS);

	seq_printf(m, "CURR_STATE = %d\n",
		   stats->cmd_evnt_dbg_params.CURR_STATE);
}

static void
wifi_nrf_wlan_fmac_dbgfs_stats_show_lmac(struct seq_file *m,
					 struct rpu_lmac_stats *stats)
{
	seq_puts(m, "************* LMAC STATS ***********\n");

	seq_printf(m, "resetCmdCnt =%d\n", stats->reset_cmd_cnt);

	seq_printf(m, "resetCompleteEventCnt =%d\n",
		   stats->reset_complete_event_cnt);

	seq_printf(m, "unableGenEvent =%d\n", stats->unable_gen_event);

	seq_printf(m, "chProgCmdCnt =%d\n", stats->ch_prog_cmd_cnt);

	seq_printf(m, "channelProgDone =%d\n", stats->channel_prog_done);

	seq_printf(m, "txPktCnt =%d\n", stats->tx_pkt_cnt);

	seq_printf(m, "txPktDoneCnt =%d\n", stats->tx_pkt_done_cnt);

	seq_printf(m, "scanPktCnt =%d\n", stats->scan_pkt_cnt);

	seq_printf(m, "internalPktCnt =%d\n", stats->internal_pkt_cnt);

	seq_printf(m, "internalPktDoneCnt =%d\n", stats->internal_pkt_done_cnt);

	seq_printf(m, "ackRespCnt =%d\n", stats->ack_resp_cnt);

	seq_printf(m, "txTimeout =%d\n", stats->tx_timeout);

	seq_printf(m, "deaggIsr =%d\n", stats->deagg_isr);

	seq_printf(m, "deaggInptrDescEmpty =%d\n",
		   stats->deagg_inptr_desc_empty);

	seq_printf(m, "deaggCircularBufferFull =%d\n",
		   stats->deagg_circular_buffer_full);

	seq_printf(m, "lmacRxisrCnt =%d\n", stats->lmac_rxisr_cnt);

	seq_printf(m, "rxDecryptcnt =%d\n", stats->rx_decryptcnt);

	seq_printf(m, "processDecryptFail =%d\n", stats->process_decrypt_fail);

	seq_printf(m, "prepaRxEventFail =%d\n", stats->prepa_rx_event_fail);

	seq_printf(m, "rxCorePoolFullCnt =%d\n", stats->rx_core_pool_full_cnt);

	seq_printf(m, "rxMpduCrcSuccessCnt =%d\n",
		   stats->rx_mpdu_crc_success_cnt);

	seq_printf(m, "rxMpduCrcFailCnt =%d\n", stats->rx_mpdu_crc_fail_cnt);

	seq_printf(m, "rxOfdmCrcSuccessCnt =%d\n",
		   stats->rx_ofdm_crc_success_cnt);

	seq_printf(m, "rxOfdmCrcFailCnt =%d\n", stats->rx_ofdm_crc_fail_cnt);

	seq_printf(m, "rxDSSSCrcSuccessCnt =%d\n", stats->rxDSSSCrcSuccessCnt);

	seq_printf(m, "rxDSSSCrcFailCnt =%d\n", stats->rxDSSSCrcFailCnt);

	seq_printf(m, "rxCryptoStartCnt =%d\n", stats->rx_crypto_start_cnt);

	seq_printf(m, "rxCryptoDoneCnt =%d\n", stats->rx_crypto_done_cnt);

	seq_printf(m, "rxEventBufFull =%d\n", stats->rx_event_buf_full);

	seq_printf(m, "rxExtramBufFull =%d\n", stats->rx_extram_buf_full);

	seq_printf(m, "scanReq =%d\n", stats->scan_req);

	seq_printf(m, "scanComplete =%d\n", stats->scan_complete);

	seq_printf(m, "scanAbortReq =%d\n", stats->scan_abort_req);

	seq_printf(m, "scanAbortComplete =%d\n", stats->scan_abort_complete);

	seq_printf(m, "internalBufPoolNull =%d\n",
		   stats->internal_buf_pool_null);

#ifdef DEBUG_MODE_SUPPORT
#endif /* DEBUG_MODE_SUPPORT */
}
#endif /* !CONFIG_NRF700X_RADIO_TEST */

static void wifi_nrf_wlan_fmac_dbgfs_stats_show_phy(struct seq_file *m,
#ifdef CONFIG_NRF700X_RADIO_TEST
						    int op_mode,
#endif /* CONFIG_NRF700X_RADIO_TEST */
						    struct rpu_phy_stats *stats)
{
	seq_puts(m, "************* PHY STATS ***********\n");

	seq_printf(m, "rssi_avg = %d dBm\n", stats->rssi_avg);

#ifdef CONFIG_NRF700X_RADIO_TEST
	if (op_mode == RPU_OP_MODE_FCM)
		seq_printf(m, "pdout_val = %d\n", stats->pdout_val);
#endif /* CONFIG_NRF700X_RADIO_TEST */

	seq_printf(m, "ofdm_crc32_pass_cnt=%d\n", stats->ofdm_crc32_pass_cnt);

	seq_printf(m, "ofdm_crc32_fail_cnt=%d\n", stats->ofdm_crc32_fail_cnt);

	seq_printf(m, "dsss_crc32_pass_cnt=%d\n", stats->dsss_crc32_pass_cnt);

	seq_printf(m, "dsss_crc32_fail_cnt=%d\n", stats->dsss_crc32_fail_cnt);
}

static int wifi_nrf_wlan_fmac_dbgfs_stats_show(struct seq_file *m, void *v)
{
	enum wifi_nrf_status status = WIFI_NRF_STATUS_FAIL;
	struct wifi_nrf_ctx_lnx *rpu_ctx_lnx = NULL;
#ifdef DEBUG_MODE_SUPPORT
	int stats_type = RPU_STATS_TYPE_ALL;
#endif /* DEBUG_MODE_SUPPORT */
	int op_mode = RPU_OP_MODE_MAX;
	struct rpu_op_stats *stats = NULL;

	rpu_ctx_lnx = (struct wifi_nrf_ctx_lnx *)m->private;

	stats = kzalloc(sizeof(*stats), GFP_KERNEL);

	status = wifi_nrf_fmac_stats_get(rpu_ctx_lnx->rpu_ctx,
#ifdef DEBUG_MODE_SUPPORT
					 stats_type,
#endif /* DEBUG_MODE_SUPPORT */
					 op_mode, stats);

	if (status != WIFI_NRF_STATUS_SUCCESS) {
		pr_err("%s: wifi_nrf_fmac_stats_get failed\n", __func__);
		goto out;
	}

#ifndef CONFIG_NRF700X_RADIO_TEST
#ifdef DEBUG_MODE_SUPPORT
	if ((stats_type == RPU_STATS_TYPE_ALL) ||
	    (stats_type == RPU_STATS_TYPE_HOST))
#endif /* DEBUG_MODE_SUPPORT */
		wifi_nrf_wlan_fmac_dbgfs_stats_show_host(
			m, rpu_ctx_lnx->rpu_ctx, &stats->host);

#ifdef DEBUG_MODE_SUPPORT
	if ((stats_type == RPU_STATS_TYPE_ALL) ||
	    (stats_type == RPU_STATS_TYPE_UMAC))
#endif /* DEBUG_MODE_SUPPORT */
		wifi_nrf_wlan_fmac_dbgfs_stats_show_umac(m, &stats->fw.umac);

#ifdef DEBUG_MODE_SUPPORT
	if ((stats_type == RPU_STATS_TYPE_ALL) ||
	    (stats_type == RPU_STATS_TYPE_LMAC))
#endif /* DEBUG_MODE_SUPPORT */
		wifi_nrf_wlan_fmac_dbgfs_stats_show_lmac(m, &stats->fw.lmac);
#endif /* !CONFIG_NRF700X_RADIO_TEST */

#ifdef DEBUG_MODE_SUPPORT
	if ((stats_type == RPU_STATS_TYPE_ALL) ||
	    (stats_type == RPU_STATS_TYPE_PHY))
#endif /* DEBUG_MODE_SUPPORT */
		wifi_nrf_wlan_fmac_dbgfs_stats_show_phy(m,
#ifdef CONFIG_NRF700X_RADIO_TEST
							op_mode,
#endif /* CONFIG_NRF700X_RADIO_TEST */
							&stats->fw.phy);
	switch (rpu_ctx_lnx->twt_params.status) {
	case 0:
		seq_printf(m, "sta_status = DISCONNECTED\n");
		break;
	case 1:
		seq_printf(m, "sta_status = CONNECTED\n");
		break;
	default:
		break;
	}

	seq_puts(m, "************* TWT CMD INFO ***********\n");
	seq_printf(m, "twt_flow_id = %d\n",
		   rpu_ctx_lnx->twt_params.twt_cmd.twt_flow_id);
	seq_printf(m, "neg_type = %d\n",
		   rpu_ctx_lnx->twt_params.twt_cmd.neg_type);
	seq_printf(m, "setup_cmd = %d\n",
		   rpu_ctx_lnx->twt_params.twt_cmd.setup_cmd);
	seq_printf(m, "ap_trigger_frame = %d\n",
		   rpu_ctx_lnx->twt_params.twt_cmd.ap_trigger_frame);
	seq_printf(m, "is_implicit = %d\n",
		   rpu_ctx_lnx->twt_params.twt_cmd.is_implicit);
	seq_printf(m, "setup_cmd  = %d\n",
		   rpu_ctx_lnx->twt_params.twt_cmd.setup_cmd);
	seq_printf(m, "twt_flow_type = %d\n",
		   rpu_ctx_lnx->twt_params.twt_cmd.twt_flow_type);
	seq_printf(m, "twt_target_wake_interval_exponent  = %d\n",
		   rpu_ctx_lnx->twt_params.twt_cmd
			   .twt_target_wake_interval_exponent);
	seq_printf(m, "twt_target_wake_interval_mantissa  = %d\n",
		   rpu_ctx_lnx->twt_params.twt_cmd
			   .twt_target_wake_interval_mantissa);
	seq_printf(
		m, "twt_nominal_min_twt_wake_duration  = %d\n",
		rpu_ctx_lnx->twt_params.twt_cmd.nominal_min_twt_wake_duration);
	seq_printf(m, "target_wake_time  = %llu\n",
		   rpu_ctx_lnx->twt_params.twt_cmd.target_wake_time);

	seq_puts(m, "\n************* TWT RESPONSE EVENT INFO ***********\n");
	if (rpu_ctx_lnx->twt_params.twt_event_info_avail) {
		seq_printf(m, "twt_flow_id = %d\n",
			   rpu_ctx_lnx->twt_params.twt_event.twt_flow_id);
		seq_printf(m, "neg_type = %d\n",
			   rpu_ctx_lnx->twt_params.twt_event.neg_type);
		seq_printf(m, "setup_cmd = %d\n",
			   rpu_ctx_lnx->twt_params.twt_event.setup_cmd);
		seq_printf(m, "ap_trigger_frame = %d\n",
			   rpu_ctx_lnx->twt_params.twt_event.ap_trigger_frame);
		seq_printf(m, "is_implicit = %d\n",
			   rpu_ctx_lnx->twt_params.twt_event.is_implicit);
		seq_printf(m, "setup_cmd  = %d\n",
			   rpu_ctx_lnx->twt_params.twt_event.setup_cmd);
		seq_printf(m, "twt_flow_type = %d\n",
			   rpu_ctx_lnx->twt_params.twt_event.twt_flow_type);
		seq_printf(m, "twt_target_wake_interval_exponent  = %d\n",
			   rpu_ctx_lnx->twt_params.twt_event
				   .twt_target_wake_interval_exponent);
		seq_printf(m, "twt_target_wake_interval_mantissa  = %d\n",
			   rpu_ctx_lnx->twt_params.twt_event
				   .twt_target_wake_interval_mantissa);
		seq_printf(m, "twt_nominal_min_twt_wake_duration  = %d\n",
			   rpu_ctx_lnx->twt_params.twt_event
				   .nominal_min_twt_wake_duration);
		seq_printf(m, "target_wake_time  = %llu\n",
			   rpu_ctx_lnx->twt_params.twt_event.target_wake_time);
	}
	seq_puts(m, "\n************* TWT TEAR DOWN EVENT INFO ***********\n");
	switch (rpu_ctx_lnx->twt_params.teardown_reason) {
	case 0:
		seq_printf(m, "twt_teardown_reason = INVALID_TSF\n");
		break;
	case 1:
		seq_printf(m, "twt_teardown_reason = OUT_OF_SYNC\n");
		break;
	case 2:
		seq_printf(
			m,
			"twt_teardown_reason = DELAY_IN_TWT_UMAC_SLEEP_RESPONSE\n");
		break;
	case 3:
		seq_printf(
			m,
			"twt_teardown_reason  = INVALID_TWT_WAKE_INTERVAL\n");
		break;
	default:
		break;
	}
	seq_printf(m, "teardown_event_cnt = %d\n",
		   rpu_ctx_lnx->twt_params.teardown_event_cnt);

out:
	return status;
}

static int open_stats(struct inode *inode, struct file *file)
{
	struct wifi_nrf_ctx_lnx *rpu_ctx_lnx =
		(struct wifi_nrf_ctx_lnx *)inode->i_private;

	return single_open(file, wifi_nrf_wlan_fmac_dbgfs_stats_show,
			   rpu_ctx_lnx);
}

static const struct file_operations fops_wlan_fmac_stats = {
	.open = open_stats,
	.read = seq_read,
	.llseek = seq_lseek,
	.write = NULL,
	.release = single_release
};

int wifi_nrf_wlan_fmac_dbgfs_stats_init(struct dentry *root,
					struct wifi_nrf_ctx_lnx *rpu_ctx_lnx)
{
	int ret = 0;

	if ((!root) || (!rpu_ctx_lnx)) {
		pr_err("%s: Invalid parameters\n", __func__);
		ret = -EINVAL;
		goto fail;
	}

	rpu_ctx_lnx->dbgfs_wlan_stats_root = debugfs_create_file(
		"stats", 0444, root, rpu_ctx_lnx, &fops_wlan_fmac_stats);

	if (!rpu_ctx_lnx->dbgfs_wlan_stats_root) {
		pr_err("%s: Failed to create debugfs entry\n", __func__);
		ret = -ENOMEM;
		goto fail;
	}

	goto out;

fail:
	wifi_nrf_wlan_fmac_dbgfs_stats_deinit(rpu_ctx_lnx);

out:
	return ret;
}

void wifi_nrf_wlan_fmac_dbgfs_stats_deinit(struct wifi_nrf_ctx_lnx *rpu_ctx_lnx)
{
	if (rpu_ctx_lnx->dbgfs_wlan_stats_root)
		debugfs_remove(rpu_ctx_lnx->dbgfs_wlan_stats_root);

	rpu_ctx_lnx->dbgfs_wlan_stats_root = NULL;
}
