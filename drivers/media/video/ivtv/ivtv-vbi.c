/*
    Vertical Blank Interval support functions
    Copyright (C) 2004-2007  Hans Verkuil <hverkuil@xs4all.nl>

    This program is free software; you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation; either version 2 of the License, or
    (at your option) any later version.

    This program is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with this program; if not, write to the Free Software
    Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 */

#include "ivtv-driver.h"
#include "ivtv-video.h"
#include "ivtv-vbi.h"
#include "ivtv-ioctl.h"
#include "ivtv-queue.h"

static int odd_parity(u8 c)
{
	c ^= (c >> 4);
	c ^= (c >> 2);
	c ^= (c >> 1);

	return c & 1;
}

static void passthrough_vbi_data(struct ivtv *itv, int cnt)
{
	int wss = 0;
	u8 cc[4] = { 0x80, 0x80, 0x80, 0x80 };
	u8 vps[13];
	int found_cc = 0;
	int found_wss = 0;
	int found_vps = 0;
	int cc_pos = itv->vbi.cc_pos;
	int i;

	for (i = 0; i < cnt; i++) {
		struct v4l2_sliced_vbi_data *d = itv->vbi.sliced_dec_data + i;

		if (d->id == V4L2_SLICED_CAPTION_525 && d->line == 21) {
			found_cc = 1;
			if (d->field) {
				cc[2] = d->data[0];
				cc[3] = d->data[1];
			} else {
				cc[0] = d->data[0];
				cc[1] = d->data[1];
			}
		}
		else if (d->id == V4L2_SLICED_VPS && d->line == 16 && d->field == 0) {
			memcpy(vps, d->data, sizeof(vps));
			found_vps = 1;
		}
		else if (d->id == V4L2_SLICED_WSS_625 && d->line == 23 && d->field == 0) {
			wss = d->data[0] | d->data[1] << 8;
			found_wss = 1;
		}
	}

	if (itv->vbi.wss_found != found_wss || itv->vbi.wss != wss) {
		itv->vbi.wss = wss;
		itv->vbi.wss_found = found_wss;
		set_bit(IVTV_F_I_UPDATE_WSS, &itv->i_flags);
	}

	if (found_vps || itv->vbi.vps_found) {
		itv->vbi.vps[0] = vps[2];
		itv->vbi.vps[1] = vps[8];
		itv->vbi.vps[2] = vps[9];
		itv->vbi.vps[3] = vps[10];
		itv->vbi.vps[4] = vps[11];
		itv->vbi.vps_found = found_vps;
		set_bit(IVTV_F_I_UPDATE_VPS, &itv->i_flags);
	}

	if (found_cc && cc_pos < sizeof(itv->vbi.cc_data_even)) {
		itv->vbi.cc_data_odd[cc_pos] = cc[0];
		itv->vbi.cc_data_odd[cc_pos + 1] = cc[1];
		itv->vbi.cc_data_even[cc_pos] = cc[2];
		itv->vbi.cc_data_even[cc_pos + 1] = cc[3];
		itv->vbi.cc_pos = cc_pos + 2;
		set_bit(IVTV_F_I_UPDATE_CC, &itv->i_flags);
	}
}

