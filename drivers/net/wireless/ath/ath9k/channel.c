/*
 * Copyright (c) 2014 Qualcomm Atheros, Inc.
 *
 * Permission to use, copy, modify, and/or distribute this software for any
 * purpose with or without fee is hereby granted, provided that the above
 * copyright notice and this permission notice appear in all copies.
 *
 * THE SOFTWARE IS PROVIDED "AS IS" AND THE AUTHOR DISCLAIMS ALL WARRANTIES
 * WITH REGARD TO THIS SOFTWARE INCLUDING ALL IMPLIED WARRANTIES OF
 * MERCHANTABILITY AND FITNESS. IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR
 * ANY SPECIAL, DIRECT, INDIRECT, OR CONSEQUENTIAL DAMAGES OR ANY DAMAGES
 * WHATSOEVER RESULTING FROM LOSS OF USE, DATA OR PROFITS, WHETHER IN AN
 * ACTION OF CONTRACT, NEGLIGENCE OR OTHER TORTIOUS ACTION, ARISING OUT OF
 * OR IN CONNECTION WITH THE USE OR PERFORMANCE OF THIS SOFTWARE.
 */

#include "ath9k.h"

/* Set/change channels.  If the channel is really being changed, it's done
 * by reseting the chip.  To accomplish this we must first cleanup any pending
 * DMA, then restart stuff.
 */
static int ath_set_channel(struct ath_softc *sc)
{
	struct ath_hw *ah = sc->sc_ah;
	struct ath_common *common = ath9k_hw_common(ah);
	struct ieee80211_hw *hw = sc->hw;
	struct ath9k_channel *hchan;
	struct cfg80211_chan_def *chandef = &sc->cur_chan->chandef;
	struct ieee80211_channel *chan = chandef->chan;
	int pos = chan->hw_value;
	int old_pos = -1;
	int r;

	if (test_bit(ATH_OP_INVALID, &common->op_flags))
		return -EIO;

	if (ah->curchan)
		old_pos = ah->curchan - &ah->channels[0];

	ath_dbg(common, CONFIG, "Set channel: %d MHz width: %d\n",
		chan->center_freq, chandef->width);

	/* update survey stats for the old channel before switching */
	spin_lock_bh(&common->cc_lock);
	ath_update_survey_stats(sc);
	spin_unlock_bh(&common->cc_lock);

	ath9k_cmn_get_channel(hw, ah, chandef);

	/* If the operating channel changes, change the survey in-use flags
	 * along with it.
	 * Reset the survey data for the new channel, unless we're switching
	 * back to the operating channel from an off-channel operation.
	 */
	if (!sc->cur_chan->offchannel && sc->cur_survey != &sc->survey[pos]) {
		if (sc->cur_survey)
			sc->cur_survey->filled &= ~SURVEY_INFO_IN_USE;

		sc->cur_survey = &sc->survey[pos];

		memset(sc->cur_survey, 0, sizeof(struct survey_info));
		sc->cur_survey->filled |= SURVEY_INFO_IN_USE;
	} else if (!(sc->survey[pos].filled & SURVEY_INFO_IN_USE)) {
		memset(&sc->survey[pos], 0, sizeof(struct survey_info));
	}

	hchan = &sc->sc_ah->channels[pos];
	r = ath_reset_internal(sc, hchan);
	if (r)
		return r;

	/* The most recent snapshot of channel->noisefloor for the old
	 * channel is only available after the hardware reset. Copy it to
	 * the survey stats now.
	 */
	if (old_pos >= 0)
		ath_update_survey_nf(sc, old_pos);

	/* Enable radar pulse detection if on a DFS channel. Spectral
	 * scanning and radar detection can not be used concurrently.
	 */
	if (hw->conf.radar_enabled) {
		u32 rxfilter;

		/* set HW specific DFS configuration */
		ath9k_hw_set_radar_params(ah);
		rxfilter = ath9k_hw_getrxfilter(ah);
		rxfilter |= ATH9K_RX_FILTER_PHYRADAR |
				ATH9K_RX_FILTER_PHYERR;
		ath9k_hw_setrxfilter(ah, rxfilter);
		ath_dbg(common, DFS, "DFS enabled at freq %d\n",
			chan->center_freq);
	} else {
		/* perform spectral scan if requested. */
		if (test_bit(ATH_OP_SCANNING, &common->op_flags) &&
			sc->spectral_mode == SPECTRAL_CHANSCAN)
			ath9k_spectral_scan_trigger(hw);
	}

	return 0;
}

void ath_chanctx_init(struct ath_softc *sc)
{
	struct ath_chanctx *ctx;
	struct ath_common *common = ath9k_hw_common(sc->sc_ah);
	struct ieee80211_supported_band *sband;
	struct ieee80211_channel *chan;
	int i, j;

	sband = &common->sbands[IEEE80211_BAND_2GHZ];
	if (!sband->n_channels)
		sband = &common->sbands[IEEE80211_BAND_5GHZ];

	chan = &sband->channels[0];
	for (i = 0; i < ATH9K_NUM_CHANCTX; i++) {
		ctx = &sc->chanctx[i];
		cfg80211_chandef_create(&ctx->chandef, chan, NL80211_CHAN_HT20);
		INIT_LIST_HEAD(&ctx->vifs);
		ctx->txpower = ATH_TXPOWER_MAX;
		for (j = 0; j < ARRAY_SIZE(ctx->acq); j++)
			INIT_LIST_HEAD(&ctx->acq[j]);
	}
}

