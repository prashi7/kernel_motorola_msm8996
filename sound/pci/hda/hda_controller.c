/*
 *
 *  Implementation of primary alsa driver code base for Intel HD Audio.
 *
 *  Copyright(c) 2004 Intel Corporation. All rights reserved.
 *
 *  Copyright (c) 2004 Takashi Iwai <tiwai@suse.de>
 *                     PeiSen Hou <pshou@realtek.com.tw>
 *
 *  This program is free software; you can redistribute it and/or modify it
 *  under the terms of the GNU General Public License as published by the Free
 *  Software Foundation; either version 2 of the License, or (at your option)
 *  any later version.
 *
 *  This program is distributed in the hope that it will be useful, but WITHOUT
 *  ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or
 *  FITNESS FOR A PARTICULAR PURPOSE.  See the GNU General Public License for
 *  more details.
 *
 *
 */

#include <linux/clocksource.h>
#include <linux/delay.h>
#include <linux/interrupt.h>
#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/slab.h>
#include <sound/core.h>
#include <sound/initval.h>
#include "hda_priv.h"
#include "hda_controller.h"

#define CREATE_TRACE_POINTS
#include "hda_intel_trace.h"

/* DSP lock helpers */
#ifdef CONFIG_SND_HDA_DSP_LOADER
#define dsp_lock_init(dev)	mutex_init(&(dev)->dsp_mutex)
#define dsp_lock(dev)		mutex_lock(&(dev)->dsp_mutex)
#define dsp_unlock(dev)		mutex_unlock(&(dev)->dsp_mutex)
#define dsp_is_locked(dev)	((dev)->locked)
#else
#define dsp_lock_init(dev)	do {} while (0)
#define dsp_lock(dev)		do {} while (0)
#define dsp_unlock(dev)		do {} while (0)
#define dsp_is_locked(dev)	0
#endif

/*
 * AZX stream operations.
 */

/* start a stream */
static void azx_stream_start(struct azx *chip, struct azx_dev *azx_dev)
{
	/*
	 * Before stream start, initialize parameter
	 */
	azx_dev->insufficient = 1;

	/* enable SIE */
	azx_writel(chip, INTCTL,
		   azx_readl(chip, INTCTL) | (1 << azx_dev->index));
	/* set DMA start and interrupt mask */
	azx_sd_writeb(chip, azx_dev, SD_CTL,
		      azx_sd_readb(chip, azx_dev, SD_CTL) |
		      SD_CTL_DMA_START | SD_INT_MASK);
}

/* stop DMA */
static void azx_stream_clear(struct azx *chip, struct azx_dev *azx_dev)
{
	azx_sd_writeb(chip, azx_dev, SD_CTL,
		      azx_sd_readb(chip, azx_dev, SD_CTL) &
		      ~(SD_CTL_DMA_START | SD_INT_MASK));
	azx_sd_writeb(chip, azx_dev, SD_STS, SD_INT_MASK); /* to be sure */
}

/* stop a stream */
void azx_stream_stop(struct azx *chip, struct azx_dev *azx_dev)
{
	azx_stream_clear(chip, azx_dev);
	/* disable SIE */
	azx_writel(chip, INTCTL,
		   azx_readl(chip, INTCTL) & ~(1 << azx_dev->index));
}
EXPORT_SYMBOL_GPL(azx_stream_stop);

/* reset stream */
static void azx_stream_reset(struct azx *chip, struct azx_dev *azx_dev)
{
	unsigned char val;
	int timeout;

	azx_stream_clear(chip, azx_dev);

	azx_sd_writeb(chip, azx_dev, SD_CTL,
		      azx_sd_readb(chip, azx_dev, SD_CTL) |
		      SD_CTL_STREAM_RESET);
	udelay(3);
	timeout = 300;
	while (!((val = azx_sd_readb(chip, azx_dev, SD_CTL)) &
		 SD_CTL_STREAM_RESET) && --timeout)
		;
	val &= ~SD_CTL_STREAM_RESET;
	azx_sd_writeb(chip, azx_dev, SD_CTL, val);
	udelay(3);

	timeout = 300;
	/* waiting for hardware to report that the stream is out of reset */
	while (((val = azx_sd_readb(chip, azx_dev, SD_CTL)) &
		SD_CTL_STREAM_RESET) && --timeout)
		;

	/* reset first position - may not be synced with hw at this time */
	*azx_dev->posbuf = 0;
}

/*
 * set up the SD for streaming
 */
static int azx_setup_controller(struct azx *chip, struct azx_dev *azx_dev)
{
	unsigned int val;
	/* make sure the run bit is zero for SD */
	azx_stream_clear(chip, azx_dev);
	/* program the stream_tag */
	val = azx_sd_readl(chip, azx_dev, SD_CTL);
	val = (val & ~SD_CTL_STREAM_TAG_MASK) |
		(azx_dev->stream_tag << SD_CTL_STREAM_TAG_SHIFT);
	if (!azx_snoop(chip))
		val |= SD_CTL_TRAFFIC_PRIO;
	azx_sd_writel(chip, azx_dev, SD_CTL, val);

	/* program the length of samples in cyclic buffer */
	azx_sd_writel(chip, azx_dev, SD_CBL, azx_dev->bufsize);

	/* program the stream format */
	/* this value needs to be the same as the one programmed */
	azx_sd_writew(chip, azx_dev, SD_FORMAT, azx_dev->format_val);

	/* program the stream LVI (last valid index) of the BDL */
	azx_sd_writew(chip, azx_dev, SD_LVI, azx_dev->frags - 1);

	/* program the BDL address */
	/* lower BDL address */
	azx_sd_writel(chip, azx_dev, SD_BDLPL, (u32)azx_dev->bdl.addr);
	/* upper BDL address */
	azx_sd_writel(chip, azx_dev, SD_BDLPU,
		      upper_32_bits(azx_dev->bdl.addr));

	/* enable the position buffer */
	if (chip->position_fix[0] != POS_FIX_LPIB ||
	    chip->position_fix[1] != POS_FIX_LPIB) {
		if (!(azx_readl(chip, DPLBASE) & ICH6_DPLBASE_ENABLE))
			azx_writel(chip, DPLBASE,
				(u32)chip->posbuf.addr | ICH6_DPLBASE_ENABLE);
	}

	/* set the interrupt enable bits in the descriptor control register */
	azx_sd_writel(chip, azx_dev, SD_CTL,
		      azx_sd_readl(chip, azx_dev, SD_CTL) | SD_INT_MASK);

	return 0;
}

/* assign a stream for the PCM */
static inline struct azx_dev *
azx_assign_device(struct azx *chip, struct snd_pcm_substream *substream)
{
	int dev, i, nums;
	struct azx_dev *res = NULL;
	/* make a non-zero unique key for the substream */
	int key = (substream->pcm->device << 16) | (substream->number << 2) |
		(substream->stream + 1);

	if (substream->stream == SNDRV_PCM_STREAM_PLAYBACK) {
		dev = chip->playback_index_offset;
		nums = chip->playback_streams;
	} else {
		dev = chip->capture_index_offset;
		nums = chip->capture_streams;
	}
	for (i = 0; i < nums; i++, dev++) {
		struct azx_dev *azx_dev = &chip->azx_dev[dev];
		dsp_lock(azx_dev);
		if (!azx_dev->opened && !dsp_is_locked(azx_dev)) {
			res = azx_dev;
			if (res->assigned_key == key) {
				res->opened = 1;
				res->assigned_key = key;
				dsp_unlock(azx_dev);
				return azx_dev;
			}
		}
		dsp_unlock(azx_dev);
	}
	if (res) {
		dsp_lock(res);
		res->opened = 1;
		res->assigned_key = key;
		dsp_unlock(res);
	}
	return res;
}

/* release the assigned stream */
static inline void azx_release_device(struct azx_dev *azx_dev)
{
	azx_dev->opened = 0;
}

static cycle_t azx_cc_read(const struct cyclecounter *cc)
{
	struct azx_dev *azx_dev = container_of(cc, struct azx_dev, azx_cc);
	struct snd_pcm_substream *substream = azx_dev->substream;
	struct azx_pcm *apcm = snd_pcm_substream_chip(substream);
	struct azx *chip = apcm->chip;

	return azx_readl(chip, WALLCLK);
}

