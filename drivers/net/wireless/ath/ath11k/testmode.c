// SPDX-License-Identifier: BSD-3-Clause-Clear
/*
 * Copyright (c) 2018-2019 The Linux Foundation. All rights reserved.
 * Copyright (c) 2023 Qualcomm Innovation Center, Inc. All rights reserved.
 */

#include "testmode.h"
#include <net/netlink.h>
#include "debug.h"
#include "wmi.h"
#include "hw.h"
#include "core.h"
#include "testmode_i.h"

static const struct nla_policy ath11k_tm_policy[ATH11K_TM_ATTR_MAX + 1] = {
	[ATH11K_TM_ATTR_CMD]		= { .type = NLA_U32 },
	[ATH11K_TM_ATTR_DATA]		= { .type = NLA_BINARY,
					    .len = ATH11K_TM_DATA_MAX_LEN },
	[ATH11K_TM_ATTR_WMI_CMDID]	= { .type = NLA_U32 },
	[ATH11K_TM_ATTR_VERSION_MAJOR]	= { .type = NLA_U32 },
	[ATH11K_TM_ATTR_VERSION_MINOR]	= { .type = NLA_U32 },
};

static int ath11k_tm_cmd_get_version(struct ath11k *ar, struct nlattr *tb[])
{
	struct sk_buff *skb;
	int ret;

	ath11k_dbg(ar->ab, ATH11K_DBG_TESTMODE,
		   "testmode cmd get version_major %d version_minor %d\n",
		   ATH11K_TESTMODE_VERSION_MAJOR,
		   ATH11K_TESTMODE_VERSION_MINOR);

	skb = cfg80211_testmode_alloc_reply_skb(ar->hw->wiphy,
						nla_total_size(sizeof(u32)));
	if (!skb)
		return -ENOMEM;

	ret = nla_put_u32(skb, ATH11K_TM_ATTR_VERSION_MAJOR,
			  ATH11K_TESTMODE_VERSION_MAJOR);
	if (ret) {
		kfree_skb(skb);
		return ret;
	}

	ret = nla_put_u32(skb, ATH11K_TM_ATTR_VERSION_MINOR,
			  ATH11K_TESTMODE_VERSION_MINOR);
	if (ret) {
		kfree_skb(skb);
		return ret;
	}

	return cfg80211_testmode_reply(skb);
}

static int ath11k_tm_cmd_wmi(struct ath11k *ar, struct nlattr *tb[])
{
	struct ath11k_pdev_wmi *wmi = ar->wmi;
	struct sk_buff *skb;
	u32 cmd_id, buf_len;
	int ret;
	void *buf;

	mutex_lock(&ar->conf_mutex);

	if (ar->state != ATH11K_STATE_ON) {
		ret = -ENETDOWN;
		goto out;
	}

	if (!tb[ATH11K_TM_ATTR_DATA]) {
		ret = -EINVAL;
		goto out;
	}

	if (!tb[ATH11K_TM_ATTR_WMI_CMDID]) {
		ret = -EINVAL;
		goto out;
	}

	buf = nla_data(tb[ATH11K_TM_ATTR_DATA]);
	buf_len = nla_len(tb[ATH11K_TM_ATTR_DATA]);
	cmd_id = nla_get_u32(tb[ATH11K_TM_ATTR_WMI_CMDID]);

	ath11k_dbg(ar->ab, ATH11K_DBG_TESTMODE,
		   "testmode cmd wmi cmd_id %d buf %pK buf_len %d\n",
		   cmd_id, buf, buf_len);

	ath11k_dbg_dump(ar->ab, ATH11K_DBG_TESTMODE, NULL, "", buf, buf_len);

	skb = ath11k_wmi_alloc_skb(wmi->wmi_ab, buf_len);
	if (!skb) {
		ret = -ENOMEM;
		goto out;
	}

	memcpy(skb->data, buf, buf_len);

	ret = ath11k_wmi_cmd_send(wmi, skb, cmd_id);
	if (ret) {
		dev_kfree_skb(skb);
		ath11k_warn(ar->ab, "failed to transmit wmi command (testmode): %d\n",
			    ret);
		goto out;
	}

	ret = 0;

out:
	mutex_unlock(&ar->conf_mutex);
	return ret;
}

int ath11k_tm_cmd(struct ieee80211_hw *hw, struct ieee80211_vif *vif,
		  void *data, int len)
{
	struct ath11k *ar = hw->priv;
	struct nlattr *tb[ATH11K_TM_ATTR_MAX + 1];
	int ret;

	ret = nla_parse(tb, ATH11K_TM_ATTR_MAX, data, len, ath11k_tm_policy,
			NULL);
	if (ret)
		return ret;

	if (!tb[ATH11K_TM_ATTR_CMD])
		return -EINVAL;

	switch (nla_get_u32(tb[ATH11K_TM_ATTR_CMD])) {
	case ATH11K_TM_CMD_GET_VERSION:
		return ath11k_tm_cmd_get_version(ar, tb);
	case ATH11K_TM_CMD_WMI:
		return ath11k_tm_cmd_wmi(ar, tb);
	default:
		return -EOPNOTSUPP;
	}
}