void ath_chanctx_set_channel(struct ath_softc *sc, struct ath_chanctx *ctx,
			     struct cfg80211_chan_def *chandef)
{
	bool cur_chan;

	spin_lock_bh(&sc->chan_lock);
	if (chandef)
		memcpy(&ctx->chandef, chandef, sizeof(*chandef));
	cur_chan = sc->cur_chan == ctx;
	spin_unlock_bh(&sc->chan_lock);

	if (!cur_chan)
		return;

	ath_set_channel(sc);
}

#ifdef CONFIG_ATH9K_CHANNEL_CONTEXT

/**********************************************************/
/* Functions to handle the channel context state machine. */
/**********************************************************/

static const char *offchannel_state_string(enum ath_offchannel_state state)
{
#define case_rtn_string(val) case val: return #val

	switch (state) {
		case_rtn_string(ATH_OFFCHANNEL_IDLE);
		case_rtn_string(ATH_OFFCHANNEL_PROBE_SEND);
		case_rtn_string(ATH_OFFCHANNEL_PROBE_WAIT);
		case_rtn_string(ATH_OFFCHANNEL_SUSPEND);
		case_rtn_string(ATH_OFFCHANNEL_ROC_START);
		case_rtn_string(ATH_OFFCHANNEL_ROC_WAIT);
		case_rtn_string(ATH_OFFCHANNEL_ROC_DONE);
	default:
		return "unknown";
	}
}

void ath_chanctx_check_active(struct ath_softc *sc, struct ath_chanctx *ctx)
{
	struct ath_common *common = ath9k_hw_common(sc->sc_ah);
	struct ath_vif *avp;
	bool active = false;
	u8 n_active = 0;

	if (!ctx)
		return;

	list_for_each_entry(avp, &ctx->vifs, list) {
		struct ieee80211_vif *vif = avp->vif;

		switch (vif->type) {
		case NL80211_IFTYPE_P2P_CLIENT:
		case NL80211_IFTYPE_STATION:
			if (vif->bss_conf.assoc)
				active = true;
			break;
		default:
			active = true;
			break;
		}
	}
	ctx->active = active;

	ath_for_each_chanctx(sc, ctx) {
		if (!ctx->assigned || list_empty(&ctx->vifs))
			continue;
		n_active++;
	}

	if (n_active <= 1) {
		clear_bit(ATH_OP_MULTI_CHANNEL, &common->op_flags);
		return;
	}
	if (test_and_set_bit(ATH_OP_MULTI_CHANNEL, &common->op_flags))
		return;

	if (ath9k_is_chanctx_enabled()) {
		ath_chanctx_event(sc, NULL,
				  ATH_CHANCTX_EVENT_ENABLE_MULTICHANNEL);
	}
}

static struct ath_chanctx *
ath_chanctx_get_next(struct ath_softc *sc, struct ath_chanctx *ctx)
{
	int idx = ctx - &sc->chanctx[0];

	return &sc->chanctx[!idx];
}

static void ath_chanctx_adjust_tbtt_delta(struct ath_softc *sc)
{
	struct ath_chanctx *prev, *cur;
	struct timespec ts;
	u32 cur_tsf, prev_tsf, beacon_int;
	s32 offset;

	beacon_int = TU_TO_USEC(sc->cur_chan->beacon.beacon_interval);

	cur = sc->cur_chan;
	prev = ath_chanctx_get_next(sc, cur);

	getrawmonotonic(&ts);
	cur_tsf = (u32) cur->tsf_val +
		  ath9k_hw_get_tsf_offset(&cur->tsf_ts, &ts);

	prev_tsf = prev->last_beacon - (u32) prev->tsf_val + cur_tsf;
	prev_tsf -= ath9k_hw_get_tsf_offset(&prev->tsf_ts, &ts);

	/* Adjust the TSF time of the AP chanctx to keep its beacons
	 * at half beacon interval offset relative to the STA chanctx.
	 */
	offset = cur_tsf - prev_tsf;

	/* Ignore stale data or spurious timestamps */
	if (offset < 0 || offset > 3 * beacon_int)
		return;

	offset = beacon_int / 2 - (offset % beacon_int);
	prev->tsf_val += offset;
}

/* Configure the TSF based hardware timer for a channel switch.
 * Also set up backup software timer, in case the gen timer fails.
 * This could be caused by a hardware reset.
 */
static void ath_chanctx_setup_timer(struct ath_softc *sc, u32 tsf_time)
{
	struct ath_hw *ah = sc->sc_ah;

#ifdef CONFIG_ATH9K_CHANNEL_CONTEXT
	ath9k_hw_gen_timer_start(ah, sc->p2p_ps_timer, tsf_time, 1000000);
#endif
	tsf_time -= ath9k_hw_gettsf32(ah);
	tsf_time = msecs_to_jiffies(tsf_time / 1000) + 1;
	mod_timer(&sc->sched.timer, tsf_time);
}