static void azx_timecounter_init(struct snd_pcm_substream *substream,
				bool force, cycle_t last)
{
	struct azx_dev *azx_dev = get_azx_dev(substream);
	struct timecounter *tc = &azx_dev->azx_tc;
	struct cyclecounter *cc = &azx_dev->azx_cc;
	u64 nsec;

	cc->read = azx_cc_read;
	cc->mask = CLOCKSOURCE_MASK(32);

	/*
	 * Converting from 24 MHz to ns means applying a 125/3 factor.
	 * To avoid any saturation issues in intermediate operations,
	 * the 125 factor is applied first. The division is applied
	 * last after reading the timecounter value.
	 * Applying the 1/3 factor as part of the multiplication
	 * requires at least 20 bits for a decent precision, however
	 * overflows occur after about 4 hours or less, not a option.
	 */

	cc->mult = 125; /* saturation after 195 years */
	cc->shift = 0;

	nsec = 0; /* audio time is elapsed time since trigger */
	timecounter_init(tc, cc, nsec);
	if (force)
		/*
		 * force timecounter to use predefined value,
		 * used for synchronized starts
		 */
		tc->cycle_last = last;
}

static u64 azx_adjust_codec_delay(struct snd_pcm_substream *substream,
				u64 nsec)
{
	struct azx_pcm *apcm = snd_pcm_substream_chip(substream);
	struct hda_pcm_stream *hinfo = apcm->hinfo[substream->stream];
	u64 codec_frames, codec_nsecs;

	if (!hinfo->ops.get_delay)
		return nsec;

	codec_frames = hinfo->ops.get_delay(hinfo, apcm->codec, substream);
	codec_nsecs = div_u64(codec_frames * 1000000000LL,
			      substream->runtime->rate);

	if (substream->stream == SNDRV_PCM_STREAM_CAPTURE)
		return nsec + codec_nsecs;

	return (nsec > codec_nsecs) ? nsec - codec_nsecs : 0;
}

/*
 * set up a BDL entry
 */
static int setup_bdle(struct azx *chip,
		      struct snd_dma_buffer *dmab,
		      struct azx_dev *azx_dev, u32 **bdlp,
		      int ofs, int size, int with_ioc)
{
	u32 *bdl = *bdlp;

	while (size > 0) {
		dma_addr_t addr;
		int chunk;

		if (azx_dev->frags >= AZX_MAX_BDL_ENTRIES)
			return -EINVAL;

		addr = snd_sgbuf_get_addr(dmab, ofs);
		/* program the address field of the BDL entry */
		bdl[0] = cpu_to_le32((u32)addr);
		bdl[1] = cpu_to_le32(upper_32_bits(addr));
		/* program the size field of the BDL entry */
		chunk = snd_sgbuf_get_chunk_size(dmab, ofs, size);
		/* one BDLE cannot cross 4K boundary on CTHDA chips */
		if (chip->driver_caps & AZX_DCAPS_4K_BDLE_BOUNDARY) {
			u32 remain = 0x1000 - (ofs & 0xfff);
			if (chunk > remain)
				chunk = remain;
		}
		bdl[2] = cpu_to_le32(chunk);
		/* program the IOC to enable interrupt
		 * only when the whole fragment is processed
		 */
		size -= chunk;
		bdl[3] = (size || !with_ioc) ? 0 : cpu_to_le32(0x01);
		bdl += 4;
		azx_dev->frags++;
		ofs += chunk;
	}
	*bdlp = bdl;
	return ofs;
}

/*
 * set up BDL entries
 */
static int azx_setup_periods(struct azx *chip,
			     struct snd_pcm_substream *substream,
			     struct azx_dev *azx_dev)
{
	u32 *bdl;
	int i, ofs, periods, period_bytes;
	int pos_adj = 0;

	/* reset BDL address */
	azx_sd_writel(chip, azx_dev, SD_BDLPL, 0);
	azx_sd_writel(chip, azx_dev, SD_BDLPU, 0);

	period_bytes = azx_dev->period_bytes;
	periods = azx_dev->bufsize / period_bytes;

	/* program the initial BDL entries */
	bdl = (u32 *)azx_dev->bdl.area;
	ofs = 0;
	azx_dev->frags = 0;

	if (chip->bdl_pos_adj)
		pos_adj = chip->bdl_pos_adj[chip->dev_index];
	if (!azx_dev->no_period_wakeup && pos_adj > 0) {
		struct snd_pcm_runtime *runtime = substream->runtime;
		int pos_align = pos_adj;
		pos_adj = (pos_adj * runtime->rate + 47999) / 48000;
		if (!pos_adj)
			pos_adj = pos_align;
		else
			pos_adj = ((pos_adj + pos_align - 1) / pos_align) *
				pos_align;
		pos_adj = frames_to_bytes(runtime, pos_adj);
		if (pos_adj >= period_bytes) {
			dev_warn(chip->card->dev,"Too big adjustment %d\n",
				 pos_adj);
			pos_adj = 0;
		} else {
			ofs = setup_bdle(chip, snd_pcm_get_dma_buf(substream),
					 azx_dev,
					 &bdl, ofs, pos_adj, true);
			if (ofs < 0)
				goto error;
		}
	} else
		pos_adj = 0;

	for (i = 0; i < periods; i++) {
		if (i == periods - 1 && pos_adj)
			ofs = setup_bdle(chip, snd_pcm_get_dma_buf(substream),
					 azx_dev, &bdl, ofs,
					 period_bytes - pos_adj, 0);
		else
			ofs = setup_bdle(chip, snd_pcm_get_dma_buf(substream),
					 azx_dev, &bdl, ofs,
					 period_bytes,
					 !azx_dev->no_period_wakeup);
		if (ofs < 0)
			goto error;
	}
	return 0;

 error:
	dev_err(chip->card->dev, "Too many BDL entries: buffer=%d, period=%d\n",
		azx_dev->bufsize, period_bytes);
	return -EINVAL;
}

/*
 * PCM ops
 */

static int azx_pcm_close(struct snd_pcm_substream *substream)
{
	struct azx_pcm *apcm = snd_pcm_substream_chip(substream);
	struct hda_pcm_stream *hinfo = apcm->hinfo[substream->stream];
	struct azx *chip = apcm->chip;
	struct azx_dev *azx_dev = get_azx_dev(substream);
	unsigned long flags;

	mutex_lock(&chip->open_mutex);
	spin_lock_irqsave(&chip->reg_lock, flags);
	azx_dev->substream = NULL;
	azx_dev->running = 0;
	spin_unlock_irqrestore(&chip->reg_lock, flags);
	azx_release_device(azx_dev);
	hinfo->ops.close(hinfo, apcm->codec, substream);
	snd_hda_power_down(apcm->codec);
	mutex_unlock(&chip->open_mutex);
	return 0;
}

static int azx_pcm_hw_params(struct snd_pcm_substream *substream,
			     struct snd_pcm_hw_params *hw_params)
{
	struct azx_pcm *apcm = snd_pcm_substream_chip(substream);
	struct azx *chip = apcm->chip;
	int ret;

	dsp_lock(get_azx_dev(substream));
	if (dsp_is_locked(get_azx_dev(substream))) {
		ret = -EBUSY;
		goto unlock;
	}

	ret = chip->ops->substream_alloc_pages(chip, substream,
					  params_buffer_bytes(hw_params));
unlock:
	dsp_unlock(get_azx_dev(substream));
	return ret;
}

static int azx_pcm_hw_free(struct snd_pcm_substream *substream)
{
	struct azx_pcm *apcm = snd_pcm_substream_chip(substream);
	struct azx_dev *azx_dev = get_azx_dev(substream);
	struct azx *chip = apcm->chip;
	struct hda_pcm_stream *hinfo = apcm->hinfo[substream->stream];
	int err;

	/* reset BDL address */
	dsp_lock(azx_dev);
	if (!dsp_is_locked(azx_dev)) {
		azx_sd_writel(chip, azx_dev, SD_BDLPL, 0);
		azx_sd_writel(chip, azx_dev, SD_BDLPU, 0);
		azx_sd_writel(chip, azx_dev, SD_CTL, 0);
		azx_dev->bufsize = 0;
		azx_dev->period_bytes = 0;
		azx_dev->format_val = 0;
	}

	snd_hda_codec_cleanup(apcm->codec, hinfo, substream);

	err = chip->ops->substream_free_pages(chip, substream);
	azx_dev->prepared = 0;
	dsp_unlock(azx_dev);
	return err;
}

