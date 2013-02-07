/*
 * VHT handling
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 */

#include <linux/ieee80211.h>
#include <linux/export.h>
#include <net/mac80211.h>
#include "ieee80211_i.h"


void ieee80211_vht_cap_ie_to_sta_vht_cap(struct ieee80211_sub_if_data *sdata,
					 struct ieee80211_supported_band *sband,
					 struct ieee80211_vht_cap *vht_cap_ie,
					 struct sta_info *sta)
{
	struct ieee80211_sta_vht_cap *vht_cap = &sta->sta.vht_cap;

	memset(vht_cap, 0, sizeof(*vht_cap));

	if (!sta->sta.ht_cap.ht_supported)
		return;

	if (!vht_cap_ie || !sband->vht_cap.vht_supported)
		return;

	/* A VHT STA must support 40 MHz */
	if (!(sta->sta.ht_cap.cap & IEEE80211_HT_CAP_SUP_WIDTH_20_40))
		return;

	vht_cap->vht_supported = true;

	vht_cap->cap = le32_to_cpu(vht_cap_ie->vht_cap_info);

	/* Copy peer MCS info, the driver might need them. */
	memcpy(&vht_cap->vht_mcs, &vht_cap_ie->supp_mcs,
	       sizeof(struct ieee80211_vht_mcs_info));

	sta->sta.bandwidth = ieee80211_sta_cur_vht_bw(sta);
}

enum ieee80211_sta_rx_bandwidth ieee80211_sta_cur_vht_bw(struct sta_info *sta)
{
	struct ieee80211_sub_if_data *sdata = sta->sdata;
	u32 cap = sta->sta.vht_cap.cap;

	if (!sta->sta.vht_cap.vht_supported)
		return sta->sta.ht_cap.cap & IEEE80211_HT_CAP_SUP_WIDTH_20_40 ?
				IEEE80211_STA_RX_BW_40 : IEEE80211_STA_RX_BW_20;

	/* TODO: handle VHT opmode notification data */

	switch (sdata->vif.bss_conf.chandef.width) {
	default:
		WARN_ON_ONCE(1);
		/* fall through */
	case NL80211_CHAN_WIDTH_20_NOHT:
	case NL80211_CHAN_WIDTH_20:
	case NL80211_CHAN_WIDTH_40:
		return sta->sta.ht_cap.cap & IEEE80211_HT_CAP_SUP_WIDTH_20_40 ?
				IEEE80211_STA_RX_BW_40 : IEEE80211_STA_RX_BW_20;
	case NL80211_CHAN_WIDTH_160:
		if (cap & IEEE80211_VHT_CAP_SUPP_CHAN_WIDTH_160MHZ)
			return IEEE80211_STA_RX_BW_160;
		/* fall through */
	case NL80211_CHAN_WIDTH_80P80:
		if (cap & IEEE80211_VHT_CAP_SUPP_CHAN_WIDTH_160_80PLUS80MHZ)
			return IEEE80211_STA_RX_BW_160;
		/* fall through */
	case NL80211_CHAN_WIDTH_80:
		return IEEE80211_STA_RX_BW_80;
	}
}