void ath_chanctx_event(struct ath_softc *sc, struct ieee80211_vif *vif,
		       enum ath_chanctx_event ev)
{
	struct ath_hw *ah = sc->sc_ah;
	struct ath_common *common = ath9k_hw_common(ah);
	struct ath_beacon_config *cur_conf;
	struct ath_vif *avp = NULL;
	struct ath_chanctx *ctx;
	u32 tsf_time;
	u32 beacon_int;
	bool noa_changed = false;

	if (vif)
		avp = (struct ath_vif *) vif->drv_priv;

	spin_lock_bh(&sc->chan_lock);

	switch (ev) {
	case ATH_CHANCTX_EVENT_BEACON_PREPARE:
		if (avp->offchannel_duration)
			avp->offchannel_duration = 0;

		if (avp->chanctx != sc->cur_chan)
			break;

		if (sc->sched.offchannel_pending) {
			sc->sched.offchannel_pending = false;
			sc->next_chan = &sc->offchannel.chan;
			sc->sched.state = ATH_CHANCTX_STATE_WAIT_FOR_BEACON;
		}

		ctx = ath_chanctx_get_next(sc, sc->cur_chan);
		if (ctx->active && sc->sched.state == ATH_CHANCTX_STATE_IDLE) {
			sc->next_chan = ctx;
			sc->sched.state = ATH_CHANCTX_STATE_WAIT_FOR_BEACON;
		}

		/* if the timer missed its window, use the next interval */
		if (sc->sched.state == ATH_CHANCTX_STATE_WAIT_FOR_TIMER)
			sc->sched.state = ATH_CHANCTX_STATE_WAIT_FOR_BEACON;

		if (sc->sched.state != ATH_CHANCTX_STATE_WAIT_FOR_BEACON)
			break;

		sc->sched.beacon_pending = true;
		sc->sched.next_tbtt = REG_READ(ah, AR_NEXT_TBTT_TIMER);

		cur_conf = &sc->cur_chan->beacon;
		beacon_int = TU_TO_USEC(cur_conf->beacon_interval);

		/* defer channel switch by a quarter beacon interval */
		tsf_time = sc->sched.next_tbtt + beacon_int / 4;
		sc->sched.switch_start_time = tsf_time;
		sc->cur_chan->last_beacon = sc->sched.next_tbtt;

		/* Prevent wrap-around issues */
		if (avp->periodic_noa_duration &&
		    tsf_time - avp->periodic_noa_start > BIT(30))
			avp->periodic_noa_duration = 0;

		if (ctx->active && !avp->periodic_noa_duration) {
			avp->periodic_noa_start = tsf_time;
			avp->periodic_noa_duration =
				TU_TO_USEC(cur_conf->beacon_interval) / 2 -
				sc->sched.channel_switch_time;
			noa_changed = true;
		} else if (!ctx->active && avp->periodic_noa_duration) {
			avp->periodic_noa_duration = 0;
			noa_changed = true;
		}

		/* If at least two consecutive beacons were missed on the STA
		 * chanctx, stay on the STA channel for one extra beacon period,
		 * to resync the timer properly.
		 */
		if (ctx->active && sc->sched.beacon_miss >= 2)
			sc->sched.offchannel_duration = 3 * beacon_int / 2;

		if (sc->sched.offchannel_duration) {
			noa_changed = true;
			avp->offchannel_start = tsf_time;
			avp->offchannel_duration =
				sc->sched.offchannel_duration;
		}

		if (noa_changed)
			avp->noa_index++;
		break;
	case ATH_CHANCTX_EVENT_BEACON_SENT:
		if (!sc->sched.beacon_pending)
			break;

		sc->sched.beacon_pending = false;
		if (sc->sched.state != ATH_CHANCTX_STATE_WAIT_FOR_BEACON)
			break;

		sc->sched.state = ATH_CHANCTX_STATE_WAIT_FOR_TIMER;
		ath_chanctx_setup_timer(sc, sc->sched.switch_start_time);
		break;
	case ATH_CHANCTX_EVENT_TSF_TIMER:
		if (sc->sched.state != ATH_CHANCTX_STATE_WAIT_FOR_TIMER)
			break;

		if (!sc->cur_chan->switch_after_beacon &&
		    sc->sched.beacon_pending)
			sc->sched.beacon_miss++;

		sc->sched.state = ATH_CHANCTX_STATE_SWITCH;
		ieee80211_queue_work(sc->hw, &sc->chanctx_work);
		break;
	case ATH_CHANCTX_EVENT_BEACON_RECEIVED:
		if (!test_bit(ATH_OP_MULTI_CHANNEL, &common->op_flags) ||
		    sc->cur_chan == &sc->offchannel.chan)
			break;

		ath_chanctx_adjust_tbtt_delta(sc);
		sc->sched.beacon_pending = false;
		sc->sched.beacon_miss = 0;

		/* TSF time might have been updated by the incoming beacon,
		 * need update the channel switch timer to reflect the change.
		 */
		tsf_time = sc->sched.switch_start_time;
		tsf_time -= (u32) sc->cur_chan->tsf_val +
			ath9k_hw_get_tsf_offset(&sc->cur_chan->tsf_ts, NULL);
		tsf_time += ath9k_hw_gettsf32(ah);


		ath_chanctx_setup_timer(sc, tsf_time);
		break;
	case ATH_CHANCTX_EVENT_ASSOC:
		if (sc->sched.state != ATH_CHANCTX_STATE_FORCE_ACTIVE ||
		    avp->chanctx != sc->cur_chan)
			break;

		sc->sched.state = ATH_CHANCTX_STATE_IDLE;
		/* fall through */
	case ATH_CHANCTX_EVENT_SWITCH:
		if (!test_bit(ATH_OP_MULTI_CHANNEL, &common->op_flags) ||
		    sc->sched.state == ATH_CHANCTX_STATE_FORCE_ACTIVE ||
		    sc->cur_chan->switch_after_beacon ||
		    sc->cur_chan == &sc->offchannel.chan)
			break;

		/* If this is a station chanctx, stay active for a half
		 * beacon period (minus channel switch time)
		 */
		sc->next_chan = ath_chanctx_get_next(sc, sc->cur_chan);
		cur_conf = &sc->cur_chan->beacon;

		sc->sched.state = ATH_CHANCTX_STATE_WAIT_FOR_TIMER;

		tsf_time = TU_TO_USEC(cur_conf->beacon_interval) / 2;
		if (sc->sched.beacon_miss >= 2) {
			sc->sched.beacon_miss = 0;
			tsf_time *= 3;
		}

		tsf_time -= sc->sched.channel_switch_time;
		tsf_time += ath9k_hw_gettsf32(sc->sc_ah);
		sc->sched.switch_start_time = tsf_time;

		ath_chanctx_setup_timer(sc, tsf_time);
		sc->sched.beacon_pending = true;
		break;
	case ATH_CHANCTX_EVENT_ENABLE_MULTICHANNEL:
		if (sc->cur_chan == &sc->offchannel.chan ||
		    sc->cur_chan->switch_after_beacon)
			break;

		sc->next_chan = ath_chanctx_get_next(sc, sc->cur_chan);
		ieee80211_queue_work(sc->hw, &sc->chanctx_work);
		break;
	case ATH_CHANCTX_EVENT_UNASSIGN:
		if (sc->cur_chan->assigned) {
			if (sc->next_chan && !sc->next_chan->assigned &&
			    sc->next_chan != &sc->offchannel.chan)
				sc->sched.state = ATH_CHANCTX_STATE_IDLE;
			break;
		}

		ctx = ath_chanctx_get_next(sc, sc->cur_chan);
		sc->sched.state = ATH_CHANCTX_STATE_IDLE;
		if (!ctx->assigned)
			break;

		sc->next_chan = ctx;
		ieee80211_queue_work(sc->hw, &sc->chanctx_work);
		break;
	}