static int azx_pcm_prepare(struct snd_pcm_substream *substream)
{
	struct azx_pcm *apcm = snd_pcm_substream_chip(substream);
	struct azx *chip = apcm->chip;
	struct azx_dev *azx_dev = get_azx_dev(substream);
	struct hda_pcm_stream *hinfo = apcm->hinfo[substream->stream];
	struct snd_pcm_runtime *runtime = substream->runtime;
	unsigned int bufsize, period_bytes, format_val, stream_tag;
	int err;
	struct hda_spdif_out *spdif =
		snd_hda_spdif_out_of_nid(apcm->codec, hinfo->nid);
	unsigned short ctls = spdif ? spdif->ctls : 0;

	dsp_lock(azx_dev);
	if (dsp_is_locked(azx_dev)) {
		err = -EBUSY;
		goto unlock;
	}

	azx_stream_reset(chip, azx_dev);
	format_val = snd_hda_calc_stream_format(runtime->rate,
						runtime->channels,
						runtime->format,
						hinfo->maxbps,
						ctls);
	if (!format_val) {
		dev_err(chip->card->dev,
			"invalid format_val, rate=%d, ch=%d, format=%d\n",
			runtime->rate, runtime->channels, runtime->format);
		err = -EINVAL;
		goto unlock;
	}

	bufsize = snd_pcm_lib_buffer_bytes(substream);
	period_bytes = snd_pcm_lib_period_bytes(substream);

	dev_dbg(chip->card->dev, "azx_pcm_prepare: bufsize=0x%x, format=0x%x\n",
		bufsize, format_val);

	if (bufsize != azx_dev->bufsize ||
	    period_bytes != azx_dev->period_bytes ||
	    format_val != azx_dev->format_val ||
	    runtime->no_period_wakeup != azx_dev->no_period_wakeup) {
		azx_dev->bufsize = bufsize;
		azx_dev->period_bytes = period_bytes;
		azx_dev->format_val = format_val;
		azx_dev->no_period_wakeup = runtime->no_period_wakeup;
		err = azx_setup_periods(chip, substream, azx_dev);
		if (err < 0)
			goto unlock;
	}

	/* when LPIB delay correction gives a small negative value,
	 * we ignore it; currently set the threshold statically to
	 * 64 frames
	 */
	if (runtime->period_size > 64)
		azx_dev->delay_negative_threshold = -frames_to_bytes(runtime, 64);
	else
		azx_dev->delay_negative_threshold = 0;

	/* wallclk has 24Mhz clock source */
	azx_dev->period_wallclk = (((runtime->period_size * 24000) /
						runtime->rate) * 1000);
	azx_setup_controller(chip, azx_dev);
	if (substream->stream == SNDRV_PCM_STREAM_PLAYBACK)
		azx_dev->fifo_size =
			azx_sd_readw(chip, azx_dev, SD_FIFOSIZE) + 1;
	else
		azx_dev->fifo_size = 0;

	stream_tag = azx_dev->stream_tag;
	/* CA-IBG chips need the playback stream starting from 1 */
	if ((chip->driver_caps & AZX_DCAPS_CTX_WORKAROUND) &&
	    stream_tag > chip->capture_streams)
		stream_tag -= chip->capture_streams;
	err = snd_hda_codec_prepare(apcm->codec, hinfo, stream_tag,
				     azx_dev->format_val, substream);

 unlock:
	if (!err)
		azx_dev->prepared = 1;
	dsp_unlock(azx_dev);
	return err;
}

static int azx_pcm_trigger(struct snd_pcm_substream *substream, int cmd)
{
	struct azx_pcm *apcm = snd_pcm_substream_chip(substream);
	struct azx *chip = apcm->chip;
	struct azx_dev *azx_dev;
	struct snd_pcm_substream *s;
	int rstart = 0, start, nsync = 0, sbits = 0;
	int nwait, timeout;

	azx_dev = get_azx_dev(substream);
	trace_azx_pcm_trigger(chip, azx_dev, cmd);

	if (dsp_is_locked(azx_dev) || !azx_dev->prepared)
		return -EPIPE;

	switch (cmd) {
	case SNDRV_PCM_TRIGGER_START:
		rstart = 1;
	case SNDRV_PCM_TRIGGER_PAUSE_RELEASE:
	case SNDRV_PCM_TRIGGER_RESUME:
		start = 1;
		break;
	case SNDRV_PCM_TRIGGER_PAUSE_PUSH:
	case SNDRV_PCM_TRIGGER_SUSPEND:
	case SNDRV_PCM_TRIGGER_STOP:
		start = 0;
		break;
	default:
		return -EINVAL;
	}

	snd_pcm_group_for_each_entry(s, substream) {
		if (s->pcm->card != substream->pcm->card)
			continue;
		azx_dev = get_azx_dev(s);
		sbits |= 1 << azx_dev->index;
		nsync++;
		snd_pcm_trigger_done(s, substream);
	}

	spin_lock(&chip->reg_lock);

	/* first, set SYNC bits of corresponding streams */
	if (chip->driver_caps & AZX_DCAPS_OLD_SSYNC)
		azx_writel(chip, OLD_SSYNC,
			azx_readl(chip, OLD_SSYNC) | sbits);
	else
		azx_writel(chip, SSYNC, azx_readl(chip, SSYNC) | sbits);

	snd_pcm_group_for_each_entry(s, substream) {
		if (s->pcm->card != substream->pcm->card)
			continue;
		azx_dev = get_azx_dev(s);
		if (start) {
			azx_dev->start_wallclk = azx_readl(chip, WALLCLK);
			if (!rstart)
				azx_dev->start_wallclk -=
						azx_dev->period_wallclk;
			azx_stream_start(chip, azx_dev);
		} else {
			azx_stream_stop(chip, azx_dev);
		}
		azx_dev->running = start;
	}
	spin_unlock(&chip->reg_lock);
	if (start) {
		/* wait until all FIFOs get ready */
		for (timeout = 5000; timeout; timeout--) {
			nwait = 0;
			snd_pcm_group_for_each_entry(s, substream) {
				if (s->pcm->card != substream->pcm->card)
					continue;
				azx_dev = get_azx_dev(s);
				if (!(azx_sd_readb(chip, azx_dev, SD_STS) &
				      SD_STS_FIFO_READY))
					nwait++;
			}
			if (!nwait)
				break;
			cpu_relax();
		}
	} else {
		/* wait until all RUN bits are cleared */
		for (timeout = 5000; timeout; timeout--) {
			nwait = 0;
			snd_pcm_group_for_each_entry(s, substream) {
				if (s->pcm->card != substream->pcm->card)
					continue;
				azx_dev = get_azx_dev(s);
				if (azx_sd_readb(chip, azx_dev, SD_CTL) &
				    SD_CTL_DMA_START)
					nwait++;
			}
			if (!nwait)
				break;
			cpu_relax();
		}
	}
	spin_lock(&chip->reg_lock);
	/* reset SYNC bits */
	if (chip->driver_caps & AZX_DCAPS_OLD_SSYNC)
		azx_writel(chip, OLD_SSYNC,
			azx_readl(chip, OLD_SSYNC) & ~sbits);
	else
		azx_writel(chip, SSYNC, azx_readl(chip, SSYNC) & ~sbits);
	if (start) {
		azx_timecounter_init(substream, 0, 0);
		if (nsync > 1) {
			cycle_t cycle_last;

			/* same start cycle for master and group */
			azx_dev = get_azx_dev(substream);
			cycle_last = azx_dev->azx_tc.cycle_last;

			snd_pcm_group_for_each_entry(s, substream) {
				if (s->pcm->card != substream->pcm->card)
					continue;
				azx_timecounter_init(s, 1, cycle_last);
			}
		}
	}
	spin_unlock(&chip->reg_lock);
	return 0;
}

