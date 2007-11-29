/******************************************************************************
 *
 * Copyright(c) 2003 - 2007 Intel Corporation. All rights reserved.
 *
 * This program is free software; you can redistribute it and/or modify it
 * under the terms of version 2 of the GNU General Public License as
 * published by the Free Software Foundation.
 *
 * This program is distributed in the hope that it will be useful, but WITHOUT
 * ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or
 * FITNESS FOR A PARTICULAR PURPOSE.  See the GNU General Public License for
 * more details.
 *
 * You should have received a copy of the GNU General Public License along with
 * this program; if not, write to the Free Software Foundation, Inc.,
 * 51 Franklin Street, Fifth Floor, Boston, MA 02110, USA
 *
 * The full GNU General Public License is included in this distribution in the
 * file called LICENSE.
 *
 * Contact Information:
 * James P. Ketrenos <ipw2100-admin@linux.intel.com>
 * Intel Corporation, 5200 N.E. Elam Young Parkway, Hillsboro, OR 97124-6497
 *
 *****************************************************************************/
#ifndef __iwl_4965_h__
#define __iwl_4965_h__

#include <linux/pci.h> /* for struct pci_device_id */
#include <linux/kernel.h>
#include <net/ieee80211_radiotap.h>

struct iwl4965_priv;

/* Hardware specific file defines the PCI IDs table for that hardware module */
extern struct pci_device_id iwl4965_hw_card_ids[];

#define DRV_NAME        "iwl4965"
#include "iwl-4965-hw.h"
#include "iwl-prph.h"
#include "iwl-4965-debug.h"

/* Default noise level to report when noise measurement is not available.
 *   This may be because we're:
 *   1)  Not associated (4965, no beacon statistics being sent to driver)
 *   2)  Scanning (noise measurement does not apply to associated channel)
 *   3)  Receiving CCK (3945 delivers noise info only for OFDM frames)
 * Use default noise value of -127 ... this is below the range of measurable
 *   Rx dBm for either 3945 or 4965, so it can indicate "unmeasurable" to user.
 *   Also, -127 works better than 0 when averaging frames with/without
 *   noise info (e.g. averaging might be done in app); measured dBm values are
 *   always negative ... using a negative value as the default keeps all
 *   averages within an s8's (used in some apps) range of negative values. */
#define IWL_NOISE_MEAS_NOT_AVAILABLE (-127)

/* Module parameters accessible from iwl-*.c */
extern int iwl4965_param_hwcrypto;
extern int iwl4965_param_queues_num;

enum iwl4965_antenna {
	IWL_ANTENNA_DIVERSITY,
	IWL_ANTENNA_MAIN,
	IWL_ANTENNA_AUX
};

/*
 * RTS threshold here is total size [2347] minus 4 FCS bytes
 * Per spec:
 *   a value of 0 means RTS on all data/management packets
 *   a value > max MSDU size means no RTS
 * else RTS for data/management frames where MPDU is larger
 *   than RTS value.
 */
#define DEFAULT_RTS_THRESHOLD     2347U
#define MIN_RTS_THRESHOLD         0U
#define MAX_RTS_THRESHOLD         2347U
#define MAX_MSDU_SIZE		  2304U
#define MAX_MPDU_SIZE		  2346U
#define DEFAULT_BEACON_INTERVAL   100U
#define	DEFAULT_SHORT_RETRY_LIMIT 7U
#define	DEFAULT_LONG_RETRY_LIMIT  4U

struct iwl4965_rx_mem_buffer {
	dma_addr_t dma_addr;
	struct sk_buff *skb;
	struct list_head list;
};

struct iwl4965_rt_rx_hdr {
	struct ieee80211_radiotap_header rt_hdr;
	__le64 rt_tsf;		/* TSF */
	u8 rt_flags;		/* radiotap packet flags */
	u8 rt_rate;		/* rate in 500kb/s */
	__le16 rt_channelMHz;	/* channel in MHz */
	__le16 rt_chbitmask;	/* channel bitfield */
	s8 rt_dbmsignal;	/* signal in dBm, kluged to signed */
	s8 rt_dbmnoise;
	u8 rt_antenna;		/* antenna number */
	u8 payload[0];		/* payload... */
} __attribute__ ((packed));

struct iwl4965_rt_tx_hdr {
	struct ieee80211_radiotap_header rt_hdr;
	u8 rt_rate;		/* rate in 500kb/s */
	__le16 rt_channel;	/* channel in mHz */
	__le16 rt_chbitmask;	/* channel bitfield */
	s8 rt_dbmsignal;	/* signal in dBm, kluged to signed */
	u8 rt_antenna;		/* antenna number */
	u8 payload[0];		/* payload... */
} __attribute__ ((packed));

/*
 * Generic queue structure
 *
 * Contains common data for Rx and Tx queues
 */
struct iwl4965_queue {
	int n_bd;              /* number of BDs in this queue */
	int write_ptr;       /* 1-st empty entry (index) host_w*/
	int read_ptr;         /* last used entry (index) host_r*/
	dma_addr_t dma_addr;   /* physical addr for BD's */
	int n_window;	       /* safe queue window */
	u32 id;
	int low_mark;	       /* low watermark, resume queue if free
				* space more than this */
	int high_mark;         /* high watermark, stop queue if free
				* space less than this */
} __attribute__ ((packed));

#define MAX_NUM_OF_TBS          (20)

struct iwl4965_tx_info {
	struct ieee80211_tx_status status;
	struct sk_buff *skb[MAX_NUM_OF_TBS];
};

/**
 * struct iwl4965_tx_queue - Tx Queue for DMA
 * @need_update: need to update read/write index
 * @shed_retry: queue is HT AGG enabled
 *
 * Queue consists of circular buffer of BD's and required locking structures.
 */
struct iwl4965_tx_queue {
	struct iwl4965_queue q;
	struct iwl4965_tfd_frame *bd;
	struct iwl4965_cmd *cmd;
	dma_addr_t dma_addr_cmd;
	struct iwl4965_tx_info *txb;
	int need_update;
	int sched_retry;
	int active;
};

#define IWL_NUM_SCAN_RATES         (2)

struct iwl4965_channel_tgd_info {
	u8 type;
	s8 max_power;
};

struct iwl4965_channel_tgh_info {
	s64 last_radar_time;
};

/* current Tx power values to use, one for each rate for each channel.
 * requested power is limited by:
 * -- regulatory EEPROM limits for this channel
 * -- hardware capabilities (clip-powers)
 * -- spectrum management
 * -- user preference (e.g. iwconfig)
 * when requested power is set, base power index must also be set. */
struct iwl4965_channel_power_info {
	struct iwl4965_tx_power tpc;	/* actual radio and DSP gain settings */
	s8 power_table_index;	/* actual (compenst'd) index into gain table */
	s8 base_power_index;	/* gain index for power at factory temp. */
	s8 requested_power;	/* power (dBm) requested for this chnl/rate */
};

/* current scan Tx power values to use, one for each scan rate for each
 * channel. */