	spin_unlock_bh(&sc->chan_lock);
}

void ath_chanctx_beacon_sent_ev(struct ath_softc *sc,
				enum ath_chanctx_event ev)
{
	if (sc->sched.beacon_pending)
		ath_chanctx_event(sc, NULL, ev);
}

void ath_chanctx_beacon_recv_ev(struct ath_softc *sc, u32 ts,
				enum ath_chanctx_event ev)
{
	sc->sched.next_tbtt = ts;
	ath_chanctx_event(sc, NULL, ev);
}

static int ath_scan_channel_duration(struct ath_softc *sc,
				     struct ieee80211_channel *chan)
{
	struct cfg80211_scan_request *req = sc->offchannel.scan_req;

	if (!req->n_ssids || (chan->flags & IEEE80211_CHAN_NO_IR))
		return (HZ / 9); /* ~110 ms */

	return (HZ / 16); /* ~60 ms */
}

static void ath_chanctx_switch(struct ath_softc *sc, struct ath_chanctx *ctx,
			       struct cfg80211_chan_def *chandef)
{
	struct ath_common *common = ath9k_hw_common(sc->sc_ah);

	spin_lock_bh(&sc->chan_lock);

	if (test_bit(ATH_OP_MULTI_CHANNEL, &common->op_flags) &&
	    (sc->cur_chan != ctx) && (ctx == &sc->offchannel.chan)) {
		sc->sched.offchannel_pending = true;
		spin_unlock_bh(&sc->chan_lock);
		return;
	}

	sc->next_chan = ctx;
	if (chandef) {
		ctx->chandef = *chandef;
		ath_dbg(common, CHAN_CTX,
			"Assigned next_chan to %d MHz\n", chandef->center_freq1);
	}

	if (sc->next_chan == &sc->offchannel.chan) {
		sc->sched.offchannel_duration =
			TU_TO_USEC(sc->offchannel.duration) +
			sc->sched.channel_switch_time;

		if (chandef) {
			ath_dbg(common, CHAN_CTX,
				"Offchannel duration for chan %d MHz : %u\n",
				chandef->center_freq1,
				sc->sched.offchannel_duration);
		}
	}
	spin_unlock_bh(&sc->chan_lock);
	ieee80211_queue_work(sc->hw, &sc->chanctx_work);
}

static void ath_chanctx_offchan_switch(struct ath_softc *sc,
				       struct ieee80211_channel *chan)
{
	struct ath_common *common = ath9k_hw_common(sc->sc_ah);
	struct cfg80211_chan_def chandef;

	cfg80211_chandef_create(&chandef, chan, NL80211_CHAN_NO_HT);
	ath_dbg(common, CHAN_CTX,
		"Channel definition created: %d MHz\n", chandef.center_freq1);

	ath_chanctx_switch(sc, &sc->offchannel.chan, &chandef);
}

