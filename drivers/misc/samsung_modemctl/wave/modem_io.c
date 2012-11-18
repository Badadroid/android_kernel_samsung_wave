/* modem_io.c
 *
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

/* TODO
 * - on modem crash return -ENODEV from recv/send, poll==readable
 * - ensure all modem off/reset cases fault out io properly
 * - request thread irq?
 * - stats/debugfs
 * - purge txq on restart
 * - test, test, test
 */

#include <linux/init.h>
#include <linux/module.h>
#include <linux/mutex.h>

#include <linux/netdevice.h>
#include <linux/skbuff.h>
#include <net/sock.h>
#include <linux/if.h>
#include <linux/if_arp.h>

#include <linux/circ_buf.h>
#include <linux/wakelock.h>

#include "modem_ctl.h"
#include "modem_ctl_p.h"


/* general purpose fifo access routines */

typedef void * (*copyfunc)(void *, const void *, __kernel_size_t);

static void *x_copy_to_user(void *dst, const void *src, __kernel_size_t sz)
{
	if (copy_to_user((void __user *) dst, src, sz) != 0)
		pr_err("modemctl: cannot copy userdata\n");
	return dst;
}

static void *x_copy_from_user(void *dst, const void *src, __kernel_size_t sz)
{
	if (copy_from_user(dst, (const void __user *) src, sz) != 0)
		pr_err("modemctl: cannot copy userdata\n");
	return dst;
}

static unsigned _fifo_read(struct m_fifo *q, void *dst,
			   unsigned count, copyfunc copy)
{
	unsigned n;
	unsigned head = *q->head;
	unsigned tail = *q->tail;
	unsigned size = q->size;

	if (CIRC_CNT(head, tail, size) < count)
		return 0;

	n = CIRC_CNT_TO_END(head, tail, size);

	if (likely(n >= count)) {
		copy(dst, q->data + tail, count);
	} else {
		copy(dst, q->data + tail, n);
		copy(dst + n, q->data, count - n);
	}
	//*q->tail = (tail + count) & (size - 1);

	return count;
}
static void fifo_move_tail(struct m_fifo *q, unsigned count)
{
	unsigned tail = *q->tail;
	unsigned size = q->size;

	*q->tail = (tail + count) & (size - 1);
}
static void fifo_move_head(struct m_fifo *q, unsigned count)
{
	unsigned head = *q->head;
	unsigned size = q->size;

	*q->head = (head + count) & (size - 1);
}


static unsigned _fifo_write(struct m_fifo *q, void *src,
			    unsigned count, copyfunc copy)
{
	unsigned n;
	unsigned head = *q->head;
	unsigned tail = *q->tail;
	unsigned size = q->size;

	if (CIRC_SPACE(head, tail, size) < count)
		return 0;

	n = CIRC_SPACE_TO_END(head, tail, size);

	if (likely(n >= count)) {
		copy(q->data + head, src, count);
	} else {
		copy(q->data + head, src, n);
		copy(q->data, src + n, count - n);
	}
	//*q->head = (head + count) & (size - 1);

	return count;
}

static void fifo_purge(struct m_fifo *q)
{
	*q->head = 0;
	*q->tail = 0;
}
/*
static unsigned fifo_skip(struct m_fifo *q, unsigned count)
{
	if (CIRC_CNT(*q->head, *q->tail, q->size) < count)
		return 0;
	*q->tail = (*q->tail + count) & (q->size - 1);
	return count;
}*/

#define fifo_read(q, dst, count) \
	_fifo_read(q, dst, count, memcpy)
#define fifo_read_user(q, dst, count) \
	_fifo_read(q, dst, count, x_copy_to_user)

#define fifo_write(q, src, count) \
	_fifo_write(q, src, count, memcpy)
#define fifo_write_user(q, src, count) \
	_fifo_write(q, src, count, x_copy_from_user)

#define fifo_count(mf) CIRC_CNT(*(mf)->head, *(mf)->tail, (mf)->size)
#define fifo_space(mf) CIRC_SPACE(*(mf)->head, *(mf)->tail, (mf)->size)

static void fifo_dump(const char *tag, struct m_fifo *q,
		      unsigned start, unsigned count)
{
	if (count > 64)
		count = 64;

	if ((start + count) <= q->size) {
		print_hex_dump_bytes(tag, DUMP_PREFIX_ADDRESS,
				     q->data + start, count);
	} else {
		print_hex_dump_bytes(tag, DUMP_PREFIX_ADDRESS,
				     q->data + start, q->size - start);
		print_hex_dump_bytes(tag, DUMP_PREFIX_ADDRESS,
				     q->data, count - (q->size - start));
	}
}