static void copy_vbi_data(struct ivtv *itv, int lines, u32 pts_stamp)
{
	int line = 0;
	int i;
	u32 linemask[2] = { 0, 0 };
	unsigned short size;
	static const u8 mpeg_hdr_data[] = {
		0x00, 0x00, 0x01, 0xba, 0x44, 0x00, 0x0c, 0x66,
		0x24, 0x01, 0x01, 0xd1, 0xd3, 0xfa, 0xff, 0xff,
		0x00, 0x00, 0x01, 0xbd, 0x00, 0x1a, 0x84, 0x80,
		0x07, 0x21, 0x00, 0x5d, 0x63, 0xa7, 0xff, 0xff
	};
	const int sd = sizeof(mpeg_hdr_data);	/* start of vbi data */
	int idx = itv->vbi.frame % IVTV_VBI_FRAMES;
	u8 *dst = &itv->vbi.sliced_mpeg_data[idx][0];

	for (i = 0; i < lines; i++) {
		int f, l;

		if (itv->vbi.sliced_data[i].id == 0)
			continue;

		l = itv->vbi.sliced_data[i].line - 6;
		f = itv->vbi.sliced_data[i].field;
		if (f)
			l += 18;
		if (l < 32)
			linemask[0] |= (1 << l);
		else
			linemask[1] |= (1 << (l - 32));
		dst[sd + 12 + line * 43] = service2vbi(itv->vbi.sliced_data[i].id);
		memcpy(dst + sd + 12 + line * 43 + 1, itv->vbi.sliced_data[i].data, 42);
		line++;
	}
	memcpy(dst, mpeg_hdr_data, sizeof(mpeg_hdr_data));
	if (line == 36) {
		/* All lines are used, so there is no space for the linemask
		   (the max size of the VBI data is 36 * 43 + 4 bytes).
		   So in this case we use the magic number 'ITV0'. */
		memcpy(dst + sd, "ITV0", 4);
		memcpy(dst + sd + 4, dst + sd + 12, line * 43);
		size = 4 + ((43 * line + 3) & ~3);
	} else {
		memcpy(dst + sd, "itv0", 4);
		memcpy(dst + sd + 4, &linemask[0], 8);
		size = 12 + ((43 * line + 3) & ~3);
	}
	dst[4+16] = (size + 10) >> 8;
	dst[5+16] = (size + 10) & 0xff;
	dst[9+16] = 0x21 | ((pts_stamp >> 29) & 0x6);
	dst[10+16] = (pts_stamp >> 22) & 0xff;
	dst[11+16] = 1 | ((pts_stamp >> 14) & 0xff);
	dst[12+16] = (pts_stamp >> 7) & 0xff;
	dst[13+16] = 1 | ((pts_stamp & 0x7f) << 1);
	itv->vbi.sliced_mpeg_size[idx] = sd + size;
}

static int ivtv_convert_ivtv_vbi(struct ivtv *itv, u8 *p)
{
	u32 linemask[2];
	int i, l, id2;
	int line = 0;

	if (!memcmp(p, "itv0", 4)) {
		memcpy(linemask, p + 4, 8);
		p += 12;
	} else if (!memcmp(p, "ITV0", 4)) {
		linemask[0] = 0xffffffff;
		linemask[1] = 0xf;
		p += 4;
	} else {
		/* unknown VBI data stream */
		return 0;
	}
	for (i = 0; i < 36; i++) {
		int err = 0;

		if (i < 32 && !(linemask[0] & (1 << i)))
			continue;
		if (i >= 32 && !(linemask[1] & (1 << (i - 32))))
			continue;
		id2 = *p & 0xf;
		switch (id2) {
		case IVTV_SLICED_TYPE_TELETEXT_B:
			id2 = V4L2_SLICED_TELETEXT_B;
			break;
		case IVTV_SLICED_TYPE_CAPTION_525:
			id2 = V4L2_SLICED_CAPTION_525;
			err = !odd_parity(p[1]) || !odd_parity(p[2]);
			break;
		case IVTV_SLICED_TYPE_VPS:
			id2 = V4L2_SLICED_VPS;
			break;
		case IVTV_SLICED_TYPE_WSS_625:
			id2 = V4L2_SLICED_WSS_625;
			break;
		default:
			id2 = 0;
			break;
		}
		if (err == 0) {
			l = (i < 18) ? i + 6 : i - 18 + 6;
			itv->vbi.sliced_dec_data[line].line = l;
			itv->vbi.sliced_dec_data[line].field = i >= 18;
			itv->vbi.sliced_dec_data[line].id = id2;
			memcpy(itv->vbi.sliced_dec_data[line].data, p + 1, 42);
			line++;
		}
		p += 43;
	}
	while (line < 36) {
		itv->vbi.sliced_dec_data[line].id = 0;
		itv->vbi.sliced_dec_data[line].line = 0;
		itv->vbi.sliced_dec_data[line].field = 0;
		line++;
	}
	return line * sizeof(itv->vbi.sliced_dec_data[0]);
}