static struct ath_chanctx *ath_chanctx_get_oper_chan(struct ath_softc *sc,
						     bool active)
{
	struct ath_chanctx *ctx;

	ath_for_each_chanctx(sc, ctx) {
		if (!ctx->assigned || list_empty(&ctx->vifs))
			continue;
		if (active && !ctx->active)
			continue;

		if (ctx->switch_after_beacon)
			return ctx;
	}

	return &sc->chanctx[0];
}

static void
ath_scan_next_channel(struct ath_softc *sc)
{
	struct ath_common *common = ath9k_hw_common(sc->sc_ah);
	struct cfg80211_scan_request *req = sc->offchannel.scan_req;
	struct ieee80211_channel *chan;

	if (sc->offchannel.scan_idx >= req->n_channels) {
		ath_dbg(common, CHAN_CTX,
			"Moving to ATH_OFFCHANNEL_IDLE state, scan_idx: %d, n_channels: %d\n",
			sc->offchannel.scan_idx,
			req->n_channels);

		sc->offchannel.state = ATH_OFFCHANNEL_IDLE;
		ath_chanctx_switch(sc, ath_chanctx_get_oper_chan(sc, false),
				   NULL);
		return;
	}

	ath_dbg(common, CHAN_CTX,
		"Moving to ATH_OFFCHANNEL_PROBE_SEND state, scan_idx: %d\n",
		sc->offchannel.scan_idx);

	chan = req->channels[sc->offchannel.scan_idx++];
	sc->offchannel.duration = ath_scan_channel_duration(sc, chan);
	sc->offchannel.state = ATH_OFFCHANNEL_PROBE_SEND;

	ath_chanctx_offchan_switch(sc, chan);
}

void ath_offchannel_next(struct ath_softc *sc)
{
	struct ieee80211_vif *vif;

	if (sc->offchannel.scan_req) {
		vif = sc->offchannel.scan_vif;
		sc->offchannel.chan.txpower = vif->bss_conf.txpower;
		ath_scan_next_channel(sc);
	} else if (sc->offchannel.roc_vif) {
		vif = sc->offchannel.roc_vif;
		sc->offchannel.chan.txpower = vif->bss_conf.txpower;
		sc->offchannel.duration = sc->offchannel.roc_duration;
		sc->offchannel.state = ATH_OFFCHANNEL_ROC_START;
		ath_chanctx_offchan_switch(sc, sc->offchannel.roc_chan);
	} else {
		ath_chanctx_switch(sc, ath_chanctx_get_oper_chan(sc, false),
				   NULL);
		sc->offchannel.state = ATH_OFFCHANNEL_IDLE;
		if (sc->ps_idle)
			ath_cancel_work(sc);
	}
}

void ath_roc_complete(struct ath_softc *sc, bool abort)
{
	sc->offchannel.roc_vif = NULL;
	sc->offchannel.roc_chan = NULL;
	if (!abort)
		ieee80211_remain_on_channel_expired(sc->hw);
	ath_offchannel_next(sc);
	ath9k_ps_restore(sc);
}

void ath_scan_complete(struct ath_softc *sc, bool abort)
{
	struct ath_common *common = ath9k_hw_common(sc->sc_ah);

	if (abort)
		ath_dbg(common, CHAN_CTX, "HW scan aborted\n");
	else
		ath_dbg(common, CHAN_CTX, "HW scan complete\n");

	sc->offchannel.scan_req = NULL;
	sc->offchannel.scan_vif = NULL;
	sc->offchannel.state = ATH_OFFCHANNEL_IDLE;
	ieee80211_scan_completed(sc->hw, abort);
	clear_bit(ATH_OP_SCANNING, &common->op_flags);
	ath_offchannel_next(sc);
	ath9k_ps_restore(sc);
}

static void ath_scan_send_probe(struct ath_softc *sc,
				struct cfg80211_ssid *ssid)
{
	struct cfg80211_scan_request *req = sc->offchannel.scan_req;
	struct ieee80211_vif *vif = sc->offchannel.scan_vif;
	struct ath_tx_control txctl = {};
	struct sk_buff *skb;
	struct ieee80211_tx_info *info;
	int band = sc->offchannel.chan.chandef.chan->band;

	skb = ieee80211_probereq_get(sc->hw, vif,
			ssid->ssid, ssid->ssid_len, req->ie_len);
	if (!skb)
		return;

	info = IEEE80211_SKB_CB(skb);
	if (req->no_cck)
		info->flags |= IEEE80211_TX_CTL_NO_CCK_RATE;

	if (req->ie_len)
		memcpy(skb_put(skb, req->ie_len), req->ie, req->ie_len);

	skb_set_queue_mapping(skb, IEEE80211_AC_VO);

	if (!ieee80211_tx_prepare_skb(sc->hw, vif, skb, band, NULL))
		goto error;

	txctl.txq = sc->tx.txq_map[IEEE80211_AC_VO];
	txctl.force_channel = true;
	if (ath_tx_start(sc->hw, skb, &txctl))
		goto error;

	return;

error:
	ieee80211_free_txskb(sc->hw, skb);
}