struct iwl4965_scan_power_info {
	struct iwl4965_tx_power tpc;	/* actual radio and DSP gain settings */
	s8 power_table_index;	/* actual (compenst'd) index into gain table */
	s8 requested_power;	/* scan pwr (dBm) requested for chnl/rate */
};

/* Channel unlock period is 15 seconds. If no beacon or probe response
 * has been received within 15 seconds on a locked channel then the channel
 * remains locked. */
#define TX_UNLOCK_PERIOD 15

/* CSA lock period is 15 seconds.  If a CSA has been received on a channel in
 * the last 15 seconds, the channel is locked */
#define CSA_LOCK_PERIOD 15
/*
 * One for each channel, holds all channel setup data
 * Some of the fields (e.g. eeprom and flags/max_power_avg) are redundant
 *     with one another!
 */
#define IWL4965_MAX_RATE (33)

struct iwl4965_channel_info {
	struct iwl4965_channel_tgd_info tgd;
	struct iwl4965_channel_tgh_info tgh;
	struct iwl4965_eeprom_channel eeprom;	/* EEPROM regulatory limit */
	struct iwl4965_eeprom_channel fat_eeprom;	/* EEPROM regulatory limit for
						 * FAT channel */

	u8 channel;	  /* channel number */
	u8 flags;	  /* flags copied from EEPROM */
	s8 max_power_avg; /* (dBm) regul. eeprom, normal Tx, any rate */
	s8 curr_txpow;	  /* (dBm) regulatory/spectrum/user (not h/w) */
	s8 min_power;	  /* always 0 */
	s8 scan_power;	  /* (dBm) regul. eeprom, direct scans, any rate */

	u8 group_index;	  /* 0-4, maps channel to group1/2/3/4/5 */
	u8 band_index;	  /* 0-4, maps channel to band1/2/3/4/5 */
	u8 phymode;	  /* MODE_IEEE80211{A,B,G} */

	/* Radio/DSP gain settings for each "normal" data Tx rate.
	 * These include, in addition to RF and DSP gain, a few fields for
	 *   remembering/modifying gain settings (indexes). */
	struct iwl4965_channel_power_info power_info[IWL4965_MAX_RATE];

	/* FAT channel info */
	s8 fat_max_power_avg;	/* (dBm) regul. eeprom, normal Tx, any rate */
	s8 fat_curr_txpow;	/* (dBm) regulatory/spectrum/user (not h/w) */
	s8 fat_min_power;	/* always 0 */
	s8 fat_scan_power;	/* (dBm) eeprom, direct scans, any rate */
	u8 fat_flags;		/* flags copied from EEPROM */
	u8 fat_extension_channel;

	/* Radio/DSP gain settings for each scan rate, for directed scans. */
	struct iwl4965_scan_power_info scan_pwr_info[IWL_NUM_SCAN_RATES];
};

struct iwl4965_clip_group {
	/* maximum power level to prevent clipping for each rate, derived by
	 *   us from this band's saturation power in EEPROM */
	const s8 clip_powers[IWL_MAX_RATES];
};

#include "iwl-4965-rs.h"

#define IWL_TX_FIFO_AC0	0
#define IWL_TX_FIFO_AC1	1
#define IWL_TX_FIFO_AC2	2
#define IWL_TX_FIFO_AC3	3
#define IWL_TX_FIFO_HCCA_1	5
#define IWL_TX_FIFO_HCCA_2	6
#define IWL_TX_FIFO_NONE	7

/* Minimum number of queues. MAX_NUM is defined in hw specific files */
#define IWL_MIN_NUM_QUEUES	4

/* Power management (not Tx power) structures */

struct iwl4965_power_vec_entry {
	struct iwl4965_powertable_cmd cmd;
	u8 no_dtim;
};
#define IWL_POWER_RANGE_0  (0)
#define IWL_POWER_RANGE_1  (1)

#define IWL_POWER_MODE_CAM	0x00	/* Continuously Aware Mode, always on */
#define IWL_POWER_INDEX_3	0x03
#define IWL_POWER_INDEX_5	0x05
#define IWL_POWER_AC		0x06
#define IWL_POWER_BATTERY	0x07
#define IWL_POWER_LIMIT		0x07
#define IWL_POWER_MASK		0x0F
#define IWL_POWER_ENABLED	0x10
#define IWL_POWER_LEVEL(x)	((x) & IWL_POWER_MASK)

struct iwl4965_power_mgr {
	spinlock_t lock;
	struct iwl4965_power_vec_entry pwr_range_0[IWL_POWER_AC];
	struct iwl4965_power_vec_entry pwr_range_1[IWL_POWER_AC];
	u8 active_index;
	u32 dtim_val;
};

#define IEEE80211_DATA_LEN              2304
#define IEEE80211_4ADDR_LEN             30
#define IEEE80211_HLEN                  (IEEE80211_4ADDR_LEN)
#define IEEE80211_FRAME_LEN             (IEEE80211_DATA_LEN + IEEE80211_HLEN)

struct iwl4965_frame {
	union {
		struct ieee80211_hdr frame;
		struct iwl4965_tx_beacon_cmd beacon;
		u8 raw[IEEE80211_FRAME_LEN];
		u8 cmd[360];
	} u;
	struct list_head list;
};

#define SEQ_TO_QUEUE(x)  ((x >> 8) & 0xbf)
#define QUEUE_TO_SEQ(x)  ((x & 0xbf) << 8)
#define SEQ_TO_INDEX(x) (x & 0xff)
#define INDEX_TO_SEQ(x) (x & 0xff)
#define SEQ_HUGE_FRAME  (0x4000)
#define SEQ_RX_FRAME    __constant_cpu_to_le16(0x8000)
#define SEQ_TO_SN(seq) (((seq) & IEEE80211_SCTL_SEQ) >> 4)
#define SN_TO_SEQ(ssn) (((ssn) << 4) & IEEE80211_SCTL_SEQ)
#define MAX_SN ((IEEE80211_SCTL_SEQ) >> 4)

enum {
	/* CMD_SIZE_NORMAL = 0, */
	CMD_SIZE_HUGE = (1 << 0),
	/* CMD_SYNC = 0, */
	CMD_ASYNC = (1 << 1),
	/* CMD_NO_SKB = 0, */
	CMD_WANT_SKB = (1 << 2),
};

struct iwl4965_cmd;
struct iwl4965_priv;

struct iwl4965_cmd_meta {
	struct iwl4965_cmd_meta *source;
	union {
		struct sk_buff *skb;
		int (*callback)(struct iwl4965_priv *priv,
				struct iwl4965_cmd *cmd, struct sk_buff *skb);
	} __attribute__ ((packed)) u;

	/* The CMD_SIZE_HUGE flag bit indicates that the command
	 * structure is stored at the end of the shared queue memory. */
	u32 flags;

} __attribute__ ((packed));