ssize_t ivtv_write_vbi(struct ivtv *itv, const char __user *ubuf, size_t count)
{
	/* Should be a __user pointer, but sparse doesn't parse this bit correctly. */
	const struct v4l2_sliced_vbi_data *p = (const struct v4l2_sliced_vbi_data *)ubuf;
	u8 cc[4] = { 0x80, 0x80, 0x80, 0x80 };
	int found_cc = 0;
	int cc_pos = itv->vbi.cc_pos;

	if (itv->vbi.service_set_out == 0)
		return -EPERM;

	while (count >= sizeof(struct v4l2_sliced_vbi_data)) {
		switch (p->id) {
		case V4L2_SLICED_CAPTION_525:
			if (p->id == V4L2_SLICED_CAPTION_525 &&
			    p->line == 21 &&
			    (itv->vbi.service_set_out &
				V4L2_SLICED_CAPTION_525) == 0) {
				break;
			}
			found_cc = 1;
			if (p->field) {
				cc[2] = p->data[0];
				cc[3] = p->data[1];
			} else {
				cc[0] = p->data[0];
				cc[1] = p->data[1];
			}
			break;

		case V4L2_SLICED_VPS:
			if (p->line == 16 && p->field == 0 &&
			    (itv->vbi.service_set_out & V4L2_SLICED_VPS)) {
				itv->vbi.vps[0] = p->data[2];
				itv->vbi.vps[1] = p->data[8];
				itv->vbi.vps[2] = p->data[9];
				itv->vbi.vps[3] = p->data[10];
				itv->vbi.vps[4] = p->data[11];
				itv->vbi.vps_found = 1;
				set_bit(IVTV_F_I_UPDATE_VPS, &itv->i_flags);
			}
			break;

		case V4L2_SLICED_WSS_625:
			if (p->line == 23 && p->field == 0 &&
			    (itv->vbi.service_set_out & V4L2_SLICED_WSS_625)) {
				/* No lock needed for WSS */
				itv->vbi.wss = p->data[0] | (p->data[1] << 8);
				itv->vbi.wss_found = 1;
				set_bit(IVTV_F_I_UPDATE_WSS, &itv->i_flags);
			}
			break;

		default:
			break;
		}
		count -= sizeof(*p);
		p++;
	}

	if (found_cc && cc_pos < sizeof(itv->vbi.cc_data_even)) {
		itv->vbi.cc_data_odd[cc_pos] = cc[0];
		itv->vbi.cc_data_odd[cc_pos + 1] = cc[1];
		itv->vbi.cc_data_even[cc_pos] = cc[2];
		itv->vbi.cc_data_even[cc_pos + 1] = cc[3];
		itv->vbi.cc_pos = cc_pos + 2;
		set_bit(IVTV_F_I_UPDATE_CC, &itv->i_flags);
	}

	return (const char __user *)p - ubuf;
}

/* Compress raw VBI format, removes leading SAV codes and surplus space after the
   field.
   Returns new compressed size. */
static u32 compress_raw_buf(struct ivtv *itv, u8 *buf, u32 size)
{
	u32 line_size = itv->vbi.raw_decoder_line_size;
	u32 lines = itv->vbi.count;
	u8 sav1 = itv->vbi.raw_decoder_sav_odd_field;
	u8 sav2 = itv->vbi.raw_decoder_sav_even_field;
	u8 *q = buf;
	u8 *p;
	int i;

	for (i = 0; i < lines; i++) {
		p = buf + i * line_size;

		/* Look for SAV code */
		if (p[0] != 0xff || p[1] || p[2] || (p[3] != sav1 && p[3] != sav2)) {
			break;
		}
		memcpy(q, p + 4, line_size - 4);
		q += line_size - 4;
	}
	return lines * (line_size - 4);
}


/* Compressed VBI format, all found sliced blocks put next to one another
   Returns new compressed size */