/* get the current DMA position with correction on VIA chips */
static unsigned int azx_via_get_position(struct azx *chip,
					 struct azx_dev *azx_dev)
{
	unsigned int link_pos, mini_pos, bound_pos;
	unsigned int mod_link_pos, mod_dma_pos, mod_mini_pos;
	unsigned int fifo_size;

	link_pos = azx_sd_readl(chip, azx_dev, SD_LPIB);
	if (azx_dev->substream->stream == SNDRV_PCM_STREAM_PLAYBACK) {
		/* Playback, no problem using link position */
		return link_pos;
	}

	/* Capture */
	/* For new chipset,
	 * use mod to get the DMA position just like old chipset
	 */
	mod_dma_pos = le32_to_cpu(*azx_dev->posbuf);
	mod_dma_pos %= azx_dev->period_bytes;

	/* azx_dev->fifo_size can't get FIFO size of in stream.
	 * Get from base address + offset.
	 */
	fifo_size = readw(chip->remap_addr + VIA_IN_STREAM0_FIFO_SIZE_OFFSET);

	if (azx_dev->insufficient) {
		/* Link position never gather than FIFO size */
		if (link_pos <= fifo_size)
			return 0;

		azx_dev->insufficient = 0;
	}

	if (link_pos <= fifo_size)
		mini_pos = azx_dev->bufsize + link_pos - fifo_size;
	else
		mini_pos = link_pos - fifo_size;

	/* Find nearest previous boudary */
	mod_mini_pos = mini_pos % azx_dev->period_bytes;
	mod_link_pos = link_pos % azx_dev->period_bytes;
	if (mod_link_pos >= fifo_size)
		bound_pos = link_pos - mod_link_pos;
	else if (mod_dma_pos >= mod_mini_pos)
		bound_pos = mini_pos - mod_mini_pos;
	else {
		bound_pos = mini_pos - mod_mini_pos + azx_dev->period_bytes;
		if (bound_pos >= azx_dev->bufsize)
			bound_pos = 0;
	}

	/* Calculate real DMA position we want */
	return bound_pos + mod_dma_pos;
}

unsigned int azx_get_position(struct azx *chip,
			      struct azx_dev *azx_dev,
			      bool with_check)
{
	struct snd_pcm_substream *substream = azx_dev->substream;
	struct azx_pcm *apcm = snd_pcm_substream_chip(substream);
	unsigned int pos;
	int stream = substream->stream;
	struct hda_pcm_stream *hinfo = apcm->hinfo[stream];
	int delay = 0;

	switch (chip->position_fix[stream]) {
	case POS_FIX_LPIB:
		/* read LPIB */
		pos = azx_sd_readl(chip, azx_dev, SD_LPIB);
		break;
	case POS_FIX_VIACOMBO:
		pos = azx_via_get_position(chip, azx_dev);
		break;
	default:
		/* use the position buffer */
		pos = le32_to_cpu(*azx_dev->posbuf);
		if (with_check && chip->position_fix[stream] == POS_FIX_AUTO) {
			if (!pos || pos == (u32)-1) {
				dev_info(chip->card->dev,
					 "Invalid position buffer, using LPIB read method instead.\n");
				chip->position_fix[stream] = POS_FIX_LPIB;
				pos = azx_sd_readl(chip, azx_dev, SD_LPIB);
			} else
				chip->position_fix[stream] = POS_FIX_POSBUF;
		}
		break;
	}

	if (pos >= azx_dev->bufsize)
		pos = 0;

	/* calculate runtime delay from LPIB */
	if (substream->runtime &&
	    chip->position_fix[stream] == POS_FIX_POSBUF &&
	    (chip->driver_caps & AZX_DCAPS_COUNT_LPIB_DELAY)) {
		unsigned int lpib_pos = azx_sd_readl(chip, azx_dev, SD_LPIB);
		if (stream == SNDRV_PCM_STREAM_PLAYBACK)
			delay = pos - lpib_pos;
		else
			delay = lpib_pos - pos;
		if (delay < 0) {
			if (delay >= azx_dev->delay_negative_threshold)
				delay = 0;
			else
				delay += azx_dev->bufsize;
		}
		if (delay >= azx_dev->period_bytes) {
			dev_info(chip->card->dev,
				 "Unstable LPIB (%d >= %d); disabling LPIB delay counting\n",
				 delay, azx_dev->period_bytes);
			delay = 0;
			chip->driver_caps &= ~AZX_DCAPS_COUNT_LPIB_DELAY;
		}
		delay = bytes_to_frames(substream->runtime, delay);
	}

	if (substream->runtime) {
		if (hinfo->ops.get_delay)
			delay += hinfo->ops.get_delay(hinfo, apcm->codec,
						      substream);
		substream->runtime->delay = delay;
	}

	trace_azx_get_position(chip, azx_dev, pos, delay);
	return pos;
}
EXPORT_SYMBOL_GPL(azx_get_position);

static snd_pcm_uframes_t azx_pcm_pointer(struct snd_pcm_substream *substream)
{
	struct azx_pcm *apcm = snd_pcm_substream_chip(substream);
	struct azx *chip = apcm->chip;
	struct azx_dev *azx_dev = get_azx_dev(substream);
	return bytes_to_frames(substream->runtime,
			       azx_get_position(chip, azx_dev, false));
}

static int azx_get_wallclock_tstamp(struct snd_pcm_substream *substream,
				struct timespec *ts)
{
	struct azx_dev *azx_dev = get_azx_dev(substream);
	u64 nsec;

	nsec = timecounter_read(&azx_dev->azx_tc);
	nsec = div_u64(nsec, 3); /* can be optimized */
	nsec = azx_adjust_codec_delay(substream, nsec);

	*ts = ns_to_timespec(nsec);

	return 0;
}

static struct snd_pcm_hardware azx_pcm_hw = {
	.info =			(SNDRV_PCM_INFO_MMAP |
				 SNDRV_PCM_INFO_INTERLEAVED |
				 SNDRV_PCM_INFO_BLOCK_TRANSFER |
				 SNDRV_PCM_INFO_MMAP_VALID |
				 /* No full-resume yet implemented */
				 /* SNDRV_PCM_INFO_RESUME |*/
				 SNDRV_PCM_INFO_PAUSE |
				 SNDRV_PCM_INFO_SYNC_START |
				 SNDRV_PCM_INFO_HAS_WALL_CLOCK |
				 SNDRV_PCM_INFO_NO_PERIOD_WAKEUP),
	.formats =		SNDRV_PCM_FMTBIT_S16_LE,
	.rates =		SNDRV_PCM_RATE_48000,
	.rate_min =		48000,
	.rate_max =		48000,
	.channels_min =		2,
	.channels_max =		2,
	.buffer_bytes_max =	AZX_MAX_BUF_SIZE,
	.period_bytes_min =	128,
	.period_bytes_max =	AZX_MAX_BUF_SIZE / 2,
	.periods_min =		2,
	.periods_max =		AZX_MAX_FRAG,
	.fifo_size =		0,
};

