/*
 * Copyright (C) 2010 Google, Inc.
 * Copyright (C) 2010 Samsung Electronics.
 *
 * Modified by Dominik Marszk according to Mocha AP-CP protocol
 *
 * This software is licensed under the terms of the GNU General Public
 * License version 2, as published by the Free Software Foundation, and
 * may be copied, distributed, and modified under those terms.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 */

#ifndef __MODEM_CONTROL_P_H__
#define __MODEM_CONTROL_P_H__

#define MODEM_OFF		0
#define MODEM_CRASHED		1
#define MODEM_RAMDUMP		2
#define MODEM_POWER_ON		3
#define MODEM_BOOTING_NORMAL	4
#define MODEM_BOOTING_RAMDUMP	5
#define MODEM_DUMPING		6
#define MODEM_RUNNING		7

#define modem_offline(mc) ((mc)->status < MODEM_POWER_ON)
#define modem_running(mc) ((mc)->status == MODEM_RUNNING)
#define modem_operating(mc) ((mc)->status >= MODEM_POWER_ON)

#define M_PIPE_MAX_HDR 16

struct net_device;

struct m_pipe {
	int (*push_header)(struct modem_io *io, void *header);
	int (*pull_header)(struct modem_io *io, void *header);

	unsigned header_size;

	struct m_fifo *tx;
	struct m_fifo *rx;

	struct modemctl *mc;
	unsigned ready;

	struct miscdevice dev;

	struct mutex tx_lock;
	struct mutex rx_lock;

	struct wake_lock wakelock;
};
#define to_m_pipe(misc) container_of(misc, struct m_pipe, dev)

struct m_fifo {
	unsigned *head;
	unsigned *tail;
	unsigned size;
	void *data;

	unsigned avail;
	unsigned bits;
	unsigned unused1;
	unsigned unused2;
};

struct modemstats {
	unsigned request_no_wait;
	unsigned request_wait;

	unsigned release_no_action;
	unsigned release_bp_waiting;
	unsigned release_bp_signaled;

	unsigned bp_req_instant;
	unsigned bp_req_delayed;
	unsigned bp_req_confused;

	unsigned rx_unknown;
	unsigned rx_dropped;
	unsigned rx_purged;
	unsigned rx_received;

	unsigned tx_no_delay;
	unsigned tx_queued;
	unsigned tx_bp_signaled;
	unsigned tx_fifo_full;

	unsigned pipe_tx;
	unsigned pipe_rx;
	unsigned pipe_tx_delayed;
	unsigned pipe_rx_purged;

	unsigned resets;
};

#define MODEM_COUNT(mc,s) (((mc)->stats.s)++)

struct modemctl {
	void __iomem *mmio;
	struct modemstats stats;

	/* lock and waitqueue for shared memory state */
	spinlock_t lock;
	wait_queue_head_t wq;

	/* shared memory semaphore management */
	unsigned mmio_req_count;
	unsigned mmio_bp_request;
	unsigned mmio_owner;
	unsigned mmio_signal_bits;

	struct m_fifo packet_tx;
	struct m_fifo packet_rx;

	int open_count;
	int status;

	unsigned mmbase;
	unsigned mmsize;

	int irq_bp;
	int irq_mbox;

	unsigned gpio_phone_active;
	unsigned gpio_pda_active;
	unsigned gpio_cp_reset;

	unsigned gpio_phone_on;
	unsigned gpio_usim_boot;
	unsigned gpio_flm_sel;

	struct miscdevice dev;

	struct m_pipe packet_pipe;

	struct mutex ctl_lock;
	ktime_t mmio_t0;

	/* used for ramdump mode */
	unsigned ramdump_size;
	loff_t ramdump_pos;

	unsigned logdump;
	unsigned logdump_data;
};
#define to_modemctl(misc) container_of(misc, struct modemctl, dev)


/* called when semaphore is held and there may be io to process */
void modem_update_state(struct modemctl *mc);

/* called once at probe() */
int modem_io_init(struct modemctl *mc, void __iomem *mmio);

/* called when modem boots and goes offline */
void modem_io_enable(struct modemctl *mc);
void modem_io_disable(struct modemctl *mc);


/* Block until control of mmio area is obtained (0)
 * or interrupt (-ERESTARTSYS) or failure (-ENODEV)
 * occurs.
 */
int modem_acquire_mmio(struct modemctl *mc);

/* Request control of mmio area.  Returns 1 if
 * control obtained, 0 if not (request pending).
 * Either way, release_mmio() must be called to
 * balance this.
 */
int modem_request_mmio(struct modemctl *mc);

/* Return control of mmio area once requested
 * by modem_request_mmio() or acquired by a
 * successful modem_acquire_mmio().
 *
 * The onedram semaphore is only actually returned
 * to the BP if there is an outstanding request
 * for it from the BP, or if the bits argument
 * to one of the release_mmio() calls was nonzero.
 */
void modem_release_mmio(struct modemctl *mc, unsigned bits);

/* Send a request for the hw mmio sem to the modem.
 * Used ONLY by the internals of modem_request_mmio() and
 * some trickery in vnet_xmit().  Please do not use elsewhere.
 */
void modem_request_sem(struct modemctl *mc);


/* internal glue */
void modem_debugfs_init(struct modemctl *mc);
void modem_force_crash(struct modemctl *mc);

/* protocol definitions */

#define MB_REQ_SEM		0x1
#define MB_REL_SEM		0x2
#define MB_SEM_CTRL		0x1000
#define MB_PACKET		0x20000
#define MB_LPACKET		0x40000 //used in WAVE and newer



#define MODEM_MSG_SBL_DONE			0x12341234
#define MODEM_CMD_BINARY_LOAD		0x45674567
#define MODEM_CMD_AMSSRUNREQ		0x89EF89EF
#define MODEM_MSG_BINARY_DONE		0xABCDABCD



/* onedram shared memory map */
#define OFF_PACKET_RX_HEAD		0x00600010
#define OFF_PACKET_RX_TAIL		0x00600014
#define OFF_PACKET_RX_DATA		0x00600020

#define OFF_PACKET_TX_HEAD		0x00800020
#define OFF_PACKET_TX_TAIL		0x00800024
#define OFF_PACKET_TX_DATA		0x00800030


#define SIZ_PACKET_DATA			0x00200000




#define INIT_M_FIFO(name, type, dir, base) \
	name.head = base + OFF_##type##_##dir##_HEAD; \
	name.tail = base + OFF_##type##_##dir##_TAIL; \
	name.data = base + OFF_##type##_##dir##_DATA; \
	name.size = SIZ_##type##_DATA;

/* onedram registers */

/* Mailboxes are named based on who writes to them.
 * MBOX_BP is written to by the (B)aseband (P)rocessor
 * and only readable by the (A)pplication (P)rocessor.
 * MBOX_AP is the opposite.
 */
#define OFF_SEM		 	0xFFF800
#define OFF_MBOX_BP		0xFFF820
#define OFF_MBOX_AP		0xFFF840
#define OFF_CHECK_BP	0xFFF8A0
#define OFF_CHECK_AP	0xFFF8C0

#endif