struct iwl4965_cmd {
	struct iwl4965_cmd_meta meta;
	struct iwl4965_cmd_header hdr;
	union {
		struct iwl4965_addsta_cmd addsta;
		struct iwl4965_led_cmd led;
		u32 flags;
		u8 val8;
		u16 val16;
		u32 val32;
		struct iwl4965_bt_cmd bt;
		struct iwl4965_rxon_time_cmd rxon_time;
		struct iwl4965_powertable_cmd powertable;
		struct iwl4965_qosparam_cmd qosparam;
		struct iwl4965_tx_cmd tx;
		struct iwl4965_tx_beacon_cmd tx_beacon;
		struct iwl4965_rxon_assoc_cmd rxon_assoc;
		u8 *indirect;
		u8 payload[360];
	} __attribute__ ((packed)) cmd;
} __attribute__ ((packed));

struct iwl4965_host_cmd {
	u8 id;
	u16 len;
	struct iwl4965_cmd_meta meta;
	const void *data;
};

#define TFD_MAX_PAYLOAD_SIZE (sizeof(struct iwl4965_cmd) - \
			      sizeof(struct iwl4965_cmd_meta))

/*
 * RX related structures and functions
 */
#define RX_FREE_BUFFERS 64
#define RX_LOW_WATERMARK 8

#define SUP_RATE_11A_MAX_NUM_CHANNELS  8
#define SUP_RATE_11B_MAX_NUM_CHANNELS  4
#define SUP_RATE_11G_MAX_NUM_CHANNELS  12

/**
 * struct iwl4965_rx_queue - Rx queue
 * @processed: Internal index to last handled Rx packet
 * @read: Shared index to newest available Rx buffer
 * @write: Shared index to oldest written Rx packet
 * @free_count: Number of pre-allocated buffers in rx_free
 * @rx_free: list of free SKBs for use
 * @rx_used: List of Rx buffers with no SKB
 * @need_update: flag to indicate we need to update read/write index
 *
 * NOTE:  rx_free and rx_used are used as a FIFO for iwl4965_rx_mem_buffers
 */
struct iwl4965_rx_queue {
	__le32 *bd;
	dma_addr_t dma_addr;
	struct iwl4965_rx_mem_buffer pool[RX_QUEUE_SIZE + RX_FREE_BUFFERS];
	struct iwl4965_rx_mem_buffer *queue[RX_QUEUE_SIZE];
	u32 processed;
	u32 read;
	u32 write;
	u32 free_count;
	struct list_head rx_free;
	struct list_head rx_used;
	int need_update;
	spinlock_t lock;
};

#define IWL_SUPPORTED_RATES_IE_LEN         8

#define SCAN_INTERVAL 100

#define MAX_A_CHANNELS  252
#define MIN_A_CHANNELS  7

#define MAX_B_CHANNELS  14
#define MIN_B_CHANNELS  1

#define STATUS_HCMD_ACTIVE	0	/* host command in progress */
#define STATUS_INT_ENABLED	1
#define STATUS_RF_KILL_HW	2
#define STATUS_RF_KILL_SW	3
#define STATUS_INIT		4
#define STATUS_ALIVE		5
#define STATUS_READY		6
#define STATUS_TEMPERATURE	7
#define STATUS_GEO_CONFIGURED	8
#define STATUS_EXIT_PENDING	9
#define STATUS_IN_SUSPEND	10
#define STATUS_STATISTICS	11
#define STATUS_SCANNING		12
#define STATUS_SCAN_ABORTING	13
#define STATUS_SCAN_HW		14
#define STATUS_POWER_PMI	15
#define STATUS_FW_ERROR		16

#define MAX_TID_COUNT        9

#define IWL_INVALID_RATE     0xFF
#define IWL_INVALID_VALUE    -1

#ifdef CONFIG_IWL4965_HT
#ifdef CONFIG_IWL4965_HT_AGG
struct iwl4965_ht_agg {
	u16 txq_id;
	u16 frame_count;
	u16 wait_for_ba;
	u16 start_idx;
	u32 bitmap0;
	u32 bitmap1;
	u32 rate_n_flags;
};
#endif /* CONFIG_IWL4965_HT_AGG */
#endif /* CONFIG_IWL4965_HT */

struct iwl4965_tid_data {
	u16 seq_number;
#ifdef CONFIG_IWL4965_HT
#ifdef CONFIG_IWL4965_HT_AGG
	struct iwl4965_ht_agg agg;
#endif	/* CONFIG_IWL4965_HT_AGG */
#endif /* CONFIG_IWL4965_HT */
};

struct iwl4965_hw_key {
	enum ieee80211_key_alg alg;
	int keylen;
	u8 key[32];
};

union iwl4965_ht_rate_supp {
	u16 rates;
	struct {
		u8 siso_rate;
		u8 mimo_rate;
	};
};

#ifdef CONFIG_IWL4965_HT
#define CFG_HT_RX_AMPDU_FACTOR_DEF  (0x3)
#define HT_IE_MAX_AMSDU_SIZE_4K     (0)
#define CFG_HT_MPDU_DENSITY_2USEC   (0x5)
#define CFG_HT_MPDU_DENSITY_DEF CFG_HT_MPDU_DENSITY_2USEC

struct sta_ht_info {
	u8 is_ht;
	u16 rx_mimo_ps_mode;
	u16 tx_mimo_ps_mode;
	u16 control_channel;
	u8 max_amsdu_size;
	u8 ampdu_factor;
	u8 mpdu_density;
	u8 operating_mode;
	u8 supported_chan_width;
	u8 extension_chan_offset;
	u8 is_green_field;
	u8 sgf;			/* HT_SHORT_GI_* short guard interval */
	u8 supp_rates[16];
	u8 tx_chan_width;
	u8 chan_width_cap;
};
#endif				/*CONFIG_IWL4965_HT */

#ifdef CONFIG_IWL4965_QOS

union iwl4965_qos_capabity {
	struct {
		u8 edca_count:4;	/* bit 0-3 */
		u8 q_ack:1;		/* bit 4 */
		u8 queue_request:1;	/* bit 5 */
		u8 txop_request:1;	/* bit 6 */
		u8 reserved:1;		/* bit 7 */
	} q_AP;
	struct {
		u8 acvo_APSD:1;		/* bit 0 */
		u8 acvi_APSD:1;		/* bit 1 */
		u8 ac_bk_APSD:1;	/* bit 2 */
		u8 ac_be_APSD:1;	/* bit 3 */
		u8 q_ack:1;		/* bit 4 */
		u8 max_len:2;		/* bit 5-6 */
		u8 more_data_ack:1;	/* bit 7 */
	} q_STA;
	u8 val;
};

/* QoS structures */
struct iwl4965_qos_info {
	int qos_enable;
	int qos_active;
	union iwl4965_qos_capabity qos_cap;
	struct iwl4965_qosparam_cmd def_qos_parm;
};
#endif /*CONFIG_IWL4965_QOS */

#define STA_PS_STATUS_WAKE             0
#define STA_PS_STATUS_SLEEP            1