static int azx_pcm_open(struct snd_pcm_substream *substream)
{
	struct azx_pcm *apcm = snd_pcm_substream_chip(substream);
	struct hda_pcm_stream *hinfo = apcm->hinfo[substream->stream];
	struct azx *chip = apcm->chip;
	struct azx_dev *azx_dev;
	struct snd_pcm_runtime *runtime = substream->runtime;
	unsigned long flags;
	int err;
	int buff_step;

	mutex_lock(&chip->open_mutex);
	azx_dev = azx_assign_device(chip, substream);
	if (azx_dev == NULL) {
		mutex_unlock(&chip->open_mutex);
		return -EBUSY;
	}
	runtime->hw = azx_pcm_hw;
	runtime->hw.channels_min = hinfo->channels_min;
	runtime->hw.channels_max = hinfo->channels_max;
	runtime->hw.formats = hinfo->formats;
	runtime->hw.rates = hinfo->rates;
	snd_pcm_limit_hw_rates(runtime);
	snd_pcm_hw_constraint_integer(runtime, SNDRV_PCM_HW_PARAM_PERIODS);

	/* avoid wrap-around with wall-clock */
	snd_pcm_hw_constraint_minmax(runtime, SNDRV_PCM_HW_PARAM_BUFFER_TIME,
				     20,
				     178000000);

	if (chip->align_buffer_size)
		/* constrain buffer sizes to be multiple of 128
		   bytes. This is more efficient in terms of memory
		   access but isn't required by the HDA spec and
		   prevents users from specifying exact period/buffer
		   sizes. For example for 44.1kHz, a period size set
		   to 20ms will be rounded to 19.59ms. */
		buff_step = 128;
	else
		/* Don't enforce steps on buffer sizes, still need to
		   be multiple of 4 bytes (HDA spec). Tested on Intel
		   HDA controllers, may not work on all devices where
		   option needs to be disabled */
		buff_step = 4;

	snd_pcm_hw_constraint_step(runtime, 0, SNDRV_PCM_HW_PARAM_BUFFER_BYTES,
				   buff_step);
	snd_pcm_hw_constraint_step(runtime, 0, SNDRV_PCM_HW_PARAM_PERIOD_BYTES,
				   buff_step);
	snd_hda_power_up_d3wait(apcm->codec);
	err = hinfo->ops.open(hinfo, apcm->codec, substream);
	if (err < 0) {
		azx_release_device(azx_dev);
		snd_hda_power_down(apcm->codec);
		mutex_unlock(&chip->open_mutex);
		return err;
	}
	snd_pcm_limit_hw_rates(runtime);
	/* sanity check */
	if (snd_BUG_ON(!runtime->hw.channels_min) ||
	    snd_BUG_ON(!runtime->hw.channels_max) ||
	    snd_BUG_ON(!runtime->hw.formats) ||
	    snd_BUG_ON(!runtime->hw.rates)) {
		azx_release_device(azx_dev);
		hinfo->ops.close(hinfo, apcm->codec, substream);
		snd_hda_power_down(apcm->codec);
		mutex_unlock(&chip->open_mutex);
		return -EINVAL;
	}

	/* disable WALLCLOCK timestamps for capture streams
	   until we figure out how to handle digital inputs */
	if (substream->stream == SNDRV_PCM_STREAM_CAPTURE)
		runtime->hw.info &= ~SNDRV_PCM_INFO_HAS_WALL_CLOCK;

	spin_lock_irqsave(&chip->reg_lock, flags);
	azx_dev->substream = substream;
	azx_dev->running = 0;
	spin_unlock_irqrestore(&chip->reg_lock, flags);

	runtime->private_data = azx_dev;
	snd_pcm_set_sync(substream);
	mutex_unlock(&chip->open_mutex);
	return 0;
}

static int azx_pcm_mmap(struct snd_pcm_substream *substream,
			struct vm_area_struct *area)
{
	struct azx_pcm *apcm = snd_pcm_substream_chip(substream);
	struct azx *chip = apcm->chip;
	if (chip->ops->pcm_mmap_prepare)
		chip->ops->pcm_mmap_prepare(substream, area);
	return snd_pcm_lib_default_mmap(substream, area);
}

static struct snd_pcm_ops azx_pcm_ops = {
	.open = azx_pcm_open,
	.close = azx_pcm_close,
	.ioctl = snd_pcm_lib_ioctl,
	.hw_params = azx_pcm_hw_params,
	.hw_free = azx_pcm_hw_free,
	.prepare = azx_pcm_prepare,
	.trigger = azx_pcm_trigger,
	.pointer = azx_pcm_pointer,
	.wall_clock =  azx_get_wallclock_tstamp,
	.mmap = azx_pcm_mmap,
	.page = snd_pcm_sgbuf_ops_page,
};

static void azx_pcm_free(struct snd_pcm *pcm)
{
	struct azx_pcm *apcm = pcm->private_data;
	if (apcm) {
		list_del(&apcm->list);
		kfree(apcm);
	}
}

#define MAX_PREALLOC_SIZE	(32 * 1024 * 1024)

int azx_attach_pcm_stream(struct hda_bus *bus, struct hda_codec *codec,
			  struct hda_pcm *cpcm)
{
	struct azx *chip = bus->private_data;
	struct snd_pcm *pcm;
	struct azx_pcm *apcm;
	int pcm_dev = cpcm->device;
	unsigned int size;
	int s, err;

	list_for_each_entry(apcm, &chip->pcm_list, list) {
		if (apcm->pcm->device == pcm_dev) {
			dev_err(chip->card->dev, "PCM %d already exists\n",
				pcm_dev);
			return -EBUSY;
		}
	}
	err = snd_pcm_new(chip->card, cpcm->name, pcm_dev,
			  cpcm->stream[SNDRV_PCM_STREAM_PLAYBACK].substreams,
			  cpcm->stream[SNDRV_PCM_STREAM_CAPTURE].substreams,
			  &pcm);
	if (err < 0)
		return err;
	strlcpy(pcm->name, cpcm->name, sizeof(pcm->name));
	apcm = kzalloc(sizeof(*apcm), GFP_KERNEL);
	if (apcm == NULL)
		return -ENOMEM;
	apcm->chip = chip;
	apcm->pcm = pcm;
	apcm->codec = codec;
	pcm->private_data = apcm;
	pcm->private_free = azx_pcm_free;
	if (cpcm->pcm_type == HDA_PCM_TYPE_MODEM)
		pcm->dev_class = SNDRV_PCM_CLASS_MODEM;
	list_add_tail(&apcm->list, &chip->pcm_list);
	cpcm->pcm = pcm;
	for (s = 0; s < 2; s++) {
		apcm->hinfo[s] = &cpcm->stream[s];
		if (cpcm->stream[s].substreams)
			snd_pcm_set_ops(pcm, s, &azx_pcm_ops);
	}
	/* buffer pre-allocation */
	size = CONFIG_SND_HDA_PREALLOC_SIZE * 1024;
	if (size > MAX_PREALLOC_SIZE)
		size = MAX_PREALLOC_SIZE;
	snd_pcm_lib_preallocate_pages_for_all(pcm, SNDRV_DMA_TYPE_DEV_SG,
					      chip->card->dev,
					      size, MAX_PREALLOC_SIZE);
	/* link to codec */
	pcm->dev = &codec->dev;
	return 0;
}
EXPORT_SYMBOL_GPL(azx_attach_pcm_stream);

/*
 * CORB / RIRB interface
 */
static int azx_alloc_cmd_io(struct azx *chip)
{
	int err;

	/* single page (at least 4096 bytes) must suffice for both ringbuffes */
	err = chip->ops->dma_alloc_pages(chip, SNDRV_DMA_TYPE_DEV,
					 PAGE_SIZE, &chip->rb);
	if (err < 0)
		dev_err(chip->card->dev, "cannot allocate CORB/RIRB\n");
	return err;
}
EXPORT_SYMBOL_GPL(azx_alloc_cmd_io);

