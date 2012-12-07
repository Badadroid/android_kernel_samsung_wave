/*
 * arch/arm/mach-s5pv210/wave.h
 */

#ifndef __WAVE_H__
#define __WAVE_H__

struct uart_port;

void wave_bt_uart_wake_peer(struct uart_port *port);
extern void s3c_setup_uart_cfg_gpio(unsigned char port);

extern struct s5p_tl2796_panel_data wave_tl2796_panel_data;
extern struct s5p_lg4573_panel_data wave_lg4573_panel_data;
extern struct s5p_tft_panel_data wave_tft_panel_data;
extern struct s5p_lcd_panel_data wave_lcd_panel_data;

#endif