struct iwl4965_station_entry {
	struct iwl4965_addsta_cmd sta;
	struct iwl4965_tid_data tid[MAX_TID_COUNT];
	u8 used;
	u8 ps_status;
	struct iwl4965_hw_key keyinfo;
};

/* one for each uCode image (inst/data, boot/init/runtime) */
struct fw_desc {
	void *v_addr;		/* access by driver */
	dma_addr_t p_addr;	/* access by card's busmaster DMA */
	u32 len;		/* bytes */
};

/* uCode file layout */
struct iwl4965_ucode {
	__le32 ver;		/* major/minor/subminor */
	__le32 inst_size;	/* bytes of runtime instructions */
	__le32 data_size;	/* bytes of runtime data */
	__le32 init_size;	/* bytes of initialization instructions */
	__le32 init_data_size;	/* bytes of initialization data */
	__le32 boot_size;	/* bytes of bootstrap instructions */
	u8 data[0];		/* data in same order as "size" elements */
};

#define IWL_IBSS_MAC_HASH_SIZE 32

struct iwl4965_ibss_seq {
	u8 mac[ETH_ALEN];
	u16 seq_num;
	u16 frag_num;
	unsigned long packet_time;
	struct list_head list;
};

struct iwl4965_driver_hw_info {
	u16 max_txq_num;
	u16 ac_queue_count;
	u16 tx_cmd_len;
	u16 max_rxq_size;
	u32 rx_buffer_size;
	u16 max_rxq_log;
	u8  max_stations;
	u8  bcast_sta_id;
	void *shared_virt;
	dma_addr_t shared_phys;
};


#define STA_FLG_RTS_MIMO_PROT_MSK	__constant_cpu_to_le32(1 << 17)
#define STA_FLG_AGG_MPDU_8US_MSK	__constant_cpu_to_le32(1 << 18)
#define STA_FLG_MAX_AGG_SIZE_POS	(19)
#define STA_FLG_MAX_AGG_SIZE_MSK	__constant_cpu_to_le32(3 << 19)
#define STA_FLG_FAT_EN_MSK		__constant_cpu_to_le32(1 << 21)
#define STA_FLG_MIMO_DIS_MSK		__constant_cpu_to_le32(1 << 22)
#define STA_FLG_AGG_MPDU_DENSITY_POS	(23)
#define STA_FLG_AGG_MPDU_DENSITY_MSK	__constant_cpu_to_le32(7 << 23)
#define HT_SHORT_GI_20MHZ_ONLY          (1 << 0)
#define HT_SHORT_GI_40MHZ_ONLY          (1 << 1)


#define IWL_RX_HDR(x) ((struct iwl4965_rx_frame_hdr *)(\
		       x->u.rx_frame.stats.payload + \
		       x->u.rx_frame.stats.phy_count))
#define IWL_RX_END(x) ((struct iwl4965_rx_frame_end *)(\
		       IWL_RX_HDR(x)->payload + \
		       le16_to_cpu(IWL_RX_HDR(x)->len)))
#define IWL_RX_STATS(x) (&x->u.rx_frame.stats)
#define IWL_RX_DATA(x) (IWL_RX_HDR(x)->payload)


/******************************************************************************
 *
 * Functions implemented in iwl-base.c which are forward declared here
 * for use by iwl-*.c
 *
 *****************************************************************************/
struct iwl4965_addsta_cmd;
extern int iwl4965_send_add_station(struct iwl4965_priv *priv,
				struct iwl4965_addsta_cmd *sta, u8 flags);
extern u8 iwl4965_add_station_flags(struct iwl4965_priv *priv, const u8 *bssid,
			  int is_ap, u8 flags);
extern int iwl4965_is_network_packet(struct iwl4965_priv *priv,
				 struct ieee80211_hdr *header);
extern int iwl4965_power_init_handle(struct iwl4965_priv *priv);
extern int iwl4965_eeprom_init(struct iwl4965_priv *priv);
#ifdef CONFIG_IWL4965_DEBUG
extern void iwl4965_report_frame(struct iwl4965_priv *priv,
			     struct iwl4965_rx_packet *pkt,
			     struct ieee80211_hdr *header, int group100);
#else
static inline void iwl4965_report_frame(struct iwl4965_priv *priv,
				    struct iwl4965_rx_packet *pkt,
				    struct ieee80211_hdr *header,
				    int group100) {}
#endif
extern void iwl4965_handle_data_packet_monitor(struct iwl4965_priv *priv,
					   struct iwl4965_rx_mem_buffer *rxb,
					   void *data, short len,
					   struct ieee80211_rx_status *stats,
					   u16 phy_flags);
extern int iwl4965_is_duplicate_packet(struct iwl4965_priv *priv,
				       struct ieee80211_hdr *header);
extern int iwl4965_rx_queue_alloc(struct iwl4965_priv *priv);
extern void iwl4965_rx_queue_reset(struct iwl4965_priv *priv,
			       struct iwl4965_rx_queue *rxq);
extern int iwl4965_calc_db_from_ratio(int sig_ratio);
extern int iwl4965_calc_sig_qual(int rssi_dbm, int noise_dbm);
extern int iwl4965_tx_queue_init(struct iwl4965_priv *priv,
			     struct iwl4965_tx_queue *txq, int count, u32 id);
extern void iwl4965_rx_replenish(void *data);
extern void iwl4965_tx_queue_free(struct iwl4965_priv *priv, struct iwl4965_tx_queue *txq);
extern int iwl4965_send_cmd_pdu(struct iwl4965_priv *priv, u8 id, u16 len,
			    const void *data);
extern int __must_check iwl4965_send_cmd(struct iwl4965_priv *priv,
		struct iwl4965_host_cmd *cmd);
extern unsigned int iwl4965_fill_beacon_frame(struct iwl4965_priv *priv,
					struct ieee80211_hdr *hdr,
					const u8 *dest, int left);
extern int iwl4965_rx_queue_update_write_ptr(struct iwl4965_priv *priv,
					 struct iwl4965_rx_queue *q);
extern int iwl4965_send_statistics_request(struct iwl4965_priv *priv);
extern void iwl4965_set_decrypted_flag(struct iwl4965_priv *priv, struct sk_buff *skb,
				   u32 decrypt_res,
				   struct ieee80211_rx_status *stats);
extern __le16 *ieee80211_get_qos_ctrl(struct ieee80211_hdr *hdr);

extern const u8 iwl4965_broadcast_addr[ETH_ALEN];

/*
 * Currently used by iwl-3945-rs... look at restructuring so that it doesn't
 * call this... todo... fix that.
*/
extern u8 iwl4965_sync_station(struct iwl4965_priv *priv, int sta_id,
			   u16 tx_rate, u8 flags);