static void azx_init_cmd_io(struct azx *chip)
{
	int timeout;

	spin_lock_irq(&chip->reg_lock);
	/* CORB set up */
	chip->corb.addr = chip->rb.addr;
	chip->corb.buf = (u32 *)chip->rb.area;
	azx_writel(chip, CORBLBASE, (u32)chip->corb.addr);
	azx_writel(chip, CORBUBASE, upper_32_bits(chip->corb.addr));

	/* set the corb size to 256 entries (ULI requires explicitly) */
	azx_writeb(chip, CORBSIZE, 0x02);
	/* set the corb write pointer to 0 */
	azx_writew(chip, CORBWP, 0);

	/* reset the corb hw read pointer */
	azx_writew(chip, CORBRP, ICH6_CORBRP_RST);
	for (timeout = 1000; timeout > 0; timeout--) {
		if ((azx_readw(chip, CORBRP) & ICH6_CORBRP_RST) == ICH6_CORBRP_RST)
			break;
		udelay(1);
	}
	if (timeout <= 0)
		dev_err(chip->card->dev, "CORB reset timeout#1, CORBRP = %d\n",
			azx_readw(chip, CORBRP));

	azx_writew(chip, CORBRP, 0);
	for (timeout = 1000; timeout > 0; timeout--) {
		if (azx_readw(chip, CORBRP) == 0)
			break;
		udelay(1);
	}
	if (timeout <= 0)
		dev_err(chip->card->dev, "CORB reset timeout#2, CORBRP = %d\n",
			azx_readw(chip, CORBRP));

	/* enable corb dma */
	azx_writeb(chip, CORBCTL, ICH6_CORBCTL_RUN);

	/* RIRB set up */
	chip->rirb.addr = chip->rb.addr + 2048;
	chip->rirb.buf = (u32 *)(chip->rb.area + 2048);
	chip->rirb.wp = chip->rirb.rp = 0;
	memset(chip->rirb.cmds, 0, sizeof(chip->rirb.cmds));
	azx_writel(chip, RIRBLBASE, (u32)chip->rirb.addr);
	azx_writel(chip, RIRBUBASE, upper_32_bits(chip->rirb.addr));

	/* set the rirb size to 256 entries (ULI requires explicitly) */
	azx_writeb(chip, RIRBSIZE, 0x02);
	/* reset the rirb hw write pointer */
	azx_writew(chip, RIRBWP, ICH6_RIRBWP_RST);
	/* set N=1, get RIRB response interrupt for new entry */
	if (chip->driver_caps & AZX_DCAPS_CTX_WORKAROUND)
		azx_writew(chip, RINTCNT, 0xc0);
	else
		azx_writew(chip, RINTCNT, 1);
	/* enable rirb dma and response irq */
	azx_writeb(chip, RIRBCTL, ICH6_RBCTL_DMA_EN | ICH6_RBCTL_IRQ_EN);
	spin_unlock_irq(&chip->reg_lock);
}
EXPORT_SYMBOL_GPL(azx_init_cmd_io);

static void azx_free_cmd_io(struct azx *chip)
{
	spin_lock_irq(&chip->reg_lock);
	/* disable ringbuffer DMAs */
	azx_writeb(chip, RIRBCTL, 0);
	azx_writeb(chip, CORBCTL, 0);
	spin_unlock_irq(&chip->reg_lock);
}
EXPORT_SYMBOL_GPL(azx_free_cmd_io);

static unsigned int azx_command_addr(u32 cmd)
{
	unsigned int addr = cmd >> 28;

	if (addr >= AZX_MAX_CODECS) {
		snd_BUG();
		addr = 0;
	}

	return addr;
}

/* send a command */
static int azx_corb_send_cmd(struct hda_bus *bus, u32 val)
{
	struct azx *chip = bus->private_data;
	unsigned int addr = azx_command_addr(val);
	unsigned int wp, rp;

	spin_lock_irq(&chip->reg_lock);

	/* add command to corb */
	wp = azx_readw(chip, CORBWP);
	if (wp == 0xffff) {
		/* something wrong, controller likely turned to D3 */
		spin_unlock_irq(&chip->reg_lock);
		return -EIO;
	}
	wp++;
	wp %= ICH6_MAX_CORB_ENTRIES;

	rp = azx_readw(chip, CORBRP);
	if (wp == rp) {
		/* oops, it's full */
		spin_unlock_irq(&chip->reg_lock);
		return -EAGAIN;
	}

	chip->rirb.cmds[addr]++;
	chip->corb.buf[wp] = cpu_to_le32(val);
	azx_writew(chip, CORBWP, wp);

	spin_unlock_irq(&chip->reg_lock);

	return 0;
}

#define ICH6_RIRB_EX_UNSOL_EV	(1<<4)

/* retrieve RIRB entry - called from interrupt handler */
static void azx_update_rirb(struct azx *chip)
{
	unsigned int rp, wp;
	unsigned int addr;
	u32 res, res_ex;

	wp = azx_readw(chip, RIRBWP);
	if (wp == 0xffff) {
		/* something wrong, controller likely turned to D3 */
		return;
	}

	if (wp == chip->rirb.wp)
		return;
	chip->rirb.wp = wp;

	while (chip->rirb.rp != wp) {
		chip->rirb.rp++;
		chip->rirb.rp %= ICH6_MAX_RIRB_ENTRIES;

		rp = chip->rirb.rp << 1; /* an RIRB entry is 8-bytes */
		res_ex = le32_to_cpu(chip->rirb.buf[rp + 1]);
		res = le32_to_cpu(chip->rirb.buf[rp]);
		addr = res_ex & 0xf;
		if ((addr >= AZX_MAX_CODECS) || !(chip->codec_mask & (1 << addr))) {
			dev_err(chip->card->dev, "spurious response %#x:%#x, rp = %d, wp = %d",
				res, res_ex,
				chip->rirb.rp, wp);
			snd_BUG();
		}
		else if (res_ex & ICH6_RIRB_EX_UNSOL_EV)
			snd_hda_queue_unsol_event(chip->bus, res, res_ex);
		else if (chip->rirb.cmds[addr]) {
			chip->rirb.res[addr] = res;
			smp_wmb();
			chip->rirb.cmds[addr]--;
		} else if (printk_ratelimit()) {
			dev_err(chip->card->dev, "spurious response %#x:%#x, last cmd=%#08x\n",
				res, res_ex,
				chip->last_cmd[addr]);
		}
	}
}

/* receive a response */
static unsigned int azx_rirb_get_response(struct hda_bus *bus,
					  unsigned int addr)
{
	struct azx *chip = bus->private_data;
	unsigned long timeout;
	unsigned long loopcounter;
	int do_poll = 0;

 again:
	timeout = jiffies + msecs_to_jiffies(1000);

	for (loopcounter = 0;; loopcounter++) {
		if (chip->polling_mode || do_poll) {
			spin_lock_irq(&chip->reg_lock);
			azx_update_rirb(chip);
			spin_unlock_irq(&chip->reg_lock);
		}
		if (!chip->rirb.cmds[addr]) {
			smp_rmb();
			bus->rirb_error = 0;

			if (!do_poll)
				chip->poll_count = 0;
			return chip->rirb.res[addr]; /* the last value */
		}
		if (time_after(jiffies, timeout))
			break;
		if (bus->needs_damn_long_delay || loopcounter > 3000)
			msleep(2); /* temporary workaround */
		else {
			udelay(10);
			cond_resched();
		}
	}

	if (!bus->no_response_fallback)
		return -1;

	if (!chip->polling_mode && chip->poll_count < 2) {
		dev_dbg(chip->card->dev,
			"azx_get_response timeout, polling the codec once: last cmd=0x%08x\n",
			chip->last_cmd[addr]);
		do_poll = 1;
		chip->poll_count++;
		goto again;
	}


	if (!chip->polling_mode) {
		dev_warn(chip->card->dev,
			 "azx_get_response timeout, switching to polling mode: last cmd=0x%08x\n",
			 chip->last_cmd[addr]);
		chip->polling_mode = 1;
		goto again;
	}

	if (chip->msi) {
		dev_warn(chip->card->dev,
			 "No response from codec, disabling MSI: last cmd=0x%08x\n",
			 chip->last_cmd[addr]);
		if (chip->ops->disable_msi_reset_irq(chip) &&
		    chip->ops->disable_msi_reset_irq(chip) < 0) {
			bus->rirb_error = 1;
			return -1;
		}
		goto again;
	}

	if (chip->probing) {
		/* If this critical timeout happens during the codec probing
		 * phase, this is likely an access to a non-existing codec
		 * slot.  Better to return an error and reset the system.
		 */
		return -1;
	}

	/* a fatal communication error; need either to reset or to fallback
	 * to the single_cmd mode
	 */
	bus->rirb_error = 1;
	if (bus->allow_bus_reset && !bus->response_reset && !bus->in_reset) {
		bus->response_reset = 1;
		return -1; /* give a chance to retry */
	}

	dev_err(chip->card->dev,
		"azx_get_response timeout, switching to single_cmd mode: last cmd=0x%08x\n",
		chip->last_cmd[addr]);
	chip->single_cmd = 1;
	bus->response_reset = 0;
	/* release CORB/RIRB */
	azx_free_cmd_io(chip);
	/* disable unsolicited responses */
	azx_writel(chip, GCTL, azx_readl(chip, GCTL) & ~ICH6_GCTL_UNSOL);
	return -1;
}

/*
 * Use the single immediate command instead of CORB/RIRB for simplicity
 *
 * Note: according to Intel, this is not preferred use.  The command was
 *       intended for the BIOS only, and may get confused with unsolicited
 *       responses.  So, we shouldn't use it for normal operation from the
 *       driver.
 *       I left the codes, however, for debugging/testing purposes.
 */

