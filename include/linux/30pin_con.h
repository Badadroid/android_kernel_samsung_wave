/*
 * Copyright (C) 2012 Samsung Electronics, Inc.
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

#ifndef __ASM_ARCH_ACC_CONN_H
#define __ASM_ARCH_ACC_CONN_H

enum accessory_type {
	ACCESSORY_NONE = 0,
	ACCESSORY_TVOUT,
	ACCESSORY_LINEOUT,
	ACCESSORY_CARMOUNT,
	ACCESSORY_OTG,
	ACCESSORY_UNKNOWN,
};

enum dock_type {
	DOCK_NONE = 0,
	DOCK_DESK,
	DOCK_KEYBOARD,
};

enum acc_type {
	P30_OTG = 0,
	P30_EARJACK_WITH_DOCK,
	P30_CARDOCK,
	P30_ANAL_TV_OUT,
	P30_KEYBOARDDOCK,
	P30_DESKDOCK,
	P30_USB,
	P30_TA,
};

extern bool enable_audio_usb;

#endif
