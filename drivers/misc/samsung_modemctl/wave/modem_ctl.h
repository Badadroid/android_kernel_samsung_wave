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

#ifndef __MODEM_CONTROL_H__
#define __MODEM_CONTROL_H__

#define IOCTL_MODEM_RAMDUMP             _IO('o', 0x19)
#define IOCTL_MODEM_RESET               _IO('o', 0x20)
#define IOCTL_MODEM_START               _IO('o', 0x21)
#define IOCTL_MODEM_OFF                 _IO('o', 0x22)

#define IOCTL_MODEM_SEND				_IO('o', 0x23)
#define IOCTL_MODEM_RECV				_IO('o', 0x24)

#define IOCTL_MODEM_ON               	_IO('o', 0x25)
#define	IOCTL_MODEM_AMSSRUNREQ			_IO('o', 0x26)
#define IOCTL_MODEM_GET_STATUS       	_IO('o', 0x27)
#define IOCTL_MODEM_SET_STATUS       	_IO('o', 0x28)


#define SIZ_PACKET_FRAME		0x00001000
#define	 SIZ_PACKET_HEADER		0x0000000C
#define	 SIZ_PACKET_BUFSIZE		SIZ_PACKET_FRAME-SIZ_PACKET_HEADER

struct modem_io {
	uint32_t magic; //filled by modem_io
	uint32_t cmd;
	uint32_t datasize;
	void *data;
};

/* platform data */
struct modemctl_data {
	const char *name;
	unsigned gpio_phone_active;
	unsigned gpio_pda_active;
	unsigned gpio_cp_reset;

	unsigned gpio_phone_on;
	unsigned gpio_usim_boot;
	unsigned gpio_flm_sel;
	unsigned gpio_sim_ndetect;


};

#endif
