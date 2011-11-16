/*
 * Jack-detection handling for HD-audio
 *
 * Copyright (c) 2011 Takashi Iwai <tiwai@suse.de>
 *
 * This driver is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 */

#include <linux/init.h>
#include <linux/slab.h>
#include <linux/export.h>
#include <sound/core.h>
#include <sound/control.h>
#include <sound/jack.h>
#include "hda_codec.h"
#include "hda_local.h"
#include "hda_jack.h"

/* execute pin sense measurement */
static u32 read_pin_sense(struct hda_codec *codec, hda_nid_t nid)
{
	u32 pincap;

	if (!codec->no_trigger_sense) {
		pincap = snd_hda_query_pin_caps(codec, nid);
		if (pincap & AC_PINCAP_TRIG_REQ) /* need trigger? */
			snd_hda_codec_read(codec, nid, 0,
					AC_VERB_SET_PIN_SENSE, 0);
	}
	return snd_hda_codec_read(codec, nid, 0,
				  AC_VERB_GET_PIN_SENSE, 0);
}

/**
 * snd_hda_jack_tbl_get - query the jack-table entry for the given NID
 */
struct hda_jack_tbl *
snd_hda_jack_tbl_get(struct hda_codec *codec, hda_nid_t nid)
{
	struct hda_jack_tbl *jack = codec->jacktbl.list;
	int i;

	if (!nid || !jack)
		return NULL;
	for (i = 0; i < codec->jacktbl.used; i++, jack++)
		if (jack->nid == nid)
			return jack;
	return NULL;
}
EXPORT_SYMBOL_HDA(snd_hda_jack_tbl_get);

/**
 * snd_hda_jack_tbl_get_from_tag - query the jack-table entry for the given tag
 */
struct hda_jack_tbl *
snd_hda_jack_tbl_get_from_tag(struct hda_codec *codec, unsigned char tag)
{
	struct hda_jack_tbl *jack = codec->jacktbl.list;
	int i;

	if (!tag || !jack)
		return NULL;
	for (i = 0; i < codec->jacktbl.used; i++, jack++)
		if (jack->tag == tag)
			return jack;
	return NULL;
}
EXPORT_SYMBOL_HDA(snd_hda_jack_tbl_get_from_tag);

/**
 * snd_hda_jack_tbl_new - create a jack-table entry for the given NID
 */
struct hda_jack_tbl *
snd_hda_jack_tbl_new(struct hda_codec *codec, hda_nid_t nid)
{
	struct hda_jack_tbl *jack = snd_hda_jack_tbl_get(codec, nid);
	if (jack)
		return jack;
	snd_array_init(&codec->jacktbl, sizeof(*jack), 16);
	jack = snd_array_new(&codec->jacktbl);
	if (!jack)
		return NULL;
	jack->nid = nid;
	jack->jack_dirty = 1;
	jack->tag = codec->jacktbl.used;
	return jack;
}
EXPORT_SYMBOL_HDA(snd_hda_jack_tbl_new);

#ifdef CONFIG_SND_HDA_INPUT_JACK
static void snd_hda_input_jack_free(struct hda_codec *codec);
#else
#define snd_hda_input_jack_free(codec)
#endif

void snd_hda_jack_tbl_clear(struct hda_codec *codec)
{
	snd_hda_input_jack_free(codec);
	snd_array_free(&codec->jacktbl);
}

/* update the cached value and notification flag if needed */
static void jack_detect_update(struct hda_codec *codec,
			       struct hda_jack_tbl *jack)
{
	if (jack->jack_dirty || !jack->jack_detect) {
		jack->pin_sense = read_pin_sense(codec, jack->nid);
		jack->jack_dirty = 0;
	}
}

/**
 * snd_hda_set_dirty_all - Mark all the cached as dirty
 *
 * This function sets the dirty flag to all entries of jack table.
 * It's called from the resume path in hda_codec.c.
 */
void snd_hda_jack_set_dirty_all(struct hda_codec *codec)
{
	struct hda_jack_tbl *jack = codec->jacktbl.list;
	int i;

	for (i = 0; i < codec->jacktbl.used; i++, jack++)
		if (jack->nid)
			jack->jack_dirty = 1;
}
EXPORT_SYMBOL_HDA(snd_hda_jack_set_dirty_all);

/**
 * snd_hda_pin_sense - execute pin sense measurement
 * @codec: the CODEC to sense
 * @nid: the pin NID to sense
 *
 * Execute necessary pin sense measurement and return its Presence Detect,
 * Impedance, ELD Valid etc. status bits.
 */