/******************************************************************************
 *
 * Functions implemented in iwl-[34]*.c which are forward declared here
 * for use by iwl-base.c
 *
 * NOTE:  The implementation of these functions are hardware specific
 * which is why they are in the hardware specific files (vs. iwl-base.c)
 *
 * Naming convention --
 * iwl4965_         <-- Its part of iwlwifi (should be changed to iwl4965_)
 * iwl4965_hw_      <-- Hardware specific (implemented in iwl-XXXX.c by all HW)
 * iwlXXXX_     <-- Hardware specific (implemented in iwl-XXXX.c for XXXX)
 * iwl4965_bg_      <-- Called from work queue context
 * iwl4965_mac_     <-- mac80211 callback
 *
 ****************************************************************************/
extern void iwl4965_hw_rx_handler_setup(struct iwl4965_priv *priv);
extern void iwl4965_hw_setup_deferred_work(struct iwl4965_priv *priv);
extern void iwl4965_hw_cancel_deferred_work(struct iwl4965_priv *priv);
extern int iwl4965_hw_rxq_stop(struct iwl4965_priv *priv);
extern int iwl4965_hw_set_hw_setting(struct iwl4965_priv *priv);
extern int iwl4965_hw_nic_init(struct iwl4965_priv *priv);
extern int iwl4965_hw_nic_stop_master(struct iwl4965_priv *priv);
extern void iwl4965_hw_txq_ctx_free(struct iwl4965_priv *priv);
extern void iwl4965_hw_txq_ctx_stop(struct iwl4965_priv *priv);
extern int iwl4965_hw_nic_reset(struct iwl4965_priv *priv);
extern int iwl4965_hw_txq_attach_buf_to_tfd(struct iwl4965_priv *priv, void *tfd,
					dma_addr_t addr, u16 len);
extern int iwl4965_hw_txq_free_tfd(struct iwl4965_priv *priv, struct iwl4965_tx_queue *txq);
extern int iwl4965_hw_get_temperature(struct iwl4965_priv *priv);
extern int iwl4965_hw_tx_queue_init(struct iwl4965_priv *priv,
				struct iwl4965_tx_queue *txq);
extern unsigned int iwl4965_hw_get_beacon_cmd(struct iwl4965_priv *priv,
				 struct iwl4965_frame *frame, u8 rate);
extern int iwl4965_hw_get_rx_read(struct iwl4965_priv *priv);
extern void iwl4965_hw_build_tx_cmd_rate(struct iwl4965_priv *priv,
				     struct iwl4965_cmd *cmd,
				     struct ieee80211_tx_control *ctrl,
				     struct ieee80211_hdr *hdr,
				     int sta_id, int tx_id);
extern int iwl4965_hw_reg_send_txpower(struct iwl4965_priv *priv);
extern int iwl4965_hw_reg_set_txpower(struct iwl4965_priv *priv, s8 power);
extern void iwl4965_hw_rx_statistics(struct iwl4965_priv *priv,
				 struct iwl4965_rx_mem_buffer *rxb);
extern void iwl4965_disable_events(struct iwl4965_priv *priv);
extern int iwl4965_get_temperature(const struct iwl4965_priv *priv);

/**
 * iwl4965_hw_find_station - Find station id for a given BSSID
 * @bssid: MAC address of station ID to find
 *
 * NOTE:  This should not be hardware specific but the code has
 * not yet been merged into a single common layer for managing the
 * station tables.
 */
extern u8 iwl4965_hw_find_station(struct iwl4965_priv *priv, const u8 *bssid);

extern int iwl4965_hw_channel_switch(struct iwl4965_priv *priv, u16 channel);
extern int iwl4965_tx_queue_reclaim(struct iwl4965_priv *priv, int txq_id, int index);

struct iwl4965_priv;
struct sta_ht_info;

/*
 * Forward declare iwl-4965.c functions for iwl-base.c
 */
extern int iwl4965_eeprom_acquire_semaphore(struct iwl4965_priv *priv);
extern void iwl4965_eeprom_release_semaphore(struct iwl4965_priv *priv);

extern int iwl4965_tx_queue_update_wr_ptr(struct iwl4965_priv *priv,
					  struct iwl4965_tx_queue *txq,
					  u16 byte_cnt);
extern void iwl4965_add_station(struct iwl4965_priv *priv, const u8 *addr,
				int is_ap);
extern void iwl4965_set_rxon_ht(struct iwl4965_priv *priv,
				struct sta_ht_info *ht_info);

extern void iwl4965_set_rxon_chain(struct iwl4965_priv *priv);
extern int iwl4965_tx_cmd(struct iwl4965_priv *priv, struct iwl4965_cmd *out_cmd,
			  u8 sta_id, dma_addr_t txcmd_phys,
			  struct ieee80211_hdr *hdr, u8 hdr_len,
			  struct ieee80211_tx_control *ctrl, void *sta_in);
extern int iwl4965_alive_notify(struct iwl4965_priv *priv);
extern void iwl4965_update_rate_scaling(struct iwl4965_priv *priv, u8 mode);
extern void iwl4965_set_ht_add_station(struct iwl4965_priv *priv, u8 index);

extern void iwl4965_chain_noise_reset(struct iwl4965_priv *priv);
extern void iwl4965_init_sensitivity(struct iwl4965_priv *priv, u8 flags,
				     u8 force);
extern int iwl4965_set_fat_chan_info(struct iwl4965_priv *priv, int phymode,
				u16 channel,
				const struct iwl4965_eeprom_channel *eeprom_ch,
				u8 fat_extension_channel);
extern void iwl4965_rf_kill_ct_config(struct iwl4965_priv *priv);

#ifdef CONFIG_IWL4965_HT
#ifdef CONFIG_IWL4965_HT_AGG
extern int iwl4965_mac_ht_tx_agg_start(struct ieee80211_hw *hw, u8 *da,
				   u16 tid, u16 *start_seq_num);
extern int iwl4965_mac_ht_rx_agg_start(struct ieee80211_hw *hw, u8 *da,
				   u16 tid, u16 start_seq_num);
extern int iwl4965_mac_ht_rx_agg_stop(struct ieee80211_hw *hw, u8 *da,
				  u16 tid, int generator);
extern int iwl4965_mac_ht_tx_agg_stop(struct ieee80211_hw *hw, u8 *da,
				  u16 tid, int generator);
extern void iwl4965_turn_off_agg(struct iwl4965_priv *priv, u8 tid);
#endif /* CONFIG_IWL4965_HT_AGG */
#endif /*CONFIG_IWL4965_HT */
/* Structures, enum, and defines specific to the 4965 */

#define IWL4965_KW_SIZE 0x1000	/*4k */

struct iwl4965_kw {
	dma_addr_t dma_addr;
	void *v_addr;
	size_t size;
};

#define TID_QUEUE_CELL_SPACING 50	/*mS */
#define TID_QUEUE_MAX_SIZE     20
#define TID_ROUND_VALUE        5	/* mS */
#define TID_MAX_LOAD_COUNT     8

#define TID_MAX_TIME_DIFF ((TID_QUEUE_MAX_SIZE - 1) * TID_QUEUE_CELL_SPACING)
#define TIME_WRAP_AROUND(x, y) (((y) > (x)) ? (y) - (x) : (0-(x)) + (y))