static u32 compress_sliced_buf(struct ivtv *itv, u32 line, u8 *buf, u32 size, u8 sav)
{
	u32 line_size = itv->vbi.sliced_decoder_line_size;
	struct v4l2_decode_vbi_line vbi;
	int i;

	/* find the first valid line */
	for (i = 0; i < size; i++, buf++) {
		if (buf[0] == 0xff && !buf[1] && !buf[2] && buf[3] == sav)
			break;
	}

	size -= i;
	if (size < line_size) {
		return line;
	}
	for (i = 0; i < size / line_size; i++) {
		u8 *p = buf + i * line_size;

		/* Look for SAV code  */
		if (p[0] != 0xff || p[1] || p[2] || p[3] != sav) {
			continue;
		}
		vbi.p = p + 4;
		itv->video_dec_func(itv, VIDIOC_INT_DECODE_VBI_LINE, &vbi);
		if (vbi.type) {
			itv->vbi.sliced_data[line].id = vbi.type;
			itv->vbi.sliced_data[line].field = vbi.is_second_field;
			itv->vbi.sliced_data[line].line = vbi.line;
			memcpy(itv->vbi.sliced_data[line].data, vbi.p, 42);
			line++;
		}
	}
	return line;
}

void ivtv_process_vbi_data(struct ivtv *itv, struct ivtv_buffer *buf,
			   u64 pts_stamp, int streamtype)
{
	u8 *p = (u8 *) buf->buf;
	u32 size = buf->bytesused;
	int y;

	/* Raw VBI data */
	if (streamtype == IVTV_ENC_STREAM_TYPE_VBI && itv->vbi.sliced_in->service_set == 0) {
		u8 type;

		ivtv_buf_swap(buf);

		type = p[3];

		size = buf->bytesused = compress_raw_buf(itv, p, size);

		/* second field of the frame? */
		if (type == itv->vbi.raw_decoder_sav_even_field) {
			/* Dirty hack needed for backwards
			   compatibility of old VBI software. */
			p += size - 4;
			memcpy(p, &itv->vbi.frame, 4);
			itv->vbi.frame++;
		}
		return;
	}

	/* Sliced VBI data with data insertion */
	if (streamtype == IVTV_ENC_STREAM_TYPE_VBI) {
		int lines;

		ivtv_buf_swap(buf);

		/* first field */
		lines = compress_sliced_buf(itv, 0, p, size / 2,
			itv->vbi.sliced_decoder_sav_odd_field);
		/* second field */
		/* experimentation shows that the second half does not always begin
		   at the exact address. So start a bit earlier (hence 32). */
		lines = compress_sliced_buf(itv, lines, p + size / 2 - 32, size / 2 + 32,
			itv->vbi.sliced_decoder_sav_even_field);
		/* always return at least one empty line */
		if (lines == 0) {
			itv->vbi.sliced_data[0].id = 0;
			itv->vbi.sliced_data[0].line = 0;
			itv->vbi.sliced_data[0].field = 0;
			lines = 1;
		}
		buf->bytesused = size = lines * sizeof(itv->vbi.sliced_data[0]);
		memcpy(p, &itv->vbi.sliced_data[0], size);

		if (itv->vbi.insert_mpeg) {
			copy_vbi_data(itv, lines, pts_stamp);
		}
		itv->vbi.frame++;
		return;
	}

	/* Sliced VBI re-inserted from an MPEG stream */
	if (streamtype == IVTV_DEC_STREAM_TYPE_VBI) {
		/* If the size is not 4-byte aligned, then the starting address
		   for the swapping is also shifted. After swapping the data the
		   real start address of the VBI data is exactly 4 bytes after the
		   original start. It's a bit fiddly but it works like a charm.
		   Non-4-byte alignment happens when an lseek is done on the input
		   mpeg file to a non-4-byte aligned position. So on arrival here
		   the VBI data is also non-4-byte aligned. */
		int offset = size & 3;
		int cnt;

		if (offset) {
			p += 4 - offset;
		}
		/* Swap Buffer */
		for (y = 0; y < size; y += 4) {
		       swab32s((u32 *)(p + y));
		}

		cnt = ivtv_convert_ivtv_vbi(itv, p + offset);
		memcpy(buf->buf, itv->vbi.sliced_dec_data, cnt);
		buf->bytesused = cnt;

		passthrough_vbi_data(itv, cnt / sizeof(itv->vbi.sliced_dec_data[0]));
		return;
	}
}

void ivtv_disable_vbi(struct ivtv *itv)
{
	clear_bit(IVTV_F_I_UPDATE_WSS, &itv->i_flags);
	clear_bit(IVTV_F_I_UPDATE_VPS, &itv->i_flags);
	clear_bit(IVTV_F_I_UPDATE_CC, &itv->i_flags);
	ivtv_set_wss(itv, 0, 0);
	ivtv_set_cc(itv, 0, 0, 0, 0, 0);
	ivtv_set_vps(itv, 0, 0, 0, 0, 0, 0);
	itv->vbi.vps_found = itv->vbi.wss_found = 0;
	itv->vbi.wss = 0;
	itv->vbi.cc_pos = 0;
}