u32 snd_hda_pin_sense(struct hda_codec *codec, hda_nid_t nid)
{
	struct hda_jack_tbl *jack = snd_hda_jack_tbl_get(codec, nid);
	if (jack) {
		jack_detect_update(codec, jack);
		return jack->pin_sense;
	}
	return read_pin_sense(codec, nid);
}
EXPORT_SYMBOL_HDA(snd_hda_pin_sense);

#define get_jack_plug_state(sense) !!(sense & AC_PINSENSE_PRESENCE)

/**
 * snd_hda_jack_detect - query pin Presence Detect status
 * @codec: the CODEC to sense
 * @nid: the pin NID to sense
 *
 * Query and return the pin's Presence Detect status.
 */
int snd_hda_jack_detect(struct hda_codec *codec, hda_nid_t nid)
{
	u32 sense = snd_hda_pin_sense(codec, nid);
	return get_jack_plug_state(sense);
}
EXPORT_SYMBOL_HDA(snd_hda_jack_detect);

/**
 * snd_hda_jack_detect_enable - enable the jack-detection
 */
int snd_hda_jack_detect_enable(struct hda_codec *codec, hda_nid_t nid,
			       unsigned char action)
{
	struct hda_jack_tbl *jack = snd_hda_jack_tbl_new(codec, nid);
	if (!jack)
		return -ENOMEM;
	if (jack->jack_detect)
		return 0; /* already registered */
	jack->jack_detect = 1;
	if (action)
		jack->action = action;
	return snd_hda_codec_write_cache(codec, nid, 0,
					 AC_VERB_SET_UNSOLICITED_ENABLE,
					 AC_USRSP_EN | jack->tag);
}
EXPORT_SYMBOL_HDA(snd_hda_jack_detect_enable);

/**
 * snd_hda_jack_report_sync - sync the states of all jacks and report if changed
 */
void snd_hda_jack_report_sync(struct hda_codec *codec)
{
	struct hda_jack_tbl *jack = codec->jacktbl.list;
	int i, state;

	for (i = 0; i < codec->jacktbl.used; i++, jack++)
		if (jack->nid) {
			jack_detect_update(codec, jack);
			if (!jack->kctl)
				continue;
			state = get_jack_plug_state(jack->pin_sense);
			snd_kctl_jack_report(codec->bus->card, jack->kctl, state);
		}
}
EXPORT_SYMBOL_HDA(snd_hda_jack_report_sync);

/**
 * snd_hda_jack_add_kctl - Add a kctl for the given pin
 *
 * This assigns a jack-detection kctl to the given pin.  The kcontrol
 * will have the given name and index.
 */
int snd_hda_jack_add_kctl(struct hda_codec *codec, hda_nid_t nid,
			  const char *name, int idx)
{
	struct hda_jack_tbl *jack;
	struct snd_kcontrol *kctl;

	jack = snd_hda_jack_tbl_new(codec, nid);
	if (!jack)
		return 0;
	if (jack->kctl)
		return 0; /* already created */
	kctl = snd_kctl_jack_new(name, idx, codec);
	if (!kctl)
		return -ENOMEM;
	if (snd_hda_ctl_add(codec, nid, kctl) < 0)
		return -ENOMEM;
	jack->kctl = kctl;
	snd_kctl_jack_report(codec->bus->card, kctl,
			     snd_hda_jack_detect(codec, nid));
	return 0;
}
EXPORT_SYMBOL_HDA(snd_hda_jack_add_kctl);

static int add_jack_kctl(struct hda_codec *codec, hda_nid_t nid,
			 const struct auto_pin_cfg *cfg)
{
	unsigned int def_conf, conn;
	char name[44];
	int idx, err;

	if (!nid)
		return 0;
	if (!is_jack_detectable(codec, nid))
		return 0;
	def_conf = snd_hda_codec_get_pincfg(codec, nid);
	conn = get_defcfg_connect(def_conf);
	if (conn != AC_JACK_PORT_COMPLEX)
		return 0;

	snd_hda_get_pin_label(codec, nid, cfg, name, sizeof(name), &idx);
	err = snd_hda_jack_add_kctl(codec, nid, name, idx);
	if (err < 0)
		return err;
	return snd_hda_jack_detect_enable(codec, nid, 0);
}

/**
 * snd_hda_jack_add_kctls - Add kctls for all pins included in the given pincfg
 */