/* Called with mc->lock held whenever we gain access
 * to the mmio region.
 */
void modem_update_state(struct modemctl *mc)
{
	/* update our idea of space available in fifos */
	mc->packet_tx.avail = fifo_space(&mc->packet_tx);
	mc->packet_rx.avail = fifo_count(&mc->packet_rx);
	if (mc->packet_rx.avail)
		wake_lock(&mc->packet_pipe.wakelock);
	else
		wake_unlock(&mc->packet_pipe.wakelock);

	/* wake up blocked or polling read/write operations */
	wake_up(&mc->wq);
}

void modem_update_pipe(struct m_pipe *pipe)
{
	unsigned long flags;
	spin_lock_irqsave(&pipe->mc->lock, flags);
	pipe->tx->avail = fifo_space(pipe->tx);
	pipe->rx->avail = fifo_count(pipe->rx);
	if (pipe->rx->avail)
		wake_lock(&pipe->wakelock);
	else
		wake_unlock(&pipe->wakelock);
	spin_unlock_irqrestore(&pipe->mc->lock, flags);
}


/* must be called with pipe->tx_lock held */
static int modem_pipe_send(struct m_pipe *pipe, struct modem_io *io)
{
	char hdr[M_PIPE_MAX_HDR];
	unsigned size;
	int ret;

	io->magic = 0xCAFECAFE;

	ret = pipe->push_header(io, hdr);
	if (ret)
		return ret;

	size = io->datasize + pipe->header_size;

	if (size > 0x1000 /*pipe->tx->size*/)
	{
		pr_err ("Trying to send bigger than 4kB frame - MULTIPACKET not implemented yet.\n");
		return -EINVAL;
	}

	for (;;) {
		ret = modem_acquire_mmio(pipe->mc);
		if (ret)
			return ret;

		modem_update_pipe(pipe);

		if (pipe->tx->avail >= size) {
			fifo_write(pipe->tx, hdr, pipe->header_size);
			fifo_move_head(pipe->tx, pipe->header_size);
			fifo_write_user(pipe->tx, io->data, io->datasize);
			fifo_move_head(pipe->tx, SIZ_PACKET_BUFSIZE);
			modem_update_pipe(pipe);
			modem_release_mmio(pipe->mc, pipe->tx->bits);
			MODEM_COUNT(pipe->mc, pipe_tx);
			return 0;
		}

		pr_info("modem_pipe_send: wait for space\n");
		MODEM_COUNT(pipe->mc, pipe_tx_delayed);
		modem_release_mmio(pipe->mc, 0);

		ret = wait_event_interruptible(pipe->mc->wq,
					       (pipe->tx->avail >= size)
					       || modem_offline(pipe->mc));
		if (ret)
			return ret;
	}
}

static int modem_pipe_read(struct m_pipe *pipe, struct modem_io *io)
{
	char hdr[M_PIPE_MAX_HDR];
	int ret;

	if (fifo_read(pipe->rx, hdr, pipe->header_size) == 0)
		return -EAGAIN;

	fifo_move_tail(pipe->rx, pipe->header_size);

	ret = pipe->pull_header(io, hdr);
	if (ret)
		return ret;

	if(io->magic != 0xCAFECAFE)
	{
		pr_err("modem_pipe_read: io->magic != 0xCAFECAFE, possible FIFO corruption\n");
		return -EIO;
	}

	if (fifo_read_user(pipe->rx, io->data, io->datasize) != io->datasize)
		return -EIO;
	fifo_move_tail(pipe->rx, SIZ_PACKET_BUFSIZE);

	return 0;
}

/* must be called with pipe->rx_lock held */
static int modem_pipe_recv(struct m_pipe *pipe, struct modem_io *io)
{
	int ret;

	ret = modem_acquire_mmio(pipe->mc);
	if (ret)
		return ret;

	ret = modem_pipe_read(pipe, io);

	modem_update_pipe(pipe);

	if ((ret != 0) && (ret != -EAGAIN)) {
		pr_err("[MODEM] purging %s fifo\n", pipe->dev.name);
		fifo_purge(pipe->rx);
		MODEM_COUNT(pipe->mc, pipe_rx_purged);
	} else if (ret == 0) {
		MODEM_COUNT(pipe->mc, pipe_rx);
	}

	modem_release_mmio(pipe->mc, 0);

	return ret;
}

