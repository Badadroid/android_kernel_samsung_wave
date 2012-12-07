/*inclue/linux/lg4573.h
 *
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
*/
#ifndef __LG4573_H__
#define __LG4573_H__
#include <linux/types.h>


#define SLEEPMSEC		0x1000
#define ENDDEF			0x2000
#define DATAMASK		0x0100
#define	DEFMASK			0xFF00

struct s5p_lg4573_panel_data {
	const u16 *seq_settings_type0;
	const u16 *seq_settings_type1;
	const u16 *seq_settings_type2;
	const u16 *seq_settings_type3;
	const u16 *seq_standby_on;
	const u16 *seq_standby_off;
	
	int (*get_lcdtype)(void);

};

#endif

