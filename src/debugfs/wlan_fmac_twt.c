/*
 * Copyright (c) 2023 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: GPL-2.0
 */

#include <linux/debugfs.h>
#include <linux/etherdevice.h>
#include <linux/fs.h>

#include "fmac_api.h"
#include "fmac_dbgfs_if.h"
#include "net_stack.h"
#include "osal_ops.h"
#include "util.h"

#define MAX_CONF_BUF_SIZE 200
#define MAX_ERR_STR_SIZE 80

int twt_setup_event;

void nrf_wifi_wlan_fmac_twt_init(struct rpu_twt_params *twt_params)
{
	memset(twt_params, 0, sizeof(*twt_params));
	twt_params->teardown_reason = -1;
	/* Initialize values which are other than 0 */
}

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

static __always_inline unsigned char param_get_match(unsigned char *buf,
						     unsigned char *str)
{
	if (strstr(buf, str))
		return 1;
	else
		return 0;
}

static ssize_t nrf_wifi_wlan_twt_write(struct file *file,
				       const char __user *in_buf, size_t count,
				       loff_t *ppos)
{
	enum nrf_wifi_status status = NRF_WIFI_STATUS_FAIL;
	struct nrf_wifi_ctx_lnx *rpu_ctx_lnx = NULL;
	struct nrf_wifi_fmac_dev_ctx *fmac_ctx = NULL;
	struct nrf_wifi_umac_cmd_config_twt *twt_setup_cmd = NULL;
	struct nrf_wifi_umac_cmd_teardown_twt *twt_teardown_cmd = NULL;
	const struct nrf_wifi_osal_ops *osal_ops = NULL;
	char *conf_buf = NULL;
	char err_str[MAX_ERR_STR_SIZE];
	size_t ret_val = count;
	unsigned long val = 0;
	unsigned long start_time_us = 0;
	int twt_retry_attempt = 0;
	char *tok_s = NULL;

	rpu_ctx_lnx = (struct nrf_wifi_ctx_lnx *)file->f_inode->i_private;
	fmac_ctx = (struct nrf_wifi_fmac_dev_ctx *)(rpu_ctx_lnx->rpu_ctx);
	osal_ops = fmac_ctx->fpriv->opriv->ops;

	if (count >= MAX_CONF_BUF_SIZE) {
		snprintf(err_str, MAX_ERR_STR_SIZE,
			 "Size of input buffer cannot be more than %d chars\n",
			 MAX_CONF_BUF_SIZE);

		ret_val = -EFAULT;
		goto error;
	}

	conf_buf = kzalloc(MAX_CONF_BUF_SIZE, GFP_KERNEL);

	if (!conf_buf) {
		snprintf(err_str, MAX_ERR_STR_SIZE,
			 "Not enough memory available\n");

		ret_val = -EFAULT;
		goto error;
	}

	if (copy_from_user(conf_buf, in_buf, count)) {
		snprintf(err_str, MAX_ERR_STR_SIZE,
			 "Copy from input buffer failed\n");

		ret_val = -EFAULT;
		goto error;
	}

	conf_buf[count - 1] = '\0';

	tok_s = strstr(conf_buf, "twt_setup ");

	if (tok_s) {
		memcpy(rpu_ctx_lnx->twt_params.twt_setup_cmd, conf_buf, 200);

		tok_s = strstr(conf_buf, " negotiation=");
		if (tok_s) {
			sscanf(tok_s + strlen(" negotiation="), "%lu", &val);
			rpu_ctx_lnx->twt_params.twt_cmd.neg_type = val;
		}

		tok_s = strstr(conf_buf, " exponent=");
		if (tok_s) {
			sscanf(tok_s + strlen(" exponent="), "%lu", &val);
			rpu_ctx_lnx->twt_params.twt_cmd
				.twt_target_wake_interval_exponent = val;
		}

		tok_s = strstr(conf_buf, " mantissa=");
		if (tok_s) {
			sscanf(tok_s + strlen(" mantissa="), "%lu", &val);
			rpu_ctx_lnx->twt_params.twt_cmd
				.twt_target_wake_interval_mantissa = val;
		}

		tok_s = strstr(conf_buf, " min_twt=");
		if (tok_s) {
			sscanf(tok_s + strlen(" min_twt="), "%lu", &val);
			rpu_ctx_lnx->twt_params.twt_cmd
				.nominal_min_twt_wake_duration = val;
		}

		tok_s = strstr(conf_buf, " setup_cmd=");
		if (tok_s) {
			sscanf(tok_s + strlen(" setup_cmd="), "%lu", &val);
			rpu_ctx_lnx->twt_params.twt_cmd.setup_cmd = val;
		}

		tok_s = strstr(conf_buf, " twt=");
		if (tok_s) {
			unsigned long long twt = 0;
			sscanf(tok_s + strlen(" twt="), "%llu", &twt);
			rpu_ctx_lnx->twt_params.twt_cmd.target_wake_time = twt;
		}

		tok_s = strstr(conf_buf, " trigger=");
		if (tok_s) {
			sscanf(tok_s + strlen(" trigger="), "%lu", &val);
			rpu_ctx_lnx->twt_params.twt_cmd.ap_trigger_frame = val;
		}

		tok_s = strstr(conf_buf, " implicit=");
		if (tok_s) {
			sscanf(tok_s + strlen(" implicit="), "%lu", &val);
			rpu_ctx_lnx->twt_params.twt_cmd.is_implicit = val;
		}

		tok_s = strstr(conf_buf, " flow_type=");
		if (tok_s) {
			sscanf(tok_s + strlen(" flow_type="), "%lu", &val);
			rpu_ctx_lnx->twt_params.twt_cmd.twt_flow_type = val;
		}

		tok_s = strstr(conf_buf, " flow_id=");
		if (tok_s) {
			sscanf(tok_s + strlen(" flow_id="), "%lu", &val);
			rpu_ctx_lnx->twt_params.twt_cmd.twt_flow_id = val;
		}

		twt_setup_cmd = kzalloc(sizeof(*twt_setup_cmd), GFP_KERNEL);

		if (!twt_setup_cmd) {
			pr_err("%s: Unable to allocate memory\n", __func__);
			goto out;
		}
		twt_setup_cmd->umac_hdr.cmd_evnt = NRF_WIFI_UMAC_CMD_CONFIG_TWT;
		twt_setup_cmd->umac_hdr.ids.wdev_id = 0;
		twt_setup_cmd->umac_hdr.ids.valid_fields |=
			NRF_WIFI_INDEX_IDS_WDEV_ID_VALID;

		twt_setup_cmd->info.twt_flow_id =
			rpu_ctx_lnx->twt_params.twt_cmd.twt_flow_id;
		twt_setup_cmd->info.neg_type =
			rpu_ctx_lnx->twt_params.twt_cmd.neg_type;
		twt_setup_cmd->info.setup_cmd =
			rpu_ctx_lnx->twt_params.twt_cmd.setup_cmd;
		twt_setup_cmd->info.ap_trigger_frame =
			rpu_ctx_lnx->twt_params.twt_cmd.ap_trigger_frame;
		twt_setup_cmd->info.is_implicit =
			rpu_ctx_lnx->twt_params.twt_cmd.is_implicit;
		twt_setup_cmd->info.twt_flow_type =
			rpu_ctx_lnx->twt_params.twt_cmd.twt_flow_type;
		twt_setup_cmd->info.twt_target_wake_interval_exponent =
			rpu_ctx_lnx->twt_params.twt_cmd
				.twt_target_wake_interval_exponent;
		twt_setup_cmd->info.twt_target_wake_interval_mantissa =
			rpu_ctx_lnx->twt_params.twt_cmd
				.twt_target_wake_interval_mantissa;
		twt_setup_cmd->info.target_wake_time =
			rpu_ctx_lnx->twt_params.twt_cmd.target_wake_time;
		twt_setup_cmd->info.nominal_min_twt_wake_duration =
			rpu_ctx_lnx->twt_params.twt_cmd
				.nominal_min_twt_wake_duration;
retry_twt:
		twt_setup_event = 0;
		status = umac_cmd_cfg(fmac_ctx, twt_setup_cmd,
				      sizeof(*twt_setup_cmd));

		if (status != NRF_WIFI_STATUS_SUCCESS) {
			pr_err("TWT SETUP_CMD failed\n");
			goto out;
		}

		start_time_us = osal_ops->time_get_curr_us();

		while (!twt_setup_event) {
#define MAX_TWT_WAIT (1 * 1000 * 1000)
			if (osal_ops->time_elapsed_us(start_time_us) >=
			    MAX_TWT_WAIT)
				break;
		}

		if (!twt_setup_event) {
			// osal_ops->log_err("%s: TWT SETUP timed out attempt=%d\n",
			//       __func__, twt_retry_attempt);
			if (++twt_retry_attempt != 2)
				goto retry_twt;
			// else
			//   osal_ops->log_err("%s: TWT SETUP timed out\n", __func__);
		}
		if (twt_setup_cmd)
			kfree(twt_setup_cmd);

		goto out;
	}

	tok_s = strstr(conf_buf, "twt_teardown");
	if (tok_s) {
		tok_s = strstr(conf_buf, " flow_id=");
		if (tok_s) {
			param_get_val(tok_s, "flow_id=", &val);
			if (val ==
			    rpu_ctx_lnx->twt_params.twt_cmd.twt_flow_id) {
				twt_teardown_cmd = kzalloc(
					sizeof(*twt_teardown_cmd), GFP_KERNEL);
				if (!twt_teardown_cmd) {
					pr_err("%s: Unable to allocate memory\n",
					       __func__);
					goto out;
				}
				twt_teardown_cmd->umac_hdr.cmd_evnt =
					NRF_WIFI_UMAC_CMD_TEARDOWN_TWT;
				twt_teardown_cmd->umac_hdr.ids.wdev_id = 0;
				twt_teardown_cmd->umac_hdr.ids.valid_fields |=
					NRF_WIFI_INDEX_IDS_WDEV_ID_VALID;

				status =
					umac_cmd_cfg(fmac_ctx, twt_teardown_cmd,
						     sizeof(*twt_teardown_cmd));
				if (twt_teardown_cmd)
					kfree(twt_teardown_cmd);

				goto out;
			}
		}
	}
error:
	pr_err("Error condition: %s\n", err_str);

out:
	if (conf_buf)
		kfree(conf_buf);

	return ret_val;
}