int snd_hda_jack_add_kctls(struct hda_codec *codec,
			   const struct auto_pin_cfg *cfg)
{
	const hda_nid_t *p;
	int i, err;

	for (i = 0, p = cfg->line_out_pins; i < cfg->line_outs; i++, p++) {
		err = add_jack_kctl(codec, *p, cfg);
		if (err < 0)
			return err;
	}
	for (i = 0, p = cfg->hp_pins; i < cfg->hp_outs; i++, p++) {
		if (*p == *cfg->line_out_pins) /* might be duplicated */
			break;
		err = add_jack_kctl(codec, *p, cfg);
		if (err < 0)
			return err;
	}
	for (i = 0, p = cfg->speaker_pins; i < cfg->speaker_outs; i++, p++) {
		if (*p == *cfg->line_out_pins) /* might be duplicated */
			break;
		err = add_jack_kctl(codec, *p, cfg);
		if (err < 0)
			return err;
	}
	for (i = 0; i < cfg->num_inputs; i++) {
		err = add_jack_kctl(codec, cfg->inputs[i].pin, cfg);
		if (err < 0)
			return err;
	}
	for (i = 0, p = cfg->dig_out_pins; i < cfg->dig_outs; i++, p++) {
		err = add_jack_kctl(codec, *p, cfg);
		if (err < 0)
			return err;
	}
	err = add_jack_kctl(codec, cfg->dig_in_pin, cfg);
	if (err < 0)
		return err;
	err = add_jack_kctl(codec, cfg->mono_out_pin, cfg);
	if (err < 0)
		return err;
	return 0;
}
EXPORT_SYMBOL_HDA(snd_hda_jack_add_kctls);

#ifdef CONFIG_SND_HDA_INPUT_JACK
/*
 * Input-jack notification support
 */
static const char *get_jack_default_name(struct hda_codec *codec, hda_nid_t nid,
					 int type)
{
	switch (type) {
	case SND_JACK_HEADPHONE:
		return "Headphone";
	case SND_JACK_MICROPHONE:
		return "Mic";
	case SND_JACK_LINEOUT:
		return "Line-out";
	case SND_JACK_LINEIN:
		return "Line-in";
	case SND_JACK_HEADSET:
		return "Headset";
	case SND_JACK_VIDEOOUT:
		return "HDMI/DP";
	default:
		return "Misc";
	}
}

static void hda_free_jack_priv(struct snd_jack *jack)
{
	struct hda_jack_tbl *jacks = jack->private_data;
	jacks->nid = 0;
	jacks->jack = NULL;
}

int snd_hda_input_jack_add(struct hda_codec *codec, hda_nid_t nid, int type,
			   const char *name)
{
	struct hda_jack_tbl *jack = snd_hda_jack_tbl_new(codec, nid);
	int err;

	if (!jack)
		return -ENOMEM;
	if (!name)
		name = get_jack_default_name(codec, nid, type);
	err = snd_jack_new(codec->bus->card, name, type, &jack->jack);
	if (err < 0)
		return err;
	jack->type = type;
	jack->jack->private_data = jack;
	jack->jack->private_free = hda_free_jack_priv;
	return 0;
}
EXPORT_SYMBOL_HDA(snd_hda_input_jack_add);

void snd_hda_input_jack_report(struct hda_codec *codec, hda_nid_t nid)
{
	struct hda_jack_tbl *jack = snd_hda_jack_tbl_get(codec, nid);
	unsigned int pin_ctl;
	unsigned int present;
	int type;

	if (!jack || !jack->jack)
		return;

	present = snd_hda_jack_detect(codec, nid);
	type = jack->type;
	if (type == (SND_JACK_HEADPHONE | SND_JACK_LINEOUT)) {
		pin_ctl = snd_hda_codec_read(codec, nid, 0,
					     AC_VERB_GET_PIN_WIDGET_CONTROL, 0);
		type = (pin_ctl & AC_PINCTL_HP_EN) ?
			SND_JACK_HEADPHONE : SND_JACK_LINEOUT;
	}
	snd_jack_report(jack->jack, present ? type : 0);
}
EXPORT_SYMBOL_HDA(snd_hda_input_jack_report);

/* free jack instances manually when clearing/reconfiguring */
static void snd_hda_input_jack_free(struct hda_codec *codec)
{
	if (!codec->bus->shutdown && codec->jacktbl.list) {
		struct hda_jack_tbl *jack = codec->jacktbl.list;
		int i;
		for (i = 0; i < codec->jacktbl.used; i++, jack++) {
			if (jack->jack)
				snd_device_free(codec->bus->card, jack->jack);
		}
	}
}
#endif /* CONFIG_SND_HDA_INPUT_JACK */