#define TID_ALL_ENABLED		0x7f
#define TID_ALL_SPECIFIED       0xff
#define TID_AGG_TPT_THREHOLD    0x0

#define IWL_CHANNEL_WIDTH_20MHZ   0
#define IWL_CHANNEL_WIDTH_40MHZ   1

#define IWL_MIMO_PS_STATIC        0
#define IWL_MIMO_PS_NONE          3
#define IWL_MIMO_PS_DYNAMIC       1
#define IWL_MIMO_PS_INVALID       2

#define IWL_OPERATION_MODE_AUTO     0
#define IWL_OPERATION_MODE_HT_ONLY  1
#define IWL_OPERATION_MODE_MIXED    2
#define IWL_OPERATION_MODE_20MHZ    3

#define IWL_EXT_CHANNEL_OFFSET_AUTO   0
#define IWL_EXT_CHANNEL_OFFSET_ABOVE  1
#define IWL_EXT_CHANNEL_OFFSET_       2
#define IWL_EXT_CHANNEL_OFFSET_BELOW  3
#define IWL_EXT_CHANNEL_OFFSET_MAX    4

#define NRG_NUM_PREV_STAT_L     20
#define NUM_RX_CHAINS           (3)

#define TX_POWER_IWL_ILLEGAL_VDET    -100000
#define TX_POWER_IWL_ILLEGAL_VOLTAGE -10000
#define TX_POWER_IWL_CLOSED_LOOP_MIN_POWER 18
#define TX_POWER_IWL_CLOSED_LOOP_MAX_POWER 34
#define TX_POWER_IWL_VDET_SLOPE_BELOW_NOMINAL 17
#define TX_POWER_IWL_VDET_SLOPE_ABOVE_NOMINAL 20
#define TX_POWER_IWL_NOMINAL_POWER            26
#define TX_POWER_IWL_CLOSED_LOOP_ITERATION_LIMIT 1
#define TX_POWER_IWL_VOLTAGE_CODES_PER_03V       7
#define TX_POWER_IWL_DEGREES_PER_VDET_CODE       11
#define IWL_TX_POWER_MAX_NUM_PA_MEASUREMENTS 1
#define IWL_TX_POWER_CCK_COMPENSATION_B_STEP (9)
#define IWL_TX_POWER_CCK_COMPENSATION_C_STEP (5)

struct iwl4965_traffic_load {
	unsigned long time_stamp;
	u32 packet_count[TID_QUEUE_MAX_SIZE];
	u8 queue_count;
	u8 head;
	u32 total;
};

#ifdef CONFIG_IWL4965_HT_AGG
struct iwl4965_agg_control {
	unsigned long next_retry;
	u32 wait_for_agg_status;
	u32 tid_retry;
	u32 requested_ba;
	u32 granted_ba;
	u8 auto_agg;
	u32 tid_traffic_load_threshold;
	u32 ba_timeout;
	struct iwl4965_traffic_load traffic_load[TID_MAX_LOAD_COUNT];
};
#endif				/*CONFIG_IWL4965_HT_AGG */

struct iwl4965_lq_mngr {
#ifdef CONFIG_IWL4965_HT_AGG
	struct iwl4965_agg_control agg_ctrl;
#endif
	spinlock_t lock;
	s32 max_window_size;
	s32 *expected_tpt;
	u8 *next_higher_rate;
	u8 *next_lower_rate;
	unsigned long stamp;
	unsigned long stamp_last;
	u32 flush_time;
	u32 tx_packets;
	u8 lq_ready;
};


/* Sensitivity and chain noise calibration */
#define INTERFERENCE_DATA_AVAILABLE	__constant_cpu_to_le32(1)
#define INITIALIZATION_VALUE		0xFFFF
#define CAL_NUM_OF_BEACONS		20
#define MAXIMUM_ALLOWED_PATHLOSS	15

/* Param table within SENSITIVITY_CMD */
#define HD_MIN_ENERGY_CCK_DET_INDEX                 (0)
#define HD_MIN_ENERGY_OFDM_DET_INDEX                (1)
#define HD_AUTO_CORR32_X1_TH_ADD_MIN_INDEX          (2)
#define HD_AUTO_CORR32_X1_TH_ADD_MIN_MRC_INDEX      (3)
#define HD_AUTO_CORR40_X4_TH_ADD_MIN_MRC_INDEX      (4)
#define HD_AUTO_CORR32_X4_TH_ADD_MIN_INDEX          (5)
#define HD_AUTO_CORR32_X4_TH_ADD_MIN_MRC_INDEX      (6)
#define HD_BARKER_CORR_TH_ADD_MIN_INDEX             (7)
#define HD_BARKER_CORR_TH_ADD_MIN_MRC_INDEX         (8)
#define HD_AUTO_CORR40_X4_TH_ADD_MIN_INDEX          (9)
#define HD_OFDM_ENERGY_TH_IN_INDEX                  (10)

#define SENSITIVITY_CMD_CONTROL_DEFAULT_TABLE	__constant_cpu_to_le16(0)
#define SENSITIVITY_CMD_CONTROL_WORK_TABLE	__constant_cpu_to_le16(1)

#define CHAIN_NOISE_MAX_DELTA_GAIN_CODE 3

#define MAX_FA_OFDM  50
#define MIN_FA_OFDM  5
#define MAX_FA_CCK   50
#define MIN_FA_CCK   5

#define NRG_MIN_CCK  97
#define NRG_MAX_CCK  0

#define AUTO_CORR_MIN_OFDM        85
#define AUTO_CORR_MIN_OFDM_MRC    170
#define AUTO_CORR_MIN_OFDM_X1     105
#define AUTO_CORR_MIN_OFDM_MRC_X1 220
#define AUTO_CORR_MAX_OFDM        120
#define AUTO_CORR_MAX_OFDM_MRC    210
#define AUTO_CORR_MAX_OFDM_X1     140
#define AUTO_CORR_MAX_OFDM_MRC_X1 270
#define AUTO_CORR_STEP_OFDM       1

#define AUTO_CORR_MIN_CCK      (125)
#define AUTO_CORR_MAX_CCK      (200)
#define AUTO_CORR_MIN_CCK_MRC  200
#define AUTO_CORR_MAX_CCK_MRC  400
#define AUTO_CORR_STEP_CCK     3
#define AUTO_CORR_MAX_TH_CCK   160

#define NRG_ALG                0
#define AUTO_CORR_ALG          1
#define NRG_DIFF               2
#define NRG_STEP_CCK           2
#define NRG_MARGIN             8
#define MAX_NUMBER_CCK_NO_FA 100

#define AUTO_CORR_CCK_MIN_VAL_DEF    (125)

#define CHAIN_A             0
#define CHAIN_B             1
#define CHAIN_C             2
#define CHAIN_NOISE_DELTA_GAIN_INIT_VAL 4
#define ALL_BAND_FILTER			0xFF00
#define IN_BAND_FILTER			0xFF
#define MIN_AVERAGE_NOISE_MAX_VALUE	0xFFFFFFFF