static void ath_scan_channel_start(struct ath_softc *sc)
{
	struct ath_common *common = ath9k_hw_common(sc->sc_ah);
	struct cfg80211_scan_request *req = sc->offchannel.scan_req;
	int i;

	if (!(sc->cur_chan->chandef.chan->flags & IEEE80211_CHAN_NO_IR) &&
	    req->n_ssids) {
		for (i = 0; i < req->n_ssids; i++)
			ath_scan_send_probe(sc, &req->ssids[i]);

	}

	ath_dbg(common, CHAN_CTX,
		"Moving to ATH_OFFCHANNEL_PROBE_WAIT state\n");

	sc->offchannel.state = ATH_OFFCHANNEL_PROBE_WAIT;
	mod_timer(&sc->offchannel.timer, jiffies + sc->offchannel.duration);
}

static void ath_chanctx_timer(unsigned long data)
{
	struct ath_softc *sc = (struct ath_softc *) data;

	ath_chanctx_event(sc, NULL, ATH_CHANCTX_EVENT_TSF_TIMER);
}

static void ath_offchannel_timer(unsigned long data)
{
	struct ath_softc *sc = (struct ath_softc *)data;
	struct ath_chanctx *ctx;
	struct ath_common *common = ath9k_hw_common(sc->sc_ah);

	ath_dbg(common, CHAN_CTX, "%s: state: %s\n",
		__func__, offchannel_state_string(sc->offchannel.state));

	switch (sc->offchannel.state) {
	case ATH_OFFCHANNEL_PROBE_WAIT:
		if (!sc->offchannel.scan_req)
			return;

		/* get first active channel context */
		ctx = ath_chanctx_get_oper_chan(sc, true);
		if (ctx->active) {
			sc->offchannel.state = ATH_OFFCHANNEL_SUSPEND;
			ath_chanctx_switch(sc, ctx, NULL);
			mod_timer(&sc->offchannel.timer, jiffies + HZ / 10);
			break;
		}
		/* fall through */
	case ATH_OFFCHANNEL_SUSPEND:
		if (!sc->offchannel.scan_req)
			return;

		ath_scan_next_channel(sc);
		break;
	case ATH_OFFCHANNEL_ROC_START:
	case ATH_OFFCHANNEL_ROC_WAIT:
		ctx = ath_chanctx_get_oper_chan(sc, false);
		sc->offchannel.state = ATH_OFFCHANNEL_ROC_DONE;
		ath_chanctx_switch(sc, ctx, NULL);
		break;
	default:
		break;
	}
}

static bool
ath_chanctx_send_vif_ps_frame(struct ath_softc *sc, struct ath_vif *avp,
			      bool powersave)
{
	struct ieee80211_vif *vif = avp->vif;
	struct ieee80211_sta *sta = NULL;
	struct ieee80211_hdr_3addr *nullfunc;
	struct ath_tx_control txctl;
	struct sk_buff *skb;
	int band = sc->cur_chan->chandef.chan->band;

	switch (vif->type) {
	case NL80211_IFTYPE_STATION:
		if (!vif->bss_conf.assoc)
			return false;

		skb = ieee80211_nullfunc_get(sc->hw, vif);
		if (!skb)
			return false;

		nullfunc = (struct ieee80211_hdr_3addr *) skb->data;
		if (powersave)
			nullfunc->frame_control |=
				cpu_to_le16(IEEE80211_FCTL_PM);

		skb_set_queue_mapping(skb, IEEE80211_AC_VO);
		if (!ieee80211_tx_prepare_skb(sc->hw, vif, skb, band, &sta)) {
			dev_kfree_skb_any(skb);
			return false;
		}
		break;
	default:
		return false;
	}

	memset(&txctl, 0, sizeof(txctl));
	txctl.txq = sc->tx.txq_map[IEEE80211_AC_VO];
	txctl.sta = sta;
	txctl.force_channel = true;
	if (ath_tx_start(sc->hw, skb, &txctl)) {
		ieee80211_free_txskb(sc->hw, skb);
		return false;
	}

	return true;
}

static bool
ath_chanctx_send_ps_frame(struct ath_softc *sc, bool powersave)
{
	struct ath_vif *avp;
	bool sent = false;

	rcu_read_lock();
	list_for_each_entry(avp, &sc->cur_chan->vifs, list) {
		if (ath_chanctx_send_vif_ps_frame(sc, avp, powersave))
			sent = true;
	}
	rcu_read_unlock();

	return sent;
}

static bool ath_chanctx_defer_switch(struct ath_softc *sc)
{
	if (sc->cur_chan == &sc->offchannel.chan)
		return false;

	switch (sc->sched.state) {
	case ATH_CHANCTX_STATE_SWITCH:
		return false;
	case ATH_CHANCTX_STATE_IDLE:
		if (!sc->cur_chan->switch_after_beacon)
			return false;

		sc->sched.state = ATH_CHANCTX_STATE_WAIT_FOR_BEACON;
		break;
	default:
		break;
	}

	return true;
}