/* receive a response */
static int azx_single_wait_for_response(struct azx *chip, unsigned int addr)
{
	int timeout = 50;

	while (timeout--) {
		/* check IRV busy bit */
		if (azx_readw(chip, IRS) & ICH6_IRS_VALID) {
			/* reuse rirb.res as the response return value */
			chip->rirb.res[addr] = azx_readl(chip, IR);
			return 0;
		}
		udelay(1);
	}
	if (printk_ratelimit())
		dev_dbg(chip->card->dev, "get_response timeout: IRS=0x%x\n",
			azx_readw(chip, IRS));
	chip->rirb.res[addr] = -1;
	return -EIO;
}

/* send a command */
static int azx_single_send_cmd(struct hda_bus *bus, u32 val)
{
	struct azx *chip = bus->private_data;
	unsigned int addr = azx_command_addr(val);
	int timeout = 50;

	bus->rirb_error = 0;
	while (timeout--) {
		/* check ICB busy bit */
		if (!((azx_readw(chip, IRS) & ICH6_IRS_BUSY))) {
			/* Clear IRV valid bit */
			azx_writew(chip, IRS, azx_readw(chip, IRS) |
				   ICH6_IRS_VALID);
			azx_writel(chip, IC, val);
			azx_writew(chip, IRS, azx_readw(chip, IRS) |
				   ICH6_IRS_BUSY);
			return azx_single_wait_for_response(chip, addr);
		}
		udelay(1);
	}
	if (printk_ratelimit())
		dev_dbg(chip->card->dev,
			"send_cmd timeout: IRS=0x%x, val=0x%x\n",
			azx_readw(chip, IRS), val);
	return -EIO;
}

/* receive a response */
static unsigned int azx_single_get_response(struct hda_bus *bus,
					    unsigned int addr)
{
	struct azx *chip = bus->private_data;
	return chip->rirb.res[addr];
}

/*
 * The below are the main callbacks from hda_codec.
 *
 * They are just the skeleton to call sub-callbacks according to the
 * current setting of chip->single_cmd.
 */

/* send a command */
int azx_send_cmd(struct hda_bus *bus, unsigned int val)
{
	struct azx *chip = bus->private_data;

	if (chip->disabled)
		return 0;
	chip->last_cmd[azx_command_addr(val)] = val;
	if (chip->single_cmd)
		return azx_single_send_cmd(bus, val);
	else
		return azx_corb_send_cmd(bus, val);
}
EXPORT_SYMBOL_GPL(azx_send_cmd);

/* get a response */
unsigned int azx_get_response(struct hda_bus *bus,
				     unsigned int addr)
{
	struct azx *chip = bus->private_data;
	if (chip->disabled)
		return 0;
	if (chip->single_cmd)
		return azx_single_get_response(bus, addr);
	else
		return azx_rirb_get_response(bus, addr);
}
EXPORT_SYMBOL_GPL(azx_get_response);

#ifdef CONFIG_SND_HDA_DSP_LOADER
/*
 * DSP loading code (e.g. for CA0132)
 */

/* use the first stream for loading DSP */
static struct azx_dev *
azx_get_dsp_loader_dev(struct azx *chip)
{
	return &chip->azx_dev[chip->playback_index_offset];
}

int azx_load_dsp_prepare(struct hda_bus *bus, unsigned int format,
			 unsigned int byte_size,
			 struct snd_dma_buffer *bufp)
{
	u32 *bdl;
	struct azx *chip = bus->private_data;
	struct azx_dev *azx_dev;
	int err;

	azx_dev = azx_get_dsp_loader_dev(chip);

	dsp_lock(azx_dev);
	spin_lock_irq(&chip->reg_lock);
	if (azx_dev->running || azx_dev->locked) {
		spin_unlock_irq(&chip->reg_lock);
		err = -EBUSY;
		goto unlock;
	}
	azx_dev->prepared = 0;
	chip->saved_azx_dev = *azx_dev;
	azx_dev->locked = 1;
	spin_unlock_irq(&chip->reg_lock);

	err = chip->ops->dma_alloc_pages(chip, SNDRV_DMA_TYPE_DEV_SG,
					 byte_size, bufp);
	if (err < 0)
		goto err_alloc;

	azx_dev->bufsize = byte_size;
	azx_dev->period_bytes = byte_size;
	azx_dev->format_val = format;

	azx_stream_reset(chip, azx_dev);

	/* reset BDL address */
	azx_sd_writel(chip, azx_dev, SD_BDLPL, 0);
	azx_sd_writel(chip, azx_dev, SD_BDLPU, 0);

	azx_dev->frags = 0;
	bdl = (u32 *)azx_dev->bdl.area;
	err = setup_bdle(chip, bufp, azx_dev, &bdl, 0, byte_size, 0);
	if (err < 0)
		goto error;

	azx_setup_controller(chip, azx_dev);
	dsp_unlock(azx_dev);
	return azx_dev->stream_tag;

 error:
	chip->ops->dma_free_pages(chip, bufp);
 err_alloc:
	spin_lock_irq(&chip->reg_lock);
	if (azx_dev->opened)
		*azx_dev = chip->saved_azx_dev;
	azx_dev->locked = 0;
	spin_unlock_irq(&chip->reg_lock);
 unlock:
	dsp_unlock(azx_dev);
	return err;
}
EXPORT_SYMBOL_GPL(azx_load_dsp_prepare);

void azx_load_dsp_trigger(struct hda_bus *bus, bool start)
{
	struct azx *chip = bus->private_data;
	struct azx_dev *azx_dev = azx_get_dsp_loader_dev(chip);

	if (start)
		azx_stream_start(chip, azx_dev);
	else
		azx_stream_stop(chip, azx_dev);
	azx_dev->running = start;
}
EXPORT_SYMBOL_GPL(azx_load_dsp_trigger);

void azx_load_dsp_cleanup(struct hda_bus *bus,
			  struct snd_dma_buffer *dmab)
{
	struct azx *chip = bus->private_data;
	struct azx_dev *azx_dev = azx_get_dsp_loader_dev(chip);

	if (!dmab->area || !azx_dev->locked)
		return;

	dsp_lock(azx_dev);
	/* reset BDL address */
	azx_sd_writel(chip, azx_dev, SD_BDLPL, 0);
	azx_sd_writel(chip, azx_dev, SD_BDLPU, 0);
	azx_sd_writel(chip, azx_dev, SD_CTL, 0);
	azx_dev->bufsize = 0;
	azx_dev->period_bytes = 0;
	azx_dev->format_val = 0;

	chip->ops->dma_free_pages(chip, dmab);
	dmab->area = NULL;

	spin_lock_irq(&chip->reg_lock);
	if (azx_dev->opened)
		*azx_dev = chip->saved_azx_dev;
	azx_dev->locked = 0;
	spin_unlock_irq(&chip->reg_lock);
	dsp_unlock(azx_dev);
}
EXPORT_SYMBOL_GPL(azx_load_dsp_cleanup);
#endif /* CONFIG_SND_HDA_DSP_LOADER */

int azx_alloc_stream_pages(struct azx *chip)
{
	int i, err;
	struct snd_card *card = chip->card;

	for (i = 0; i < chip->num_streams; i++) {
		dsp_lock_init(&chip->azx_dev[i]);
		/* allocate memory for the BDL for each stream */
		err = chip->ops->dma_alloc_pages(chip, SNDRV_DMA_TYPE_DEV,
						 BDL_SIZE,
						 &chip->azx_dev[i].bdl);
		if (err < 0) {
			dev_err(card->dev, "cannot allocate BDL\n");
			return -ENOMEM;
		}
	}
	/* allocate memory for the position buffer */
	err = chip->ops->dma_alloc_pages(chip, SNDRV_DMA_TYPE_DEV,
					 chip->num_streams * 8, &chip->posbuf);
	if (err < 0) {
		dev_err(card->dev, "cannot allocate posbuf\n");
		return -ENOMEM;
	}

	/* allocate CORB/RIRB */
	err = azx_alloc_cmd_io(chip);
	if (err < 0)
		return err;
	return 0;
}
EXPORT_SYMBOL_GPL(azx_alloc_stream_pages);