enum iwl4965_false_alarm_state {
	IWL_FA_TOO_MANY = 0,
	IWL_FA_TOO_FEW = 1,
	IWL_FA_GOOD_RANGE = 2,
};

enum iwl4965_chain_noise_state {
	IWL_CHAIN_NOISE_ALIVE = 0,  /* must be 0 */
	IWL_CHAIN_NOISE_ACCUMULATE = 1,
	IWL_CHAIN_NOISE_CALIBRATED = 2,
};

enum iwl4965_sensitivity_state {
	IWL_SENS_CALIB_ALLOWED = 0,
	IWL_SENS_CALIB_NEED_REINIT = 1,
};

enum iwl4965_calib_enabled_state {
	IWL_CALIB_DISABLED = 0,  /* must be 0 */
	IWL_CALIB_ENABLED = 1,
};

struct statistics_general_data {
	u32 beacon_silence_rssi_a;
	u32 beacon_silence_rssi_b;
	u32 beacon_silence_rssi_c;
	u32 beacon_energy_a;
	u32 beacon_energy_b;
	u32 beacon_energy_c;
};

/* Sensitivity calib data */
struct iwl4965_sensitivity_data {
	u32 auto_corr_ofdm;
	u32 auto_corr_ofdm_mrc;
	u32 auto_corr_ofdm_x1;
	u32 auto_corr_ofdm_mrc_x1;
	u32 auto_corr_cck;
	u32 auto_corr_cck_mrc;

	u32 last_bad_plcp_cnt_ofdm;
	u32 last_fa_cnt_ofdm;
	u32 last_bad_plcp_cnt_cck;
	u32 last_fa_cnt_cck;

	u32 nrg_curr_state;
	u32 nrg_prev_state;
	u32 nrg_value[10];
	u8  nrg_silence_rssi[NRG_NUM_PREV_STAT_L];
	u32 nrg_silence_ref;
	u32 nrg_energy_idx;
	u32 nrg_silence_idx;
	u32 nrg_th_cck;
	s32 nrg_auto_corr_silence_diff;
	u32 num_in_cck_no_fa;
	u32 nrg_th_ofdm;

	u8 state;
};

/* Chain noise (differential Rx gain) calib data */
struct iwl4965_chain_noise_data {
	u8 state;
	u16 beacon_count;
	u32 chain_noise_a;
	u32 chain_noise_b;
	u32 chain_noise_c;
	u32 chain_signal_a;
	u32 chain_signal_b;
	u32 chain_signal_c;
	u8 disconn_array[NUM_RX_CHAINS];
	u8 delta_gain_code[NUM_RX_CHAINS];
	u8 radio_write;
};

/* IWL4965 */
#define RATE_MCS_CODE_MSK 0x7
#define RATE_MCS_MIMO_POS 3
#define RATE_MCS_MIMO_MSK 0x8
#define RATE_MCS_HT_DUP_POS 5
#define RATE_MCS_HT_DUP_MSK 0x20
#define RATE_MCS_FLAGS_POS 8
#define RATE_MCS_HT_POS 8
#define RATE_MCS_HT_MSK 0x100
#define RATE_MCS_CCK_POS 9
#define RATE_MCS_CCK_MSK 0x200
#define RATE_MCS_GF_POS 10
#define RATE_MCS_GF_MSK 0x400

#define RATE_MCS_FAT_POS 11
#define RATE_MCS_FAT_MSK 0x800
#define RATE_MCS_DUP_POS 12
#define RATE_MCS_DUP_MSK 0x1000
#define RATE_MCS_SGI_POS 13
#define RATE_MCS_SGI_MSK 0x2000

#define	EEPROM_SEM_TIMEOUT 10
#define EEPROM_SEM_RETRY_LIMIT 1000


#ifdef CONFIG_IWL4965_SPECTRUM_MEASUREMENT

enum {
	MEASUREMENT_READY = (1 << 0),
	MEASUREMENT_ACTIVE = (1 << 1),
};

#endif

struct iwl4965_priv {

	/* ieee device used by generic ieee processing code */
	struct ieee80211_hw *hw;
	struct ieee80211_channel *ieee_channels;
	struct ieee80211_rate *ieee_rates;

	/* temporary frame storage list */
	struct list_head free_frames;
	int frames_count;

	u8 phymode;
	int alloc_rxb_skb;

	void (*rx_handlers[REPLY_MAX])(struct iwl4965_priv *priv,
				       struct iwl4965_rx_mem_buffer *rxb);

	const struct ieee80211_hw_mode *modes;

#ifdef CONFIG_IWL4965_SPECTRUM_MEASUREMENT
	/* spectrum measurement report caching */
	struct iwl4965_spectrum_notification measure_report;
	u8 measurement_status;
#endif
	/* ucode beacon time */
	u32 ucode_beacon_time;

	/* we allocate array of iwl4965_channel_info for NIC's valid channels.
	 *    Access via channel # using indirect index array */
	struct iwl4965_channel_info *channel_info;	/* channel info array */
	u8 channel_count;	/* # of channels */

	/* each calibration channel group in the EEPROM has a derived
	 * clip setting for each rate. */
	const struct iwl4965_clip_group clip_groups[5];

	/* thermal calibration */
	s32 temperature;	/* degrees Kelvin */
	s32 last_temperature;

	/* Scan related variables */
	unsigned long last_scan_jiffies;
	unsigned long scan_start;
	unsigned long scan_pass_start;
	unsigned long scan_start_tsf;
	int scan_bands;
	int one_direct_scan;
	u8 direct_ssid_len;
	u8 direct_ssid[IW_ESSID_MAX_SIZE];
	struct iwl4965_scan_cmd *scan;
	u8 only_active_channel;

	/* spinlock */
	spinlock_t lock;	/* protect general shared data */
	spinlock_t hcmd_lock;	/* protect hcmd */
	struct mutex mutex;

	/* basic pci-network driver stuff */
	struct pci_dev *pci_dev;

	/* pci hardware address support */
	void __iomem *hw_base;

	/* uCode images, save to reload in case of failure */
	struct fw_desc ucode_code;	/* runtime inst */
	struct fw_desc ucode_data;	/* runtime data original */
	struct fw_desc ucode_data_backup;	/* runtime data save/restore */
	struct fw_desc ucode_init;	/* initialization inst */
	struct fw_desc ucode_init_data;	/* initialization data */
	struct fw_desc ucode_boot;	/* bootstrap inst */


	struct iwl4965_rxon_time_cmd rxon_timing;

	/* We declare this const so it can only be
	 * changed via explicit cast within the
	 * routines that actually update the physical
	 * hardware */
	const struct iwl4965_rxon_cmd active_rxon;
	struct iwl4965_rxon_cmd staging_rxon;

	int error_recovering;
	struct iwl4965_rxon_cmd recovery_rxon;

	/* 1st responses from initialize and runtime uCode images.
	 * 4965's initialize alive response contains some calibration data. */
	struct iwl4965_init_alive_resp card_alive_init;
	struct iwl4965_alive_resp card_alive;

#ifdef LED
	/* LED related variables */
	struct iwl4965_activity_blink activity;
	unsigned long led_packets;
	int led_state;
#endif