static int nrf_wifi_wlan_fmac_dbgfs_twt_show(struct seq_file *m, void *v)
{
	struct nrf_wifi_ctx_lnx *rpu_ctx_lnx = NULL;

	rpu_ctx_lnx = (struct nrf_wifi_ctx_lnx *)m->private;

	return 0;
}

static int nrf_wifi_wlan_twt_open(struct inode *inode, struct file *file)
{
	struct nrf_wifi_ctx_lnx *rpu_ctx_lnx =
		(struct nrf_wifi_ctx_lnx *)inode->i_private;

	return single_open(file, nrf_wifi_wlan_fmac_dbgfs_twt_show,
			   rpu_ctx_lnx);
}

static const struct file_operations fops_wlan_twt = {
	.open = nrf_wifi_wlan_twt_open,
	.read = seq_read,
	.llseek = seq_lseek,
	.write = nrf_wifi_wlan_twt_write,
	.release = single_release
};

int nrf_wifi_wlan_fmac_dbgfs_twt_init(struct dentry *root,
				      struct nrf_wifi_ctx_lnx *rpu_ctx_lnx)
{
	int ret = 0;

	nrf_wifi_wlan_fmac_twt_init(&rpu_ctx_lnx->twt_params);

	if ((!root) || (!rpu_ctx_lnx)) {
		pr_err("%s: Invalid parameters\n", __func__);
		ret = -EINVAL;
		goto fail;
	}

	rpu_ctx_lnx->dbgfs_nrf_wifi_twt_root = debugfs_create_file(
		"twt", 0644, root, rpu_ctx_lnx, &fops_wlan_twt);

	if (!rpu_ctx_lnx->dbgfs_nrf_wifi_twt_root) {
		pr_err("%s: Failed to create debugfs entry\n", __func__);
		ret = -ENOMEM;
		goto fail;
	}

	goto out;

fail:
	nrf_wifi_wlan_fmac_dbgfs_twt_deinit(rpu_ctx_lnx);
out:
	return ret;
}

void nrf_wifi_wlan_fmac_dbgfs_twt_deinit(struct nrf_wifi_ctx_lnx *rpu_ctx_lnx)
{
	if (rpu_ctx_lnx->dbgfs_nrf_wifi_twt_root)
		debugfs_remove(rpu_ctx_lnx->dbgfs_nrf_wifi_twt_root);

	rpu_ctx_lnx->dbgfs_nrf_wifi_twt_root = NULL;
}