void azx_free_stream_pages(struct azx *chip)
{
	int i;
	if (chip->azx_dev) {
		for (i = 0; i < chip->num_streams; i++)
			if (chip->azx_dev[i].bdl.area)
				chip->ops->dma_free_pages(
					chip, &chip->azx_dev[i].bdl);
	}
	if (chip->rb.area)
		chip->ops->dma_free_pages(chip, &chip->rb);
	if (chip->posbuf.area)
		chip->ops->dma_free_pages(chip, &chip->posbuf);
}
EXPORT_SYMBOL_GPL(azx_free_stream_pages);

/*
 * Lowlevel interface
 */

/* enter link reset */
void azx_enter_link_reset(struct azx *chip)
{
	unsigned long timeout;

	/* reset controller */
	azx_writel(chip, GCTL, azx_readl(chip, GCTL) & ~ICH6_GCTL_RESET);

	timeout = jiffies + msecs_to_jiffies(100);
	while ((azx_readb(chip, GCTL) & ICH6_GCTL_RESET) &&
			time_before(jiffies, timeout))
		usleep_range(500, 1000);
}
EXPORT_SYMBOL_GPL(azx_enter_link_reset);

/* exit link reset */
static void azx_exit_link_reset(struct azx *chip)
{
	unsigned long timeout;

	azx_writeb(chip, GCTL, azx_readb(chip, GCTL) | ICH6_GCTL_RESET);

	timeout = jiffies + msecs_to_jiffies(100);
	while (!azx_readb(chip, GCTL) &&
			time_before(jiffies, timeout))
		usleep_range(500, 1000);
}

/* reset codec link */
static int azx_reset(struct azx *chip, int full_reset)
{
	if (!full_reset)
		goto __skip;

	/* clear STATESTS */
	azx_writew(chip, STATESTS, STATESTS_INT_MASK);

	/* reset controller */
	azx_enter_link_reset(chip);

	/* delay for >= 100us for codec PLL to settle per spec
	 * Rev 0.9 section 5.5.1
	 */
	usleep_range(500, 1000);

	/* Bring controller out of reset */
	azx_exit_link_reset(chip);

	/* Brent Chartrand said to wait >= 540us for codecs to initialize */
	usleep_range(1000, 1200);

      __skip:
	/* check to see if controller is ready */
	if (!azx_readb(chip, GCTL)) {
		dev_dbg(chip->card->dev, "azx_reset: controller not ready!\n");
		return -EBUSY;
	}

	/* Accept unsolicited responses */
	if (!chip->single_cmd)
		azx_writel(chip, GCTL, azx_readl(chip, GCTL) |
			   ICH6_GCTL_UNSOL);

	/* detect codecs */
	if (!chip->codec_mask) {
		chip->codec_mask = azx_readw(chip, STATESTS);
		dev_dbg(chip->card->dev, "codec_mask = 0x%x\n",
			chip->codec_mask);
	}

	return 0;
}

/* enable interrupts */
static void azx_int_enable(struct azx *chip)
{
	/* enable controller CIE and GIE */
	azx_writel(chip, INTCTL, azx_readl(chip, INTCTL) |
		   ICH6_INT_CTRL_EN | ICH6_INT_GLOBAL_EN);
}

/* disable interrupts */
static void azx_int_disable(struct azx *chip)
{
	int i;

	/* disable interrupts in stream descriptor */
	for (i = 0; i < chip->num_streams; i++) {
		struct azx_dev *azx_dev = &chip->azx_dev[i];
		azx_sd_writeb(chip, azx_dev, SD_CTL,
			      azx_sd_readb(chip, azx_dev, SD_CTL) &
					~SD_INT_MASK);
	}

	/* disable SIE for all streams */
	azx_writeb(chip, INTCTL, 0);

	/* disable controller CIE and GIE */
	azx_writel(chip, INTCTL, azx_readl(chip, INTCTL) &
		   ~(ICH6_INT_CTRL_EN | ICH6_INT_GLOBAL_EN));
}

/* clear interrupts */
static void azx_int_clear(struct azx *chip)
{
	int i;

	/* clear stream status */
	for (i = 0; i < chip->num_streams; i++) {
		struct azx_dev *azx_dev = &chip->azx_dev[i];
		azx_sd_writeb(chip, azx_dev, SD_STS, SD_INT_MASK);
	}

	/* clear STATESTS */
	azx_writew(chip, STATESTS, STATESTS_INT_MASK);

	/* clear rirb status */
	azx_writeb(chip, RIRBSTS, RIRB_INT_MASK);

	/* clear int status */
	azx_writel(chip, INTSTS, ICH6_INT_CTRL_EN | ICH6_INT_ALL_STREAM);
}

/*
 * reset and start the controller registers
 */
void azx_init_chip(struct azx *chip, int full_reset)
{
	if (chip->initialized)
		return;

	/* reset controller */
	azx_reset(chip, full_reset);

	/* initialize interrupts */
	azx_int_clear(chip);
	azx_int_enable(chip);

	/* initialize the codec command I/O */
	if (!chip->single_cmd)
		azx_init_cmd_io(chip);

	/* program the position buffer */
	azx_writel(chip, DPLBASE, (u32)chip->posbuf.addr);
	azx_writel(chip, DPUBASE, upper_32_bits(chip->posbuf.addr));

	chip->initialized = 1;
}
EXPORT_SYMBOL_GPL(azx_init_chip);

void azx_stop_chip(struct azx *chip)
{
	if (!chip->initialized)
		return;

	/* disable interrupts */
	azx_int_disable(chip);
	azx_int_clear(chip);

	/* disable CORB/RIRB */
	azx_free_cmd_io(chip);

	/* disable position buffer */
	azx_writel(chip, DPLBASE, 0);
	azx_writel(chip, DPUBASE, 0);

	chip->initialized = 0;
}

/*
 * interrupt handler
 */
irqreturn_t azx_interrupt(int irq, void *dev_id)
{
	struct azx *chip = dev_id;
	struct azx_dev *azx_dev;
	u32 status;
	u8 sd_status;
	int i;

#ifdef CONFIG_PM_RUNTIME
	if (chip->driver_caps & AZX_DCAPS_PM_RUNTIME)
		if (chip->card->dev->power.runtime_status != RPM_ACTIVE)
			return IRQ_NONE;
#endif

	spin_lock(&chip->reg_lock);

	if (chip->disabled) {
		spin_unlock(&chip->reg_lock);
		return IRQ_NONE;
	}

	status = azx_readl(chip, INTSTS);
	if (status == 0 || status == 0xffffffff) {
		spin_unlock(&chip->reg_lock);
		return IRQ_NONE;
	}

	for (i = 0; i < chip->num_streams; i++) {
		azx_dev = &chip->azx_dev[i];
		if (status & azx_dev->sd_int_sta_mask) {
			sd_status = azx_sd_readb(chip, azx_dev, SD_STS);
			azx_sd_writeb(chip, azx_dev, SD_STS, SD_INT_MASK);
			if (!azx_dev->substream || !azx_dev->running ||
			    !(sd_status & SD_INT_COMPLETE))
				continue;
			/* check whether this IRQ is really acceptable */
			if (!chip->ops->position_check ||
			    chip->ops->position_check(chip, azx_dev)) {
				spin_unlock(&chip->reg_lock);
				snd_pcm_period_elapsed(azx_dev->substream);
				spin_lock(&chip->reg_lock);
			}
		}
	}

	/* clear rirb int */
	status = azx_readb(chip, RIRBSTS);
	if (status & RIRB_INT_MASK) {
		if (status & RIRB_INT_RESPONSE) {
			if (chip->driver_caps & AZX_DCAPS_RIRB_PRE_DELAY)
				udelay(80);
			azx_update_rirb(chip);
		}
		azx_writeb(chip, RIRBSTS, RIRB_INT_MASK);
	}

	spin_unlock(&chip->reg_lock);

	return IRQ_HANDLED;
}
EXPORT_SYMBOL_GPL(azx_interrupt);

MODULE_LICENSE("GPL");
MODULE_DESCRIPTION("Common HDA driver funcitons");