static void ath_offchannel_channel_change(struct ath_softc *sc)
{
	struct ath_common *common = ath9k_hw_common(sc->sc_ah);

	ath_dbg(common, CHAN_CTX, "%s: state: %s\n",
		__func__, offchannel_state_string(sc->offchannel.state));

	switch (sc->offchannel.state) {
	case ATH_OFFCHANNEL_PROBE_SEND:
		if (!sc->offchannel.scan_req)
			return;

		if (sc->cur_chan->chandef.chan !=
		    sc->offchannel.chan.chandef.chan)
			return;

		ath_scan_channel_start(sc);
		break;
	case ATH_OFFCHANNEL_IDLE:
		if (!sc->offchannel.scan_req)
			return;

		ath_scan_complete(sc, false);
		break;
	case ATH_OFFCHANNEL_ROC_START:
		if (sc->cur_chan != &sc->offchannel.chan)
			break;

		sc->offchannel.state = ATH_OFFCHANNEL_ROC_WAIT;
		mod_timer(&sc->offchannel.timer, jiffies +
			  msecs_to_jiffies(sc->offchannel.duration));
		ieee80211_ready_on_channel(sc->hw);
		break;
	case ATH_OFFCHANNEL_ROC_DONE:
		ath_roc_complete(sc, false);
		break;
	default:
		break;
	}
}

void ath_chanctx_set_next(struct ath_softc *sc, bool force)
{
	struct timespec ts;
	bool measure_time = false;
	bool send_ps = false;

	spin_lock_bh(&sc->chan_lock);
	if (!sc->next_chan) {
		spin_unlock_bh(&sc->chan_lock);
		return;
	}

	if (!force && ath_chanctx_defer_switch(sc)) {
		spin_unlock_bh(&sc->chan_lock);
		return;
	}

	if (sc->cur_chan != sc->next_chan) {
		sc->cur_chan->stopped = true;
		spin_unlock_bh(&sc->chan_lock);

		if (sc->next_chan == &sc->offchannel.chan) {
			getrawmonotonic(&ts);
			measure_time = true;
		}
		__ath9k_flush(sc->hw, ~0, true);

		if (ath_chanctx_send_ps_frame(sc, true))
			__ath9k_flush(sc->hw, BIT(IEEE80211_AC_VO), false);

		send_ps = true;
		spin_lock_bh(&sc->chan_lock);

		if (sc->cur_chan != &sc->offchannel.chan) {
			getrawmonotonic(&sc->cur_chan->tsf_ts);
			sc->cur_chan->tsf_val = ath9k_hw_gettsf64(sc->sc_ah);
		}
	}
	sc->cur_chan = sc->next_chan;
	sc->cur_chan->stopped = false;
	sc->next_chan = NULL;
	sc->sched.offchannel_duration = 0;
	if (sc->sched.state != ATH_CHANCTX_STATE_FORCE_ACTIVE)
		sc->sched.state = ATH_CHANCTX_STATE_IDLE;

	spin_unlock_bh(&sc->chan_lock);

	if (sc->sc_ah->chip_fullsleep ||
	    memcmp(&sc->cur_chandef, &sc->cur_chan->chandef,
		   sizeof(sc->cur_chandef))) {
		ath_set_channel(sc);
		if (measure_time)
			sc->sched.channel_switch_time =
				ath9k_hw_get_tsf_offset(&ts, NULL);
	}
	if (send_ps)
		ath_chanctx_send_ps_frame(sc, false);

	ath_offchannel_channel_change(sc);
	ath_chanctx_event(sc, NULL, ATH_CHANCTX_EVENT_SWITCH);
}

static void ath_chanctx_work(struct work_struct *work)
{
	struct ath_softc *sc = container_of(work, struct ath_softc,
					    chanctx_work);
	mutex_lock(&sc->mutex);
	ath_chanctx_set_next(sc, false);
	mutex_unlock(&sc->mutex);
}

void ath9k_offchannel_init(struct ath_softc *sc)
{
	struct ath_chanctx *ctx;
	struct ath_common *common = ath9k_hw_common(sc->sc_ah);
	struct ieee80211_supported_band *sband;
	struct ieee80211_channel *chan;
	int i;

	sband = &common->sbands[IEEE80211_BAND_2GHZ];
	if (!sband->n_channels)
		sband = &common->sbands[IEEE80211_BAND_5GHZ];

	chan = &sband->channels[0];

	ctx = &sc->offchannel.chan;
	INIT_LIST_HEAD(&ctx->vifs);
	ctx->txpower = ATH_TXPOWER_MAX;
	cfg80211_chandef_create(&ctx->chandef, chan, NL80211_CHAN_HT20);

	for (i = 0; i < ARRAY_SIZE(ctx->acq); i++)
		INIT_LIST_HEAD(&ctx->acq[i]);

	sc->offchannel.chan.offchannel = true;
}

void ath9k_init_channel_context(struct ath_softc *sc)
{
	INIT_WORK(&sc->chanctx_work, ath_chanctx_work);

	setup_timer(&sc->offchannel.timer, ath_offchannel_timer,
		    (unsigned long)sc);
	setup_timer(&sc->sched.timer, ath_chanctx_timer,
		    (unsigned long)sc);
}

void ath9k_deinit_channel_context(struct ath_softc *sc)
{
	cancel_work_sync(&sc->chanctx_work);
}

bool ath9k_is_chanctx_enabled(void)
{
	return (ath9k_use_chanctx == 1);
}

/********************/
/* Queue management */
/********************/

void ath9k_chanctx_wake_queues(struct ath_softc *sc)
{
	struct ath_hw *ah = sc->sc_ah;
	int i;

	if (sc->cur_chan == &sc->offchannel.chan) {
		ieee80211_wake_queue(sc->hw,
				     sc->hw->offchannel_tx_hw_queue);
	} else {
		for (i = 0; i < IEEE80211_NUM_ACS; i++)
			ieee80211_wake_queue(sc->hw,
					     sc->cur_chan->hw_queue_base + i);
	}

	if (ah->opmode == NL80211_IFTYPE_AP)
		ieee80211_wake_queue(sc->hw, sc->hw->queues - 2);
}