struct packet_hdr {
	u32 magic;
	u32 cmd;
	u32 datasize;
} __attribute__ ((packed));

static int push_packet_header(struct modem_io *io, void *header)
{
	struct packet_hdr *ph = header;

	ph->magic = io->magic;
	ph->cmd = io->cmd;
	ph->datasize = io->datasize;
	return 0;
}

static int pull_packet_header(struct modem_io *io, void *header)
{
	struct packet_hdr *ph = header;

	io->magic = ph->magic;
	io->cmd = ph->cmd;
	io->datasize = ph->datasize;
	return 0;
}

static int pipe_open(struct inode *inode, struct file *filp)
{
	struct m_pipe *pipe = to_m_pipe(filp->private_data);
	filp->private_data = pipe;
	return 0;
}

static long pipe_ioctl(struct file *filp, unsigned int cmd, unsigned long _arg)
{
	void __user *arg = (void *) _arg;
	struct m_pipe *pipe = filp->private_data;
	struct modem_io mio;
	int ret;

	switch (cmd) {
	case IOCTL_MODEM_SEND:
		if (copy_from_user(&mio, arg, sizeof(mio)) != 0)
			return -EFAULT;
		if (mutex_lock_interruptible(&pipe->tx_lock))
			return -EINTR;
		ret = modem_pipe_send(pipe, &mio);
		mutex_unlock(&pipe->tx_lock);
		if(ret)
			return -EFAULT;
		break;
	case IOCTL_MODEM_RECV:
		if (copy_from_user(&mio, arg, sizeof(mio)) != 0)
		{		
			pr_info("modem_io: cannot copy_from_user\n");
			return -EFAULT;
		}
		if (mutex_lock_interruptible(&pipe->rx_lock))
		{		
			pr_info("modem_io: cannot mutex_lock_interruptible\n");
			return -EINTR;
		}
		ret = modem_pipe_recv(pipe, &mio);
		mutex_unlock(&pipe->rx_lock);
		if (copy_to_user(arg, &mio, sizeof(mio)) != 0)
			return -EFAULT;
		if(ret)
			return -EFAULT;
		break;
	default:
		return -EINVAL;
	}
	return 0;
}

static unsigned int pipe_poll(struct file *filp, poll_table *wait)
{
	unsigned long flags;
	struct m_pipe *pipe = filp->private_data;
	int ret;

	poll_wait(filp, &pipe->mc->wq, wait);

	spin_lock_irqsave(&pipe->mc->lock, flags);
	if (pipe->rx->avail || modem_offline(pipe->mc))
		ret = POLLIN | POLLRDNORM;
	else
		ret = 0;
	spin_unlock_irqrestore(&pipe->mc->lock, flags);

	return ret;
}

static const struct file_operations modem_io_fops = {
	.owner =		THIS_MODULE,
	.open =			pipe_open,
	.poll =			pipe_poll,
	.unlocked_ioctl =	pipe_ioctl,
};

static int modem_pipe_register(struct m_pipe *pipe, const char *devname)
{
	pipe->dev.minor = MISC_DYNAMIC_MINOR;
	pipe->dev.name = devname;
	pipe->dev.fops = &modem_io_fops;

	wake_lock_init(&pipe->wakelock, WAKE_LOCK_SUSPEND, devname);

	mutex_init(&pipe->tx_lock);
	mutex_init(&pipe->rx_lock);
	return misc_register(&pipe->dev);
}

int modem_io_init(struct modemctl *mc, void __iomem *mmio)
{

	INIT_M_FIFO(mc->packet_tx, PACKET, TX, mmio);
	INIT_M_FIFO(mc->packet_rx, PACKET, RX, mmio);


	mc->packet_pipe.tx = &mc->packet_tx;
	mc->packet_pipe.rx = &mc->packet_rx;
	mc->packet_pipe.tx->bits = MB_PACKET;
	mc->packet_pipe.push_header = push_packet_header;
	mc->packet_pipe.pull_header = pull_packet_header;
	mc->packet_pipe.header_size = sizeof(struct packet_hdr);
	mc->packet_pipe.mc = mc;
	if (modem_pipe_register(&mc->packet_pipe, "modem_packet"))
		pr_err("failed to register modem_packet pipe\n");

	pr_info("modem_io_init done\n");

	return 0;
}