void ivtv_vbi_work_handler(struct ivtv *itv)
{
	struct v4l2_sliced_vbi_data data;

	/* Lock */
	if (itv->output_mode == OUT_PASSTHROUGH) {
		/* Note: currently only the saa7115 is used in a PVR350,
		   so these commands are for now saa7115 specific. */
		if (itv->is_50hz) {
			data.id = V4L2_SLICED_WSS_625;
			data.field = 0;

			if (itv->video_dec_func(itv, VIDIOC_INT_G_VBI_DATA, &data) == 0) {
				ivtv_set_wss(itv, 1, data.data[0] & 0xf);
				itv->vbi.wss_no_update = 0;
			} else if (itv->vbi.wss_no_update == 4) {
				ivtv_set_wss(itv, 1, 0x8);  /* 4x3 full format */
			} else {
				itv->vbi.wss_no_update++;
			}
		}
		else {
			u8 c1 = 0, c2 = 0, c3 = 0, c4 = 0;
			int mode = 0;

			data.id = V4L2_SLICED_CAPTION_525;
			data.field = 0;
			if (itv->video_dec_func(itv, VIDIOC_INT_G_VBI_DATA, &data) == 0) {
				mode |= 1;
				c1 = data.data[0];
				c2 = data.data[1];
			}
			data.field = 1;
			if (itv->video_dec_func(itv, VIDIOC_INT_G_VBI_DATA, &data) == 0) {
				mode |= 2;
				c3 = data.data[0];
				c4 = data.data[1];
			}
			if (mode) {
				itv->vbi.cc_no_update = 0;
				ivtv_set_cc(itv, mode, c1, c2, c3, c4);
			} else if (itv->vbi.cc_no_update == 4) {
				ivtv_set_cc(itv, 0, 0, 0, 0, 0);
			} else {
				itv->vbi.cc_no_update++;
			}
		}
		return;
	}

	if (test_and_clear_bit(IVTV_F_I_UPDATE_WSS, &itv->i_flags)) {
		/* Lock */
		ivtv_set_wss(itv, itv->vbi.wss_found, itv->vbi.wss & 0xf);
	}

	if (test_and_clear_bit(IVTV_F_I_UPDATE_CC, &itv->i_flags)) {
		if (itv->vbi.cc_pos == 0) {
			ivtv_set_cc(itv, 3, 0x80, 0x80, 0x80, 0x80);
		}
		while (itv->vbi.cc_pos) {
			u8 cc_odd0 = itv->vbi.cc_data_odd[0];
			u8 cc_odd1 = itv->vbi.cc_data_odd[1];
			u8 cc_even0 = itv->vbi.cc_data_even[0];
			u8 cc_even1 = itv->vbi.cc_data_even[1];

			memcpy(itv->vbi.cc_data_odd, itv->vbi.cc_data_odd + 2, sizeof(itv->vbi.cc_data_odd) - 2);
			memcpy(itv->vbi.cc_data_even, itv->vbi.cc_data_even + 2, sizeof(itv->vbi.cc_data_even) - 2);
			itv->vbi.cc_pos -= 2;
			if (itv->vbi.cc_pos && cc_odd0 == 0x80 && cc_odd1 == 0x80)
				continue;

			/* Send to Saa7127 */
			ivtv_set_cc(itv, 3, cc_odd0, cc_odd1, cc_even0, cc_even1);
			if (itv->vbi.cc_pos == 0)
				set_bit(IVTV_F_I_UPDATE_CC, &itv->i_flags);
			break;
		}
	}

	if (test_and_clear_bit(IVTV_F_I_UPDATE_VPS, &itv->i_flags)) {
		/* Lock */
		ivtv_set_vps(itv, itv->vbi.vps_found,
			itv->vbi.vps[0], itv->vbi.vps[1],
			itv->vbi.vps[2], itv->vbi.vps[3], itv->vbi.vps[4]);
	}
}