	u16 active_rate;
	u16 active_rate_basic;

	u8 call_post_assoc_from_beacon;
	u8 assoc_station_added;
	u8 use_ant_b_for_management_frame;	/* Tx antenna selection */
	/* HT variables */
	u8 is_dup;
	u8 is_ht_enabled;
	u8 channel_width;	/* 0=20MHZ, 1=40MHZ */
	u8 current_channel_width;
	u8 valid_antenna;	/* Bit mask of antennas actually connected */
#ifdef CONFIG_IWL4965_SENSITIVITY
	struct iwl4965_sensitivity_data sensitivity_data;
	struct iwl4965_chain_noise_data chain_noise_data;
	u8 start_calib;
	__le16 sensitivity_tbl[HD_TABLE_SIZE];
#endif /*CONFIG_IWL4965_SENSITIVITY*/

#ifdef CONFIG_IWL4965_HT
	struct sta_ht_info current_assoc_ht;
#endif
	u8 active_rate_ht[2];
	u8 last_phy_res[100];

	/* Rate scaling data */
	struct iwl4965_lq_mngr lq_mngr;

	/* Rate scaling data */
	s8 data_retry_limit;
	u8 retry_rate;

	wait_queue_head_t wait_command_queue;

	int activity_timer_active;

	/* Rx and Tx DMA processing queues */
	struct iwl4965_rx_queue rxq;
	struct iwl4965_tx_queue txq[IWL_MAX_NUM_QUEUES];
	unsigned long txq_ctx_active_msk;
	struct iwl4965_kw kw;	/* keep warm address */
	u32 scd_base_addr;	/* scheduler sram base address */

	unsigned long status;
	u32 config;

	int last_rx_rssi;	/* From Rx packet statisitics */
	int last_rx_noise;	/* From beacon statistics */

	struct iwl4965_power_mgr power_data;

	struct iwl4965_notif_statistics statistics;
	unsigned long last_statistics_time;

	/* context information */
	u8 essid[IW_ESSID_MAX_SIZE];
	u8 essid_len;
	u16 rates_mask;

	u32 power_mode;
	u32 antenna;
	u8 bssid[ETH_ALEN];
	u16 rts_threshold;
	u8 mac_addr[ETH_ALEN];

	/*station table variables */
	spinlock_t sta_lock;
	int num_stations;
	struct iwl4965_station_entry stations[IWL_STATION_COUNT];

	/* Indication if ieee80211_ops->open has been called */
	int is_open;

	u8 mac80211_registered;
	int is_abg;

	u32 notif_missed_beacons;

	/* Rx'd packet timing information */
	u32 last_beacon_time;
	u64 last_tsf;

	/* Duplicate packet detection */
	u16 last_seq_num;
	u16 last_frag_num;
	unsigned long last_packet_time;
	struct list_head ibss_mac_hash[IWL_IBSS_MAC_HASH_SIZE];

	/* eeprom */
	struct iwl4965_eeprom eeprom;

	int iw_mode;

	struct sk_buff *ibss_beacon;

	/* Last Rx'd beacon timestamp */
	u32 timestamp0;
	u32 timestamp1;
	u16 beacon_int;
	struct iwl4965_driver_hw_info hw_setting;
	int interface_id;

	/* Current association information needed to configure the
	 * hardware */
	u16 assoc_id;
	u16 assoc_capability;
	u8 ps_mode;

#ifdef CONFIG_IWL4965_QOS
	struct iwl4965_qos_info qos_data;
#endif /*CONFIG_IWL4965_QOS */

	struct workqueue_struct *workqueue;

	struct work_struct up;
	struct work_struct restart;
	struct work_struct calibrated_work;
	struct work_struct scan_completed;
	struct work_struct rx_replenish;
	struct work_struct rf_kill;
	struct work_struct abort_scan;
	struct work_struct update_link_led;
	struct work_struct auth_work;
	struct work_struct report_work;
	struct work_struct request_scan;
	struct work_struct beacon_update;

	struct tasklet_struct irq_tasklet;

	struct delayed_work init_alive_start;
	struct delayed_work alive_start;
	struct delayed_work activity_timer;
	struct delayed_work thermal_periodic;
	struct delayed_work gather_stats;
	struct delayed_work scan_check;
	struct delayed_work post_associate;

#define IWL_DEFAULT_TX_POWER 0x0F
	s8 user_txpower_limit;
	s8 max_channel_txpower_limit;

#ifdef CONFIG_PM
	u32 pm_state[16];
#endif

#ifdef CONFIG_IWL4965_DEBUG
	/* debugging info */
	u32 framecnt_to_us;
	atomic_t restrict_refcnt;
#endif

	struct work_struct txpower_work;
#ifdef CONFIG_IWL4965_SENSITIVITY
	struct work_struct sensitivity_work;
#endif
	struct work_struct statistics_work;
	struct timer_list statistics_periodic;

#ifdef CONFIG_IWL4965_HT_AGG
	struct work_struct agg_work;
#endif
};				/*iwl4965_priv */

static inline int iwl4965_is_associated(struct iwl4965_priv *priv)
{
	return (priv->active_rxon.filter_flags & RXON_FILTER_ASSOC_MSK) ? 1 : 0;
}

static inline int is_channel_valid(const struct iwl4965_channel_info *ch_info)
{
	if (ch_info == NULL)
		return 0;
	return (ch_info->flags & EEPROM_CHANNEL_VALID) ? 1 : 0;
}

static inline int is_channel_narrow(const struct iwl4965_channel_info *ch_info)
{
	return (ch_info->flags & EEPROM_CHANNEL_NARROW) ? 1 : 0;
}

static inline int is_channel_radar(const struct iwl4965_channel_info *ch_info)
{
	return (ch_info->flags & EEPROM_CHANNEL_RADAR) ? 1 : 0;
}

static inline u8 is_channel_a_band(const struct iwl4965_channel_info *ch_info)
{
	return ch_info->phymode == MODE_IEEE80211A;
}

static inline u8 is_channel_bg_band(const struct iwl4965_channel_info *ch_info)
{
	return ((ch_info->phymode == MODE_IEEE80211B) ||
		(ch_info->phymode == MODE_IEEE80211G));
}

static inline int is_channel_passive(const struct iwl4965_channel_info *ch)
{
	return (!(ch->flags & EEPROM_CHANNEL_ACTIVE)) ? 1 : 0;
}

static inline int is_channel_ibss(const struct iwl4965_channel_info *ch)
{
	return ((ch->flags & EEPROM_CHANNEL_IBSS)) ? 1 : 0;
}

extern const struct iwl4965_channel_info *iwl4965_get_channel_info(
	const struct iwl4965_priv *priv, int phymode, u16 channel);

/* Requires full declaration of iwl4965_priv before including */
#include "iwl-4965-io.h"

#endif				/* __iwl4965_4965_h__ */