/*****************/
/* P2P Powersave */
/*****************/

static void ath9k_update_p2p_ps_timer(struct ath_softc *sc, struct ath_vif *avp)
{
	struct ath_hw *ah = sc->sc_ah;
	s32 tsf, target_tsf;

	if (!avp || !avp->noa.has_next_tsf)
		return;

	ath9k_hw_gen_timer_stop(ah, sc->p2p_ps_timer);

	tsf = ath9k_hw_gettsf32(sc->sc_ah);

	target_tsf = avp->noa.next_tsf;
	if (!avp->noa.absent)
		target_tsf -= ATH_P2P_PS_STOP_TIME;

	if (target_tsf - tsf < ATH_P2P_PS_STOP_TIME)
		target_tsf = tsf + ATH_P2P_PS_STOP_TIME;

	ath9k_hw_gen_timer_start(ah, sc->p2p_ps_timer, (u32) target_tsf, 1000000);
}

static void ath9k_update_p2p_ps(struct ath_softc *sc, struct ieee80211_vif *vif)
{
	struct ath_vif *avp = (void *)vif->drv_priv;
	u32 tsf;

	if (!sc->p2p_ps_timer)
		return;

	if (vif->type != NL80211_IFTYPE_STATION || !vif->p2p)
		return;

	sc->p2p_ps_vif = avp;
	tsf = ath9k_hw_gettsf32(sc->sc_ah);
	ieee80211_parse_p2p_noa(&vif->bss_conf.p2p_noa_attr, &avp->noa, tsf);
	ath9k_update_p2p_ps_timer(sc, avp);
}

void ath9k_p2p_ps_timer(void *priv)
{
	struct ath_softc *sc = priv;
	struct ath_vif *avp = sc->p2p_ps_vif;
	struct ieee80211_vif *vif;
	struct ieee80211_sta *sta;
	struct ath_node *an;
	u32 tsf;

	del_timer_sync(&sc->sched.timer);
	ath9k_hw_gen_timer_stop(sc->sc_ah, sc->p2p_ps_timer);
	ath_chanctx_event(sc, NULL, ATH_CHANCTX_EVENT_TSF_TIMER);

	if (!avp || avp->chanctx != sc->cur_chan)
		return;

	tsf = ath9k_hw_gettsf32(sc->sc_ah);
	if (!avp->noa.absent)
		tsf += ATH_P2P_PS_STOP_TIME;

	if (!avp->noa.has_next_tsf ||
	    avp->noa.next_tsf - tsf > BIT(31))
		ieee80211_update_p2p_noa(&avp->noa, tsf);

	ath9k_update_p2p_ps_timer(sc, avp);

	rcu_read_lock();

	vif = avp->vif;
	sta = ieee80211_find_sta(vif, vif->bss_conf.bssid);
	if (!sta)
		goto out;

	an = (void *) sta->drv_priv;
	if (an->sleeping == !!avp->noa.absent)
		goto out;

	an->sleeping = avp->noa.absent;
	if (an->sleeping)
		ath_tx_aggr_sleep(sta, sc, an);
	else
		ath_tx_aggr_wakeup(sc, an);

out:
	rcu_read_unlock();
}

void ath9k_p2p_bss_info_changed(struct ath_softc *sc,
				struct ieee80211_vif *vif)
{
	unsigned long flags;

	spin_lock_bh(&sc->sc_pcu_lock);
	spin_lock_irqsave(&sc->sc_pm_lock, flags);
	if (!(sc->ps_flags & PS_BEACON_SYNC))
		ath9k_update_p2p_ps(sc, vif);
	spin_unlock_irqrestore(&sc->sc_pm_lock, flags);
	spin_unlock_bh(&sc->sc_pcu_lock);
}

void ath9k_p2p_beacon_sync(struct ath_softc *sc)
{
	if (sc->p2p_ps_vif)
		ath9k_update_p2p_ps(sc, sc->p2p_ps_vif->vif);
}

void ath9k_p2p_remove_vif(struct ath_softc *sc,
			  struct ieee80211_vif *vif)
{
	struct ath_vif *avp = (void *)vif->drv_priv;

	spin_lock_bh(&sc->sc_pcu_lock);
	if (avp == sc->p2p_ps_vif) {
		sc->p2p_ps_vif = NULL;
		ath9k_update_p2p_ps_timer(sc, NULL);
	}
	spin_unlock_bh(&sc->sc_pcu_lock);
}

int ath9k_init_p2p(struct ath_softc *sc)
{
	sc->p2p_ps_timer = ath_gen_timer_alloc(sc->sc_ah, ath9k_p2p_ps_timer,
					       NULL, sc, AR_FIRST_NDP_TIMER);
	if (!sc->p2p_ps_timer)
		return -ENOMEM;

	return 0;
}

void ath9k_deinit_p2p(struct ath_softc *sc)
{
	if (sc->p2p_ps_timer)
		ath_gen_timer_free(sc->sc_ah, sc->p2p_ps_timer);
}

#endif /* CONFIG_ATH9K_CHANNEL_CONTEXT */
