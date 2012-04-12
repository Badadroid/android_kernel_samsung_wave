/*
 * This program is free software; you can redistribute it and/or modify it
 * under the terms of the GNU General Public License as published by the
 * Free Software Foundation; either version 2 of the License, or (at your
 * option) any later version.
 *
 * This program is distributed in the hope that it will be useful, but
 * WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License along
 * with this program; if not, write to the Free Software Foundation, Inc.,
 * 59 Temple Place - Suite 330, Boston, MA  02111-1307, USA.
 */
#include <linux/kernel.h>
#include <linux/types.h>
#include <linux/tl2796.h>
#include <linux/nt35580.h>
#include <mach/gpio.h>
#include <mach/gpio-wave.h>
#include <linux/delay.h>

#ifdef CONFIG_FB_S3C_LG4573
#define SLEEPMSEC		0x1000
#define ENDDEF			0x2000

static const u16 s6e63m0_SEQ_DISPLAY_ON[] = {
	0x029,
	ENDDEF, 0x0000
};

static const u16 s6e63m0_SEQ_DISPLAY_OFF[] = {
	0x028,
	ENDDEF, 0x0000
};

static const u16 s6e63m0_SEQ_STANDBY_ON[] = {
	0x010,	/* Stand-by On Command */
	SLEEPMSEC, 120,
	ENDDEF, 0x0000
};

static const u16 s6e63m0_SEQ_STANDBY_OFF[] = {
	0x011,	/* Stand-by Off Command */
	SLEEPMSEC, 120,
	ENDDEF, 0x0000
};

static const u16 s6e63m0_SEQ_DISPLAY_SETTING[] = {
	SLEEPMSEC, 10,
	0x0F8,	/* Panel Condition Set Command*/
	0x101,	/* DOCT */
	0x127,	/* CLWEA */
	0x127,	/* CLWEB*/
	0x107,	/* CLTE */
	0x107,	/* SHE */
	0x154,	/* FLTE */
	0x19F,	/* FLWE */
	0x163,	/* SCTE */
	0x186,	/* SCWE */
	0x11A,	/* INTE */
	0x133,	/* INWE */
	0x10D,	/* EMPS */
	0x100,	/* E_INTE */
	0x100,	/* E_INWE */

	0x0F2,	/* Display Condition Set Command*/
	0x102,	/* Number of Line */
	0x103,	/* VBP */
	0x11C,	/* VFP */
	0x110,	/* HBP */
	0x110,	/* HFP */

	0x0F7,	/* Command */
	0x100,	/* GTCON */ //0x103 by default
	0x100,	/* Display Mode */
	0x100,	/* Vsync/Hsync, DOCCLK, RGB mode */
	ENDDEF, 0x0000
};

static const u16 s6e63m0_SEQ_ETC_SETTING[] = {
	/* ETC Condition Set Command  */
	0x0F6,
	0x100,	0x18E,
	0x107,

	0x0B3,
	0x16C,

	0x0B5,
	0x12C,	0x112,
	0x10C,	0x10A,
	0x110,	0x10E,
	0x117,	0x113,
	0x11F,	0x11A,
	0x12A,	0x124,
	0x11F,	0x11B,
	0x11A,	0x117,
	0x12B,	0x126,
	0x122,	0x120,
	0x13A,	0x134,
	0x130,	0x12C,
	0x129,	0x126,
	0x125,	0x123,
	0x121,	0x120,
	0x11E,	0x11E,

	0x0B6,
	0x100,	0x100,
	0x111,	0x122,
	0x133,	0x144,
	0x144,	0x144,
	0x155,	0x155,
	0x166,	0x166,
	0x166,	0x166,
	0x166,	0x166,

	0x0B7,
	0x12C,	0x112,
	0x10C,	0x10A,
	0x110,	0x10E,
	0x117,	0x113,
	0x11F,	0x11A,
	0x12A,	0x124,
	0x11F,	0x11B,
	0x11A,	0x117,
	0x12B,	0x126,
	0x122,	0x120,
	0x13A,	0x134,
	0x130,	0x12C,
	0x129,	0x126,
	0x125,	0x123,
	0x121,	0x120,
	0x11E,	0x11E,

	0x0B8,
	0x100,	0x100,
	0x111,	0x122,
	0x133,	0x144,
	0x144,	0x144,
	0x155,	0x155,
	0x166,	0x166,
	0x166,	0x166,
	0x166,	0x166,

	0x0B9,
	0x12C,	0x112,
	0x10C,	0x10A,
	0x110,	0x10E,
	0x117,	0x113,
	0x11F,	0x11A,
	0x12A,	0x124,
	0x11F,	0x11B,
	0x11A,	0x117,
	0x12B,	0x126,
	0x122,	0x120,
	0x13A,	0x134,
	0x130,	0x12C,
	0x129,	0x126,
	0x125,	0x123,
	0x121,	0x120,
	0x11E,	0x11E,

	0x0BA,
	0x100,	0x100,
	0x111,	0x122,
	0x133,	0x144,
	0x144,	0x144,
	0x155,	0x155,
	0x166,	0x166,
	0x166,	0x166,
	0x166,	0x166,

	0x011,
	SLEEPMSEC, 120,
	0x029,
	ENDDEF, 0x0000
};

static const u16 s6e63m0_22gamma_300cd[] = {
	0x0FA,
	0x102,

	0x118,
	0x108,
	0x124,
	0x170,
	0x16E,
	0x14E,
	0x1BC,
	0x1C0,
	0x1AF,
	0x1B3,
	0x1B8,
	0x1A5,
	0x1C5,
	0x1C7,
	0x1BB,
	0x100,
	0x1B9,
	0x100,
	0x1B8,
	0x100,
	0x1FC,
                                                          
	ENDDEF, 0x0000                                
};                                              
                                                
static const u16 s6e63m0_22gamma_290cd[] = {
	0x0FA,
	0x102,

	0x118,
	0x108,
	0x124,
	0x171,
	0x170,
	0x150,
	0x1BD,
	0x1C1,
	0x1B0,
	0x1B2,
	0x1B8,
	0x1A4,
	0x1C6,
	0x1C7,
	0x1BB,
	0x100,
	0x1B6,
	0x100,
	0x1B6,
	0x100,
	0x1FA,
                                                                                        
	ENDDEF, 0x0000                                
};                                              
                                                
static const u16 s6e63m0_22gamma_280cd[] = {
	0x0FA,
	0x102,

	0x118,
	0x108,
	0x124,
	0x16E,
	0x16C,
	0x14D,
	0x1BE,
	0x1C3,
	0x1B1,
	0x1B3,
	0x1B8,
	0x1A5,
	0x1C6,
	0x1C8,
	0x1BB,
	0x100,
	0x1B4,
	0x100,
	0x1B3,
	0x100,
	0x1F7,
                                                
	ENDDEF, 0x0000                                
};                                              
                                                
static const u16 s6e63m0_22gamma_270cd[] = {
	0x0FA,
	0x102,

	0x118,
	0x108,
	0x124,
	0x171,
	0x16C,
	0x150,
	0x1BD,
	0x1C3,
	0x1B0,
	0x1B4,
	0x1B8,
	0x1A6,
	0x1C6,
	0x1C9,
	0x1BB,
	0x100,
	0x1B2,
	0x100,
	0x1B1,
	0x100,
	0x1F4,                                       
                                                
	ENDDEF, 0x0000                                
};                                              
                                                
static const u16 s6e63m0_22gamma_260cd[] = {
	0x0FA,
	0x102,

	0x118,
	0x108,
	0x124,
	0x174,
	0x16E,
	0x154,
	0x1BD,
	0x1C2,
	0x1B0,
	0x1B5,
	0x1BA,
	0x1A7,
	0x1C5,
	0x1C9,
	0x1BA,
	0x100,
	0x1B0,
	0x100,
	0x1AE,
	0x100,
	0x1F1,                                       
                                                
	ENDDEF, 0x0000                                
};                                              
                                                
static const u16 s6e63m0_22gamma_250cd[] = {
	0x0FA,
	0x102,

	0x118,
	0x108,
	0x124,
	0x174,
	0x16D,
	0x154,
	0x1BF,
	0x1C3,
	0x1B2,
	0x1B4,
	0x1BA,
	0x1A7,
	0x1C6,
	0x1CA,
	0x1BA,
	0x100,
	0x1AD,
	0x100,
	0x1AB,
	0x100,
	0x1ED,                                                                                      
                                                
	ENDDEF, 0x0000                                
};                                              
                                                
static const u16 s6e63m0_22gamma_240cd[] = {
	0x0FA,
	0x102,

	0x118,
	0x108,
	0x124,
	0x176,
	0x16F,
	0x156,
	0x1C0,
	0x1C3,
	0x1B2,
	0x1B5,
	0x1BA,
	0x1A8,
	0x1C6,
	0x1CB,
	0x1BB,
	0x100,
	0x1AA,
	0x100,
	0x1A8,
	0x100,
	0x1E9,
                                               
	ENDDEF, 0x0000                                
};                                              
                                                
static const u16 s6e63m0_22gamma_230cd[] = {
	0x0FA,
	0x102,

	0x118,
	0x108,
	0x124,
	0x175,
	0x16F,
	0x156,
	0x1BF,
	0x1C3,
	0x1B2,
	0x1B6,
	0x1BB,
	0x1A8,
	0x1C7,
	0x1CB,
	0x1BC,
	0x100,
	0x1A8,
	0x100,
	0x1A6,
	0x100,
	0x1E6,
                                             
	ENDDEF, 0x0000                                
};                                              
                                                
static const u16 s6e63m0_22gamma_220cd[] = {
	0x0FA,
	0x102,

	0x118,
	0x108,
	0x124,
	0x178,
	0x16F,
	0x158,
	0x1BF,
	0x1C4,
	0x1B3,
	0x1B5,
	0x1BB,
	0x1A9,
	0x1C8,
	0x1CC,
	0x1BC,
	0x100,
	0x1A6,
	0x100,
	0x1A3,
	0x100,
	0x1E2,
                                                                                       
	ENDDEF, 0x0000                                
};                                              
                                                
static const u16 s6e63m0_22gamma_210cd[] = {
	0x0FA,
	0x102,                                        

	0x118,
	0x108,
	0x124,
	0x179,
	0x16D,
	0x157,
	0x1C0,
	0x1C4,
	0x1B4,
	0x1B7,
	0x1BD,
	0x1AA,
	0x1C8,
	0x1CC,
	0x1BD,
	0x100,
	0x1A2,
	0x100,
	0x1A0,
	0x100,
	0x1DD,
                                                                                     
	ENDDEF, 0x0000                                
};                                              
                                                
static const u16 s6e63m0_22gamma_200cd[] = {
	0x0FA,
	0x102,

	0x118,
	0x108,
	0x124,
	0x179,
	0x16D,
	0x158,
	0x1C1,
	0x1C4,
	0x1B4,
	0x1B6,
	0x1BD,
	0x1AA,
	0x1CA,
	0x1CD,
	0x1BE,
	0x100,
	0x19F,
	0x100,
	0x19D,
	0x100,
	0x1D9,
                                               
	ENDDEF, 0x0000                                
};                                              
                                                
static const u16 s6e63m0_22gamma_190cd[] = {
	0x0FA,
	0x102,

	0x118,
	0x108,
	0x124,
	0x17A,
	0x16D,
	0x159,
	0x1C1,
	0x1C5,
	0x1B4,
	0x1B8,
	0x1BD,
	0x1AC,
	0x1C9,
	0x1CE,
	0x1BE,
	0x100,
	0x19D,
	0x100,
	0x19A,
	0x100,
	0x1D5,
                                               
	ENDDEF, 0x0000                                
};                                              
                                                
static const u16 s6e63m0_22gamma_180cd[] = {
	0x0FA,
	0x102,

	0x118,	
	0x108,
	0x124,
	0x17B,
	0x16D,
	0x15B,
	0x1C0,
	0x1C5,
	0x1B3,
	0x1BA,
	0x1BE,
	0x1AD,
	0x1CA,
	0x1CE,
	0x1BF,
	0x100,
	0x199,
	0x100,
	0x197,
	0x100,
	0x1D0,
                                                
	ENDDEF, 0x0000                                
};                                              
                                                
static const u16 s6e63m0_22gamma_170cd[] = {
	0x0FA,
	0x102,

	0x118,
	0x108,
	0x124,
	0x17C,
	0x16D,
	0x15C,
	0x1C0,
	0x1C6,
	0x1B4,
	0x1BB,
	0x1BE,
	0x1AD,
	0x1CA,
	0x1CF,
	0x1C0,
	0x100,
	0x196,
	0x100,
	0x194,
	0x100,
	0x1CC,
                                                
	ENDDEF, 0x0000                                
};                                              
                                                
static const u16 s6e63m0_22gamma_160cd[] = {
	0x0FA,
	0x102,

	0x118,
	0x108,
	0x124,
	0x17F,
	0x16E,
	0x15F,
	0x1C0,
	0x1C6,
	0x1B5,
	0x1BA,
	0x1BF,
	0x1AD,
	0x1CB,
	0x1CF,
	0x1C0,
	0x100,
	0x194,
	0x100,
	0x191,
	0x100,
	0x1C8,                                        
                                                
	ENDDEF, 0x0000                                
};                                              
                                                
static const u16 s6e63m0_22gamma_150cd[] = {
	0x0FA,
	0x102,


	0x118,
	0x108,
	0x124,
	0x180,
	0x16E,
	0x15F,
	0x1C1,
	0x1C6,
	0x1B6,
	0x1BC,
	0x1C0,
	0x1AE,
	0x1CC,
	0x1D0,
	0x1C2,
	0x100,
	0x18F,
	0x100,
	0x18D,
	0x100,
	0x1C2,                                     
                                                
	ENDDEF, 0x0000                                
};                                              
                                                
static const u16 s6e63m0_22gamma_140cd[] = {
	0x0FA,
	0x102,

	0x118,
	0x108,
	0x124,
	0x180,
	0x16C,
	0x15F,
	0x1C1,
	0x1C6,
	0x1B7,
	0x1BC,
	0x1C1,
	0x1AE,
	0x1CD,
	0x1D0,
	0x1C2,
	0x100,
	0x18C,
	0x100,
	0x18A,
	0x100,
	0x1BE,                                     
                                                
	ENDDEF, 0x0000                                
};                                              
                                                
static const u16 s6e63m0_22gamma_130cd[] = {
	0x0FA,
	0x102,

	0x118,
	0x108,
	0x124,
	0x18C,
	0x16C,
	0x160,
	0x1C3,
	0x1C7,
	0x1B9,
	0x1BC,
	0x1C1,
	0x1AF,
	0x1CE,
	0x1D2,
	0x1C3,
	0x100,
	0x188,
	0x100,
	0x186,
	0x100,
	0x1B8,
                                             
	ENDDEF, 0x0000                                
};                                              
                                                
static const u16 s6e63m0_22gamma_120cd[] = {
	0x0FA,
	0x102,

	0x118,
	0x108,
	0x124,
	0x182,
	0x16B,
	0x15E,
	0x1C4,
	0x1C8,
	0x1B9,
	0x1BD,
	0x1C2,
	0x1B1,
	0x1CE,
	0x1D2,
	0x1C4,
	0x100,
	0x185,
	0x100,
	0x182,
	0x100,
	0x1B3,                                    
                                                
	ENDDEF, 0x0000                                
};                                              
                                                
static const u16 s6e63m0_22gamma_110cd[] = {
	0x0FA,                                                                                      
	0x102,

	0x118,
	0x108,
	0x124,
	0x186,
	0x16A,
	0x160,
	0x1C5,
	0x1C7,
	0x1BA,
	0x1BD,
	0x1C3,
	0x1B2,
	0x1D0,
	0x1D4,
	0x1C5,
	0x100,
	0x180,
	0x100,
	0x17E,
	0x100,
	0x1AD,                                       
                                                
	ENDDEF, 0x0000                                
};                                              
                                                
                                                
static const u16 s6e63m0_22gamma_100cd[] = {                                 
	0x0FA,                                        	                                              
	0x102,
	
	0x118,
	0x108,
	0x124,
	0x186,
	0x169,
	0x160,
	0x1C6,
	0x1C8,
	0x1BA,
	0x1BF,
	0x1C4,
	0x1B4,
	0x1D0,
	0x1D4,
	0x1C6,
	0x100,
	0x17C,
	0x100,
	0x17A,
	0x100,
	0x1A7,
                                           
	ENDDEF, 0x0000                                
};                                              
                                                
static const u16 s6e63m0_22gamma_90cd[] = {                                 
	0x0FA,                                                                                        
	0x102,  
	
	0x118,
	0x108,
	0x124,
	0x1B9,
	0x169,
	0x164,
	0x1C7,
	0x1C8,
	0x1BB,
	0x1C0,
	0x1C5,
	0x1B4,
	0x1D2,
	0x1D5,
	0x1C9,
	0x100,
	0x177,
	0x100,
	0x176,
	0x100,
	0x1A0,
                                             
	ENDDEF, 0x0000                                
};                                              
                                                
static const u16 s6e63m0_22gamma_80cd[] = {                                   
	0x0FA,                                                                                       
	0x102,                                        
	0x118,
	0x108,
	0x124,
	0x189,
	0x168,
	0x165,
	0x1C9,
	0x1C9,
	0x1BC,
	0x1C1,
	0x1C5,
	0x1B6,
	0x1D2,
	0x1D5,
	0x1C9,
	0x100,
	0x173,
	0x100,
	0x172,
	0x100,
	0x19A,
                                                                                     
	ENDDEF, 0x0000                                
};                                              
                                                
static const u16 s6e63m0_22gamma_70cd[] = {                                 
	0x0FA,                                                                                     
	0x102,                                        
	0x118,
	0x108,
	0x124,
	0x18E,
	0x162,
	0x16B,
	0x1C7,
	0x1C9,
	0x1BB,
	0x1C3,
	0x1C7,
	0x1B7,
	0x1D3,
	0x1D7,
	0x1CA,
	0x100,
	0x16E,
	0x100,
	0x16C,
	0x100,
	0x194,
                                     
	ENDDEF, 0x0000                                
};                                              
                                                
static const u16 s6e63m0_22gamma_60cd[] = {                                  
	0x0FA,                                                                                        
	0x102,                                        
	0x118,
	0x108,
	0x124,
	0x191,
	0x15E,
	0x16E,
	0x1C9,
	0x1C9,
	0x1BD,
	0x1C4,
	0x1C9,
	0x1B8,
	0x1D3,
	0x1D7,
	0x1CA,
	0x100,
	0x169,
	0x100,
	0x167,
	0x100,
	0x18D,
                                              
	ENDDEF, 0x0000                                
};                                              
                                                
static const u16 s6e63m0_22gamma_50cd[] = {                                  
	0x0FA,                                        
	0x102,
	
	0x118,                                        
	0x108,
	0x124,
	0x196,
	0x158,
	0x172,
	0x1CB,
	0x1CA,
	0x1BF,
	0x1C6,
	0x1C9,
	0x1BA,
	0x1D6,
	0x1D9,
	0x1CD,
	0x100,
	0x161,
	0x100,
	0x161,
	0x100,
	0x183,
                                              
	ENDDEF, 0x0000                                
}; 

static const u16 s6e63m0_22gamma_40cd[] = { 
	0x0FA,	
	0x102,
	
	0x118,
	0x108,
	0x124,
	0x197,
	0x158,
	0x171,
	0x1CC,
	0x1CB,
	0x1C0,
	0x1C5,
	0x1C9,
	0x1BA,
	0x1D9,
	0x1DC,
	0x1D1,
	0x100,
	0x15B,
	0x100,
	0x15A,
	0x100,
	0x17A,

	ENDDEF, 0x0000                               
}; 

static const u16 s6e63m0_22gamma_30cd[] = { 
	0x0FA,	
	0x102,
	
	0x118,
	0x108,
	0x124,
	0x1A1,
	0x151,
	0x17B,
	0x1CE,
	0x1CB,
	0x1C2,
	0x1C7,
	0x1CB,
	0x1BC,
	0x1DA,
	0x1DD,
	0x1D3,
	0x100,
	0x153,
	0x100,
	0x152,
	0x100,
	0x16F, 	                                      

	ENDDEF, 0x0000                              
}; 


#if !defined(CONFIG_ARIES_NTT)
static const u16 *p22Gamma_set[] = {
	s6e63m0_22gamma_30cd,
	s6e63m0_22gamma_40cd,                         
	s6e63m0_22gamma_70cd,                         
	s6e63m0_22gamma_90cd,                         
	s6e63m0_22gamma_100cd,                     
	s6e63m0_22gamma_110cd,                    
	s6e63m0_22gamma_120cd,                        
	s6e63m0_22gamma_130cd,                        
	s6e63m0_22gamma_140cd,	                      
	s6e63m0_22gamma_150cd,                    
	s6e63m0_22gamma_160cd,                
	s6e63m0_22gamma_170cd,                        
	s6e63m0_22gamma_180cd,                        
	s6e63m0_22gamma_190cd,	                      
	s6e63m0_22gamma_200cd,                    
	s6e63m0_22gamma_210cd,                
	s6e63m0_22gamma_220cd,                        
	s6e63m0_22gamma_230cd,                        
	s6e63m0_22gamma_240cd,                        
	s6e63m0_22gamma_250cd,                   
	s6e63m0_22gamma_260cd,                       
	s6e63m0_22gamma_270cd,                        
	s6e63m0_22gamma_280cd,                        
	s6e63m0_22gamma_290cd,                        
	s6e63m0_22gamma_300cd,         
};                                             
#else                                
static const u16 *p22Gamma_set[] = {        
	s6e63m0_22gamma_30cd,
	s6e63m0_22gamma_40cd,  
	s6e63m0_22gamma_50cd,
	s6e63m0_22gamma_60cd,
	s6e63m0_22gamma_70cd,	
	s6e63m0_22gamma_80cd,
	s6e63m0_22gamma_90cd,
	s6e63m0_22gamma_100cd,
	s6e63m0_22gamma_110cd,	
	s6e63m0_22gamma_120cd,	
	s6e63m0_22gamma_130cd,
	s6e63m0_22gamma_140cd,
	s6e63m0_22gamma_150cd,
	s6e63m0_22gamma_160cd,	
	s6e63m0_22gamma_170cd,	
	s6e63m0_22gamma_180cd,
	s6e63m0_22gamma_190cd,
	s6e63m0_22gamma_200cd,
	s6e63m0_22gamma_210cd,	
	s6e63m0_22gamma_220cd,	
	s6e63m0_22gamma_230cd,
	s6e63m0_22gamma_240cd,
	s6e63m0_22gamma_260cd,
	s6e63m0_22gamma_280cd,
	s6e63m0_22gamma_300cd,     
};        
#endif
                                                
static const u16 s6e63m0_19gamma_300cd[] = {                                 
	0x0FA,                                        	                                              
	0x102,                                        
                                                
	0x118,
	0x108,
	0x124,
	0x179,
	0x17A,
	0x15B,
	0x1C1,
	0x1C5,
	0x1B5,
	0x1B8,
	0x1BD,
	0x1AB,
	0x1CB,
	0x1CE,
	0x1C1,
	0x100,
	0x1B8,
	0x100,
	0x1B7,
	0x100,
	0x1FC,
                                               
	ENDDEF, 0x0000                                
};                                              
                                                
static const u16 s6e63m0_19gamma_290cd[] = {                              
	0x0FA,                                        	                                              
	0x102,                                        
                                                
	0x118,
	0x108,
	0x124,
	0x178,
	0x17A,
	0x15B,
	0x1C2,
	0x1C7,
	0x1B6,
	0x1BA,
	0x1BE,
	0x1AC,
	0x1CB,
	0x1CE,
	0x1C2,
	0x100,
	0x1B8,
	0x100,
	0x1B6,
	0x100,
	0x1FB,
                                               
	ENDDEF, 0x0000                                
};                                              
                                                
static const u16 s6e63m0_19gamma_280cd[] = {                                 
	0x0FA,                                        	                                              
	0x102,  
	
	0x118,
	0x108,
	0x124,
	0x17B,
	0x17D,
	0x15F,
	0x1C1,
	0x1C7,
	0x1B5,
	0x1BA,
	0x1BE,
	0x1AD,
	0x1CC,
	0x1CE,
	0x1C2,
	0x100,
	0x1B5,
	0x100,
	0x1B4,
	0x100,
	0x1F8,
                                               
	ENDDEF, 0x0000                                
};                                              
                                                
static const u16 s6e63m0_19gamma_270cd[] = {                               
	0x0FA,                                        	                                              
	0x102,                                        
                                                
	0x118,
	0x108,
	0x124,
	0x17C,
	0x17E,
	0x161,
	0x1C1,
	0x1C6,
	0x1B5,
	0x1BA,
	0x1BF,
	0x1AD,
	0x1CC,
	0x1CF,
	0x1C2,
	0x100,
	0x1B3,
	0x100,
	0x1B1,
	0x100,
	0x1F5,
                                                
	ENDDEF, 0x0000                                
};                                              
                                                
static const u16 s6e63m0_19gamma_260cd[] = {
	0x0FA,
	0x102,

	0x118,
	0x108,
	0x124,
	0x179,
	0x17C,
	0x15E,
	0x1C3,
	0x1C7,
	0x1B6,
	0x1BA,
	0x1BF,
	0x1AE,
	0x1CC,
	0x1D0,
	0x1C2,
	0x100,
	0x1B1,
	0x100,
	0x1AE,
	0x100,
	0x1F2,
                                                
	ENDDEF, 0x0000                                
};                                              
                                                
static const u16 s6e63m0_19gamma_250cd[] = {                                   
	0x0FA,                                        	                                              
	0x102,                                        
                                                
	0x118,
	0x108,
	0x124,
	0x178,
	0x17C,
	0x15E,
	0x1C3,
	0x1C8,
	0x1B6,
	0x1BC,
	0x1C0,
	0x1AF,
	0x1CC,
	0x1CF,
	0x1C2,
	0x100,
	0x1AE,
	0x100,
	0x1AC,
	0x100,
	0x1EE,
                                                 
	ENDDEF, 0x0000                                
};                                              
                                                
static const u16 s6e63m0_19gamma_240cd[] = {                             
	0x0FA,                                        	                                              
	0x102,                                        
                                                
	0x118,
	0x108,
	0x124,
	0x17A,
	0x17F,
	0x161,
	0x1C2,
	0x1C7,
	0x1B6,
	0x1BC,
	0x1C1,
	0x1AF,
	0x1CE,
	0x1D0,
	0x1C3,
	0x100,
	0x1AB,
	0x100,
	0x1A9,
	0x100,
	0x1EA,
                                                
	ENDDEF, 0x0000                                
};                                              
                                                
static const u16 s6e63m0_19gamma_230cd[] = {                                 
	0x0FA,                                        	                                              
	0x102,                                        
                                                
	0x118,
	0x108,
	0x124,
	0x17A,
	0x17E,
	0x161,
	0x1C4,
	0x1C8,
	0x1B8,
	0x1BB,
	0x1C1,
	0x1AF,
	0x1CE,
	0x1D1,
	0x1C3,
	0x100,
	0x1A8,
	0x100,
	0x1A6,
	0x100,
	0x1E6,
                                                
	ENDDEF, 0x0000                                
};                                              
                                                
static const u16 s6e63m0_19gamma_220cd[] = {                                  
	0x0FA,                                        	                                              
	0x102,                                        
                                                
	0x118,
	0x108,
	0x124,
	0x17C,
	0x17F,
	0x162,
	0x1C5,
	0x1C8,
	0x1B9,
	0x1BC,
	0x1C1,
	0x1B0,
	0x1CE,
	0x1D2,
	0x1C3,
	0x100,
	0x1A5,
	0x100,
	0x1A3,
	0x100,
	0x1E2,
                                                
	ENDDEF, 0x0000                                
};                                              
                                                
static const u16 s6e63m0_19gamma_210cd[] = {                                
	0x0FA,                                        	                                              
	0x102,                                        
                                                
	0x118,
	0x108,
	0x124,
	0x17B,
	0x17F,
	0x161,
	0x1C6,
	0x1C8,
	0x1B9,
	0x1BE,
	0x1C2,
	0x1B2,
	0x1CE,
	0x1D3,
	0x1C4,
	0x100,
	0x1A2,
	0x100,
	0x1A0,
	0x100,
	0x1DD,
                                                
	ENDDEF, 0x0000                                
};                                              
                                                
static const u16 s6e63m0_19gamma_200cd[] = {                                   
	0x0FA,                                        	                                              
	0x102,                                        
                                                
	0x118,
	0x108,
	0x124,
	0x17B,
	0x17E,
	0x161,
	0x1C5,
	0x1C9,
	0x1B9,
	0x1BF,
	0x1C3,
	0x1B2,
	0x1CF,
	0x1D3,
	0x1C5,
	0x100,
	0x19F,
	0x100,
	0x19D,
	0x100,
	0x1D9,
                                                
	ENDDEF, 0x0000                                
};                                              
                                                
static const u16 s6e63m0_19gamma_190cd[] = {                                  
	0x0FA,                                        	                                              
	0x102,                                        
                                                
	0x118,
	0x108,
	0x124,
	0x17D,
	0x17F,
	0x163,
	0x1C5,
	0x1C9,
	0x1BA,
	0x1BF,
	0x1C4,
	0x1B2,
	0x1D0,
	0x1D3,
	0x1C6,
	0x100,
	0x19C,
	0x100,
	0x19A,
	0x100,
	0x1D5,
                                                
	ENDDEF, 0x0000                                
};                                              
                                                
static const u16 s6e63m0_19gamma_180cd[] = {                                
	0x0FA,                                        	                                              
	0x102,                                        
                                                
	0x118,
	0x108,
	0x124,
	0x180,
	0x182,
	0x165,
	0x1C6,
	0x1C9,
	0x1BB,
	0x1BF,
	0x1C4,
	0x1B3,
	0x1D0,
	0x1D4,
	0x1C6,
	0x100,
	0x199,
	0x100,
	0x197,
	0x100,
	0x1D0,
                                                
	ENDDEF, 0x0000                                
};                                              
                                                
static const u16 s6e63m0_19gamma_170cd[] = {                                 
	0x0FA,                                        	                                              
	0x102,                                        
                                                
	0x118,
	0x108,
	0x124,
	0x17D,
	0x17F,
	0x161,
	0x1C7,
	0x1CA,
	0x1BB,
	0x1C0,
	0x1C5,
	0x1B5,
	0x1D1,
	0x1D4,
	0x1C8,
	0x100,
	0x196,
	0x100,
	0x194,
	0x100,
	0x1CB,
                                                
	ENDDEF, 0x0000                                
};                                              
                                                
static const u16 s6e63m0_19gamma_160cd[] = {                                 
	0x0FA,                                        
	0x102,                                        
                                                
	0x118,
	0x108,
	0x124,
	0x17C,
	0x17E,
	0x161,
	0x1C7,
	0x1CB,
	0x1BB,
	0x1C1,
	0x1C5,
	0x1B5,
	0x1D1,
	0x1D5,
	0x1C8,
	0x100,
	0x193,
	0x100,
	0x190,
	0x100,
	0x1C7,
                                                
	ENDDEF, 0x0000                                
};                                              
                                                
static const u16 s6e63m0_19gamma_150cd[] = {                                
	0x0FA,                                        	                                              
	0x102,                                        
                                                
	0x118,
	0x108,
	0x124,
	0x17E,
	0x17F,
	0x163,
	0x1C8,
	0x1CB,
	0x1BC,
	0x1C1,
	0x1C6,
	0x1B6,
	0x1D2,
	0x1D6,
	0x1C8,
	0x100,
	0x18F,
	0x100,
	0x18D,
	0x100,
	0x1C2,
                                                
	ENDDEF, 0x0000                                
};                                              
                                                
static const u16 s6e63m0_19gamma_140cd[] = {                                   
	0x0FA,                                        	                                              
	0x102,                                        
                                                
	0x118,
	0x108,
	0x124,
	0x17E,
	0x17F,
	0x165,
	0x1C8,
	0x1CC,
	0x1BC,
	0x1C2,
	0x1C6,
	0x1B6,
	0x1D3,
	0x1D6,
	0x1C9,
	0x100,
	0x18C,
	0x100,
	0x18A,
	0x100,
	0x1BE,
                                                
	ENDDEF, 0x0000                                
};                                              
                                                
static const u16 s6e63m0_19gamma_130cd[] = {                               
	0x0FA,                                                                                      
	0x102,                                        
                                                
	0x118,
	0x108,
	0x124,
	0x182,
	0x17F,
	0x169,
	0x1C8,
	0x1CC,
	0x1BD,
	0x1C2,
	0x1C7,
	0x1B6,
	0x1D4,
	0x1D7,
	0x1CB,
	0x100,
	0x188,
	0x100,
	0x186,
	0x100,
	0x1B8,
                                                
	ENDDEF, 0x0000                                
};                                              
                                                
static const u16 s6e63m0_19gamma_120cd[] = {                             
	0x0FA,                                        	                                              
	0x102,                                        
                                                
	0x118,
	0x108,
	0x124,
	0x183,
	0x17F,
	0x169,
	0x1C9,
	0x1CC,
	0x1BE,
	0x1C3,
	0x1C8,
	0x1B8,
	0x1D4,
	0x1D7,
	0x1CB,
	0x100,
	0x185,
	0x100,
	0x183,
	0x100,
	0x1B3,
                                               
	ENDDEF, 0x0000                                
};                                              
                                                
static const u16 s6e63m0_19gamma_110cd[] = {                         
	0x0FA,                                        	                                              
	0x102,                                        
                                                
	0x118,
	0x108,
	0x124,
	0x185,
	0x181,
	0x16A,
	0x1CB,
	0x1CD,
	0x1BF,
	0x1C4,
	0x1C8,
	0x1B9,
	0x1D5,
	0x1D9,
	0x1CC,
	0x100,
	0x180,
	0x100,
	0x17E,
	0x100,
	0x1AD,
                                                
	ENDDEF, 0x0000                                
};                                              
                                                
static const u16 s6e63m0_19gamma_100cd[] = {                               
	0x0FA,                                        	                                              
	0x102,                                        
                                                
	0x118,
	0x108,
	0x124,
	0x186,
	0x181,
	0x16B,
	0x1CA,
	0x1CD,
	0x1BE,
	0x1C6,
	0x1C9,
	0x1BB,
	0x1D5,
	0x1DA,
	0x1CD,
	0x100,
	0x17C,
	0x100,
	0x17A,
	0x100,
	0x1A7,
                                                
	ENDDEF, 0x0000                                
};                                              
                                                
static const u16 s6e63m0_19gamma_90cd[] = {                                  
	0x0FA,                                        	                                              
	0x102,                                        
                                                
	0x118,
	0x108,
	0x124,
	0x188,
	0x181,
	0x16D,
	0x1CB,
	0x1CE,
	0x1C0,
	0x1C6,
	0x1CA,
	0x1BB,
	0x1D6,
	0x1DA,
	0x1CE,
	0x100,
	0x178,
	0x100,
	0x176,
	0x100,
	0x1A1,
                                                 
	ENDDEF, 0x0000                                
};                                              
                                                
static const u16 s6e63m0_19gamma_80cd[] = {                         
	0x0FA,                                        	                                              
	0x102,                                        
                                                
	0x118,
	0x108,
	0x124,
	0x18E,
	0x17F,
	0x172,
	0x1CB,
	0x1CF,
	0x1C1,
	0x1C6,
	0x1CB,
	0x1BB,
	0x1D8,
	0x1DB,
	0x1CF,
	0x100,
	0x173,
	0x100,
	0x171,
	0x100,
	0x19B,
                                               
	ENDDEF, 0x0000                                
};                                              
                                                
static const u16 s6e63m0_19gamma_70cd[] = {                              
	0x0FA,                                        	                                              
	0x102,                                        
                                                
	0x118,
	0x108,
	0x124,
	0x192,
	0x17F,
	0x175,
	0x1CC,
	0x1CF,
	0x1C2,
	0x1C7,
	0x1CC,
	0x1BD,
	0x1D8,
	0x1DC,
	0x1CF,
	0x100,
	0x16E,
	0x100,
	0x16C,
	0x100,
	0x194,
                                                
	ENDDEF, 0x0000                                
};                                              
                                                
static const u16 s6e63m0_19gamma_60cd[] = {                                  
	0x0FA,                                        	                                              
	0x102,                                        
                                                
	0x118,
	0x108,
	0x124,
	0x193,
	0x17F,
	0x174,
	0x1CD,
	0x1CF,
	0x1C3,
	0x1CA,
	0x1CD,
	0x1C0,
	0x1D8,
	0x1DD,
	0x1D1,
	0x100,
	0x169,
	0x100,
	0x167,
	0x100,
	0x18C,
                                                
	ENDDEF, 0x0000                                
};                                              
                                                
static const u16 s6e63m0_19gamma_50cd[] = {                                
	0x0FA,                                                                                      
	0x102,                                        
                                                
	0x118,
	0x108,
	0x124,
	0x194,
	0x17C,
	0x173,
	0x1CF,
	0x1D0,
	0x1C5,
	0x1CA,
	0x1CE,
	0x1C0,
	0x1DB,
	0x1DF,
	0x1D4,
	0x100,
	0x162,
	0x100,
	0x160,
	0x100,
	0x183,
                                                
	ENDDEF, 0x0000                                
};                                              

static const u16 s6e63m0_19gamma_40cd[] = { 
	0x0FA,
	0x102,	
                                                
	0x118,
	0x108,
	0x124,
	0x193,
	0x177,
	0x172,
	0x1CF,
	0x1D0,
	0x1C5,
	0x1CB,
	0x1CF,
	0x1C1,
	0x1DD,
	0x1E0,
	0x1D6,
	0x100,
	0x15B,
	0x100,
	0x15A,
	0x100,
	0x17A,

	ENDDEF, 0x0000                              
}; 

static const u16 s6e63m0_19gamma_30cd[] = { 
	0x0FA,
	0x102,	
                                                
	0x118,
	0x108,
	0x124,
	0x19D,
	0x175,
	0x17C,
	0x1D0,
	0x1D0,
	0x1C6,
	0x1CD,
	0x1D1,
	0x1C3,
	0x1DE,
	0x1E1,
	0x1D8,
	0x100,
	0x153,
	0x100,
	0x152,
	0x100,
	0x16F,

	ENDDEF, 0x0000                               
}; 

#if !defined(CONFIG_ARIES_NTT)  
static const u16 *p19Gamma_set[] = {        
	s6e63m0_19gamma_30cd,                                   
	s6e63m0_19gamma_40cd,                         
	s6e63m0_19gamma_70cd,                         
	s6e63m0_19gamma_90cd,                         
	s6e63m0_19gamma_100cd,                    
	s6e63m0_19gamma_110cd,                        
	s6e63m0_19gamma_120cd,                        
	s6e63m0_19gamma_130cd,                        
	s6e63m0_19gamma_140cd,	                      
	s6e63m0_19gamma_150cd,            
	s6e63m0_19gamma_160cd,                        
	s6e63m0_19gamma_170cd,                        
	s6e63m0_19gamma_180cd,                        
	s6e63m0_19gamma_190cd,	                      
	s6e63m0_19gamma_200cd,               
	s6e63m0_19gamma_210cd,                        
	s6e63m0_19gamma_220cd,                        
	s6e63m0_19gamma_230cd,                        
	s6e63m0_19gamma_240cd,                        
	s6e63m0_19gamma_250cd,              
	s6e63m0_19gamma_260cd,                        
	s6e63m0_19gamma_270cd,                        
	s6e63m0_19gamma_280cd,                        
	s6e63m0_19gamma_290cd,                        
	s6e63m0_19gamma_300cd,                  
}; 
#else
static const u16 *p19Gamma_set[] = {        
	s6e63m0_19gamma_30cd,             
	s6e63m0_19gamma_40cd,  
	s6e63m0_19gamma_50cd,
	s6e63m0_19gamma_60cd,
	s6e63m0_19gamma_70cd,	
	s6e63m0_19gamma_80cd,
	s6e63m0_19gamma_90cd,
	s6e63m0_19gamma_100cd,
	s6e63m0_19gamma_110cd,	
	s6e63m0_19gamma_120cd,	
	s6e63m0_19gamma_130cd,
	s6e63m0_19gamma_140cd,
	s6e63m0_19gamma_150cd,
	s6e63m0_19gamma_160cd,	
	s6e63m0_19gamma_170cd,	
	s6e63m0_19gamma_180cd,
	s6e63m0_19gamma_190cd,
	s6e63m0_19gamma_200cd,
	s6e63m0_19gamma_210cd,	
	s6e63m0_19gamma_220cd,	
	s6e63m0_19gamma_230cd,
	s6e63m0_19gamma_240cd,
	s6e63m0_19gamma_260cd,
	s6e63m0_19gamma_280cd,
	s6e63m0_19gamma_300cd,
}; 
#endif

static const u16 gamma_updates[] = { 
	0x0FA,	
	0x103,
	SLEEPMSEC, 10,
	0x029,
	ENDDEF, 0x0000                              
}; 

static const u16 acl_cutoff_off[] = {
	/* ACL Off */
	0x0C0,
	0x100,

	ENDDEF, 0x0000 
};

/* ACL INIT : delta Y is all zero */
static const u16 acl_cutoff_init[] = {
	0x0F0,
	0x15A,
	0x15A,

	/* ACL parameter set */
	0x0C1,
	0x14D,	0x196,	0x11D,
	0x100,	0x100,	0x101,	0x1DF,
	0x100,	0x100,	0x103,	0x11F,
	0x100,  0x100,  0x100,  0x100,       	
	0x100,  0x100,  0x100,  0x100,       	
	0x100,  0x100,  0x100,  0x100,       	
	0x100,  0x100,  0x100,  0x100,	
	
	0x0C0,
	0x101,
	ENDDEF, 0x0000
};

static const u16 acl_cutoff_8p[] = {
	0x0F0,
	0x15A,
	0x15A,

	0x0C1,
	0x14D,	0x196,	0x11D,
	0x100,	0x100,	0x101,	0x1DF,
	0x100,	0x100,	0x103,	0x11F,
	0x100,  0x100,  0x100,  0x100,       	
	0x100,  0x101,  0x102,  0x103,       	
	0x104,  0x105,  0x106,  0x107,       	
	0x108,  0x109,  0x10A,  0x10B,

	0x0C0,
	0x101,
	ENDDEF, 0x0000
};

static const u16 acl_cutoff_14p[] = {
	0x0F0,
	0x15A,
	0x15A,

	0x0C1,
	0x14D,	0x196,	0x11D,
	0x100,	0x100,	0x101,	0x1DF,
	0x100,	0x100,	0x103,	0x11F,
	0x100,  0x100,  0x100,  0x100,       	
	0x100,  0x101,  0x103,  0x105,       	
	0x106,  0x108,  0x10A,  0x10C,       	
	0x10E,  0x10F,  0x111,  0x113,

	0x0C0,
	0x101,
	ENDDEF, 0x0000
};

static const u16 acl_cutoff_20p[] = {
	0x0F0,
	0x15A,
	0x15A,

	0x0C1,
	0x14D,	0x196,	0x11D,
	0x100,	0x100,	0x101,	0x1DF,
	0x100,	0x100,	0x103,	0x11F,
	0x100,  0x100,  0x100,  0x100,       	
	0x100,  0x101,  0x103,  0x108,       	
	0x10C,  0x10F,  0x112,  0x114,       	
	0x115,  0x117,  0x118,  0x119,

	0x0C0,
	0x101,
	ENDDEF, 0x0000
};

static const u16 acl_cutoff_24p[] = {
	0x0F0,
	0x15A,
	0x15A,

	0x0C1,
	0x14D,	0x196,	0x11D,
	0x100,	0x100,	0x101,	0x1DF,
	0x100,	0x100,	0x103,	0x11F,
	0x100,  0x100,  0x100,  0x100,       	
	0x100,  0x101,  0x104,  0x107,       	
	0x10A,  0x10D,  0x111,  0x114,       	
	0x117,  0x11A,  0x11D,  0x120,

	0x0C0,
	0x101,
	ENDDEF, 0x0000
};

static const u16 acl_cutoff_28p[] = {
	0x0F0,
	0x15A,
	0x15A,

	0x0C1,
	0x14D,	0x196,	0x11D,
	0x100,	0x100,	0x101,	0x1DF,
	0x100,	0x100,	0x103,	0x11F,
	0x100,  0x100,  0x100,  0x100,       	
	0x100,  0x101,  0x105,  0x108,       	
	0x10C,  0x10F,  0x113,  0x117,       	
	0x11A,  0x11E,  0x121,  0x125,

	0x0C0,
	0x101,
	ENDDEF, 0x0000
};


static const u16 acl_cutoff_32p[] = {
	0x0F0,
	0x15A,
	0x15A,

	0x0C1,
	0x14D,	0x196,	0x11D,
	0x100,	0x100,	0x101,	0x1DF,
	0x100,	0x100,	0x103,	0x11F,
	0x100,  0x100,  0x100,  0x100,       	
	0x100,  0x101,  0x105,  0x109,       	
	0x10D,  0x111,  0x116,  0x11A,       	
	0x11E,  0x122,  0x126,  0x12A,

	0x0C0,
	0x101,
	ENDDEF, 0x0000
};

static const u16 acl_cutoff_35p[] = {
	0x0F0,
	0x15A,
	0x15A,

	0x0C1,
	0x14D,	0x196,	0x11D,
	0x100,	0x100,	0x101,	0x1DF,
	0x100,	0x100,	0x103,	0x11F,
	0x100,  0x100,  0x100,  0x100,       	
	0x100,  0x101,  0x106,  0x10A,       	
	0x10F,  0x113,  0x118,  0x11C,       	
	0x121,  0x125,  0x12A,  0x12E,

	0x0C0,
	0x101,
	ENDDEF, 0x0000
};

static const u16 acl_cutoff_37p[] = {
	0x0F0,
	0x15A,
	0x15A,

	0x0C1,
	0x14D,	0x196,	0x11D,
	0x100,	0x100,	0x101,	0x1DF,
	0x100,	0x100,	0x103,	0x11F,
	0x100,  0x100,  0x100,  0x100,       	
	0x100,  0x101,  0x106,  0x10B,       	
	0x110,  0x115,  0x11A,  0x11E,       	
	0x123,  0x128,  0x12D,  0x132,

	0x0C0,
	0x101,
	ENDDEF, 0x0000
};


static const u16 acl_cutoff_40p[] = {
	0x0F0,
	0x15A,
	0x15A,

	0x0C1,
	0x14D,	0x196,	0x11D,
	0x100,	0x100,	0x101,	0x1DF,
	0x100,	0x100,	0x103,	0x11F,
	0x100,  0x100,  0x100,  0x100,       	
	0x100,  0x101,  0x106,  0x111,       	
	0x11A,  0x120,  0x125,  0x129,       	
	0x12D,  0x130,  0x133,  0x135,
	
	0x0C0,
	0x101,
	ENDDEF, 0x0000
};

static const u16 acl_cutoff_43p[] = {
	0x0F0,
	0x15A,
	0x15A,

	0x0C1,
	0x14D,	0x196,	0x11D,
	0x100,	0x100,	0x101,	0x1DF,
	0x100,	0x100,	0x103,	0x11F,
	0x100,  0x100,  0x100,  0x100,       	
	0x100,  0x101,  0x107,  0x112,       	
	0x11C,  0x123,  0x129,  0x12D,       	
	0x131,  0x134,  0x137,  0x13A,
	
	0x0C0,
	0x101,
	ENDDEF, 0x0000
};


static const u16 acl_cutoff_45p[] = {
	0x0F0,
	0x15A,
	0x15A,

	0x0C1,
	0x14D,	0x196,	0x11D,
	0x100,	0x100,	0x101,	0x1DF,
	0x100,	0x100,	0x103,	0x11F,
	0x100,  0x100,  0x100,  0x100,       	
	0x100,  0x101,  0x107,  0x113,       	
	0x11E,  0x125,  0x12B,  0x130,       	
	0x134,  0x137,  0x13A,  0x13D,
	
	0x0C0,
	0x101,
	ENDDEF, 0x0000
};

static const u16 acl_cutoff_47p[] = {
	0x0F0,
	0x15A,
	0x15A,

	0x0C1,
	0x14D,	0x196,	0x11D,
	0x100,	0x100,	0x101,	0x1DF,
	0x100,	0x100,	0x103,	0x11F,
	0x100,  0x100,  0x100,  0x100,       	
	0x100,  0x101,  0x107,  0x114,       	
	0x120,  0x128,  0x12E,  0x133,       	
	0x137,  0x13B,  0x13E,  0x141,
	
	0x0C0,
	0x101,
	ENDDEF, 0x0000
};

static const u16 acl_cutoff_48p[] = {
	0x0F0,
	0x15A,
	0x15A,

	0x0C1,
	0x14D,	0x196,	0x11D,
	0x100,	0x100,	0x101,	0x1DF,
	0x100,	0x100,	0x103,	0x11F,
	0x100,  0x100,  0x100,  0x100,       	
	0x100,  0x101,  0x108,  0x115,
	0x120,  0x129,  0x12F,  0x134,       	
	0x139,  0x13D,  0x140,  0x143,
	
	0x0C0,
	0x101,
	ENDDEF, 0x0000
};

static const u16 acl_cutoff_50p[] = {
	0x0F0,
	0x15A,
	0x15A,

	0x0C1,
	0x14D,	0x196,	0x11D,
	0x100,	0x100,	0x101,	0x1DF,
	0x100,	0x100,	0x103,	0x11F,
	0x100,  0x100,  0x100,  0x100,       	
	0x100,  0x101,  0x108,  0x116,       	
	0x122,  0x12B,  0x131,  0x137,       	
	0x13B,  0x13F,  0x143,  0x146,	
	
	0x0C0,
	0x101,
	ENDDEF, 0x0000
};

static const u16 acl_cutoff_55p[] = {
	0x0F0,
	0x15A,
	0x15A,

	0x0C1,
	0x14D,	0x196,	0x11D,
	0x100,	0x100,	0x101,	0x1DF,
	0x100,	0x100,	0x103,	0x11F,
	0x100,  0x100,  0x100,  0x100,       	
	0x100,  0x101,  0x109,  0x118,       	
	0x126,  0x12F,  0x137,  0x13D,       	
	0x142,  0x147,  0x14A,  0x14E,	
	
	0x0C0,
	0x101,
	ENDDEF, 0x0000
};

static const u16 acl_cutoff_60p[] = {	
	0x0F0,	
	0x15A,	
	0x15A,	

	0x0C1,	
	0x14D,	0x196,	0x11D,	
	0x100,	0x100,	0x101,	0x1DF,	
	0x100,	0x100,	0x103,	0x11F,	
	0x100,  0x100,  0x100,  0x100,
	0x100,  0x101,  0x10A,  0x11B,
	0x12A,  0x135,  0x13D,  0x144,
	0x14A,  0x14F,  0x153,  0x157,
	
	0x0C0,
	0x101,	
	ENDDEF, 0x0000
};

static const u16 acl_cutoff_65p[] = {
	0x0F0,
	0x15A,
	0x15A,

	0x0C1,
	0x14D,	0x196,	0x11D,
	0x100,	0x100,	0x101,	0x1DF,
	0x100,	0x100,	0x103,	0x11F,
	0x100,  0x100,  0x100,  0x100,       	
	0x100,  0x101,  0x10B,  0x11E,       	
	0x12F,  0x13B,  0x144,  0x14C,       	
	0x152,  0x158,  0x15D,  0x161,	

	0x0C0,
	0x101,
	ENDDEF, 0x0000
};

static const u16 acl_cutoff_70p[] = {
	0x0F0,
	0x15A,
	0x15A,

	0x0C1,
	0x14D,	0x196,	0x11D,
	0x100,	0x100,	0x101,	0x1DF,
	0x100,	0x100,	0x103,	0x11F,
	0x100,  0x100,  0x100,  0x100,       	
	0x100,  0x101,  0x10C,  0x121,       	
	0x134,  0x141,  0x14B,  0x153,       	
	0x15B,  0x161,  0x166,  0x16B,
	
	0x0C0,
	0x101,
	ENDDEF, 0x0000
};

static const u16 acl_cutoff_75p[] = {
	0x0F0,
	0x15A,
	0x15A,

	0x0C1,
	0x14D,	0x196,	0x11D,
	0x100,	0x100,	0x101,	0x1DF,
	0x100,	0x100,	0x103,	0x11F,
	0x100,  0x100,  0x100,  0x100,       	
	0x100,  0x101,  0x10D,  0x124,       	
	0x139,  0x147,  0x153,  0x15C,       	
	0x164,  0x16B,  0x171,  0x176,	

	0x0C0,
	0x101,
	ENDDEF, 0x0000
};

static const u16 acl_cutoff_80p[] = {
	0x0F0,
	0x15A,
	0x15A,

	0x0C1,
	0x14D,	0x196,	0x11D,
	0x100,	0x100,	0x101,	0x1DF,
	0x100,	0x100,	0x103,	0x11F,
	0x100,  0x100,  0x100,  0x100,       	
	0x100,  0x101,  0x10E,  0x128,       	
	0x13F,  0x14F,  0x15C,  0x166,       	
	0x16F,  0x176,  0x17D,  0x183,	

	0x0C0,
	0x101,
	ENDDEF, 0x0000
};

static const u16 acl_cutoff_85p[] = {
	0x0F0,
	0x15A,
	0x15A,

	0x0C1,
	0x14D,	0x196,	0x11D,
	0x100,	0x100,	0x101,	0x1DF,
	0x100,	0x100,	0x103,	0x11F,
	0x100,  0x100,  0x100,  0x100,       	
	0x100,  0x101,  0x110,  0x12D,       	
	0x147,  0x159,  0x167,  0x173,       	
	0x17C,  0x185,  0x18C,  0x193,	

	0x0C0,
	0x101,
	ENDDEF, 0x0000
};



static const u16 *ACL_cutoff_set[] = {
	acl_cutoff_off,
	acl_cutoff_8p,
	acl_cutoff_14p,
	acl_cutoff_20p,
	acl_cutoff_24p,
	acl_cutoff_28p,
	acl_cutoff_32p,
	acl_cutoff_35p,
	acl_cutoff_37p,
	acl_cutoff_40p,
	acl_cutoff_45p,
	acl_cutoff_47p,
	acl_cutoff_48p,
	acl_cutoff_50p, 
	acl_cutoff_60p,
	acl_cutoff_75p,
	acl_cutoff_43p,
};

struct s5p_panel_data wave_panel_data = {
	.seq_display_set = s6e63m0_SEQ_DISPLAY_SETTING,
	.seq_etc_set = s6e63m0_SEQ_ETC_SETTING,
	.display_on = s6e63m0_SEQ_DISPLAY_ON,
	.display_off = s6e63m0_SEQ_DISPLAY_OFF,
	.standby_on = s6e63m0_SEQ_STANDBY_ON,
	.standby_off = s6e63m0_SEQ_STANDBY_OFF,
	.gamma19_table = p19Gamma_set,
	.gamma22_table = p22Gamma_set,
	.gamma_update = gamma_updates,
	.acl_table = ACL_cutoff_set,
	.acl_init = acl_cutoff_init,
	.gamma_table_size = ARRAY_SIZE(p22Gamma_set),
};

#else

static const u16 s6e63m0_SEQ_STANDBY_ON[] = {
	0x010,	/* Stand-by On Command */
	SLEEPMSEC, 160,
	ENDDEF, 0x0000
};

static const u16 s6e63m0_SEQ_STANDBY_OFF[] = {
	0x011,	/* Stand-by Off Command */
	SLEEPMSEC, 120,
	ENDDEF, 0x0000
};

static const u16 s6e63m0_SEQ_DISPLAY_SETTING[] = {
	SLEEPMSEC, 10,
	0x0F8,	/* Panel Condition Set Command*/
	0x101,	/* DOCT */
	0x127,	/* CLWEA */
	0x127,	/* CLWEB*/
	0x107,	/* CLTE */
	0x107,	/* SHE */
	0x154,	/* FLTE */
	0x19F,	/* FLWE */
	0x163,	/* SCTE */
	0x186,	/* SCWE */
	0x11A,	/* INTE */
	0x133,	/* INWE */
	0x10D,	/* EMPS */
	0x100,	/* E_INTE */
	0x100,	/* E_INWE */
	0x0F2,	/* Display Condition Set Command*/
	0x102,	/* Number of Line */
	0x103,	/* VBP */
	0x11C,	/* VFP */
	0x110,	/* HBP */
	0x110,	/* HFP */
	0x0F7,	/* Command */
	0x103,	/* GTCON */
	0x100,	/* Display Mode */
	0x100,	/* Vsync/Hsync, DOCCLK, RGB mode */
	ENDDEF, 0x0000
};

static const u16 s6e63m0_SEQ_ETC_SETTING[] = {
	/* ETC Condition Set Command  */
	0x0F6,
	0x100,	0x18E,
	0x107,
	0x0B3,
	0x16C,
	0x0B5,
	0x127,	0x10A,
	0x109,	0x107,
	0x130,	0x11C,
	0x113,	0x109,
	0x110,	0x11A,
	0x12A,	0x124,
	0x11F,	0x11B,
	0x11A,	0x117,
	0x12B,	0x126,
	0x122,	0x120,
	0x13A,	0x134,
	0x130,	0x12C,
	0x129,	0x126,
	0x125,	0x123,
	0x121,	0x120,
	0x11E,	0x11E,
	0x0B6,
	0x100,	0x100,
	0x123,	0x111,
	0x132,	0x144,
	0x144,	0x144,
	0x155,	0x155,
	0x166,	0x166,
	0x166,	0x166,
	0x166,	0x166,
	0x0B7,
	0x127,	0x10A,
	0x109,	0x107,
	0x130,	0x11C,
	0x113,	0x109,
	0x110,	0x11A,
	0x12A,	0x124,
	0x11F,	0x11B,
	0x11A,	0x117,
	0x12B,	0x126,
	0x122,	0x120,
	0x13A,	0x134,
	0x130,	0x12C,
	0x129,	0x126,
	0x125,	0x123,
	0x121,	0x120,
	0x11E,	0x11E,
	0x0B8,
	0x100,	0x100,
	0x123,	0x111,
	0x132,	0x144,
	0x144,	0x144,
	0x155,	0x155,
	0x166,	0x166,
	0x166,	0x166,
	0x166,	0x166,
	0x0B9,
	0x127,	0x10A,
	0x109,	0x107,
	0x130,	0x11C,
	0x113,	0x109,
	0x110,	0x11A,
	0x12A,	0x124,
	0x11F,	0x11B,
	0x11A,	0x117,
	0x12B,	0x126,
	0x122,	0x120,
	0x13A,	0x134,
	0x130,	0x12C,
	0x129,	0x126,
	0x125,	0x123,
	0x121,	0x120,
	0x11E,	0x11E,
	0x0BA,
	0x100,	0x100,
	0x123,	0x111,
	0x132,	0x144,
	0x144,	0x144,
	0x155,	0x155,
	0x166,	0x166,
	0x166,	0x166,
	0x166,	0x166,
	0x011,
	SLEEPMSEC, 120,
	0x029,
	ENDDEF, 0x0000
};

static const struct tl2796_gamma_adj_points gamma_adj_points = {
	.v0 = 0,
	.v1 = BV_1,
	.v19 = BV_19,
	.v43 = BV_43,
	.v87 = BV_87,
	.v171 = BV_171,
	.v255 = BV_255,
};

static const struct gamma_entry gamma_table[] = {
	{       BV_0, { 4200000, 4200000, 4200000, }, },
	{          1, { 3994200, 4107600, 3910200, }, },
	{ 0x00000400, { 3669486, 3738030, 3655093, }, },
	{ 0x000004C2, { 3664456, 3732059, 3649872, }, },
	{ 0x000005A8, { 3659356, 3726019, 3644574, }, },
	{ 0x000006BA, { 3654160, 3719879, 3639171, }, },
	{ 0x00000800, { 3648872, 3713646, 3633668, }, },
	{ 0x00000983, { 3643502, 3707331, 3628075, }, },
	{ 0x00000B50, { 3638029, 3700909, 3622368, }, },
	{ 0x00000D74, { 3632461, 3694392, 3616558, }, },
	{ 0x00001000, { 3626792, 3687772, 3610636, }, },
	{ 0x00001307, { 3621022, 3681052, 3604605, }, },
	{ 0x000016A1, { 3615146, 3674224, 3598455, }, },
	{ 0x00001AE9, { 3609163, 3667289, 3592189, }, },
	{ 0x00002000, { 3603070, 3660245, 3585801, }, },
	{ 0x0000260E, { 3596860, 3653083, 3579284, }, },
	{ 0x00002D41, { 3590532, 3645805, 3572637, }, },
	{ 0x000035D1, { 3584081, 3638403, 3565854, }, },
	{ 0x00004000, { 3577504, 3630876, 3558930, }, },
	{ 0x00004C1C, { 3570797, 3623221, 3551863, }, },
	{ 0x00005A82, { 3563956, 3615434, 3544649, }, },
	{ 0x00006BA2, { 3556976, 3607510, 3537279, }, },
	{ 0x00008000, { 3549853, 3599444, 3529750, }, },
	{ 0x00009838, { 3542582, 3591234, 3522056, }, },
	{ 0x0000B505, { 3535159, 3582874, 3514193, }, },
	{ 0x0000D745, { 3527577, 3574360, 3506153, }, },
	{ 0x00010000, { 3519832, 3565687, 3497931, }, },
	{ 0x00013070, { 3511918, 3556849, 3489519, }, },
	{ 0x00016A0A, { 3503829, 3547842, 3480912, }, },
	{ 0x0001AE8A, { 3495559, 3538659, 3472102, }, },
	{ 0x00020000, { 3487101, 3529295, 3463080, }, },
	{ 0x000260E0, { 3478447, 3519742, 3453839, }, },
	{ 0x0002D414, { 3469592, 3509996, 3444372, }, },
	{ 0x00035D14, { 3460527, 3500049, 3434667, }, },
	{ 0x00040000, { 3451244, 3489893, 3424717, }, },
	{ 0x0004C1C0, { 3441734, 3479522, 3414512, }, },
	{ 0x0005A828, { 3431990, 3468927, 3404040, }, },
	{ 0x0006BA28, { 3422000, 3458099, 3393292, }, },
	{ 0x00080000, { 3411756, 3447030, 3382254, }, },
	{ 0x0009837F, { 3401247, 3435711, 3370915, }, },
	{ 0x000B504F, { 3390462, 3424131, 3359262, }, },
	{ 0x000D7450, { 3379388, 3412280, 3347281, }, },
	{ 0x00100000, { 3368014, 3400147, 3334957, }, },
	{ 0x001306FE, { 3356325, 3387721, 3322274, }, },
	{ 0x0016A09E, { 3344309, 3374988, 3309216, }, },
	{ 0x001AE8A0, { 3331950, 3361936, 3295765, }, },
	{ 0x00200000, { 3319231, 3348550, 3281902, }, },
	{ 0x00260DFC, { 3306137, 3334817, 3267607, }, },
	{ 0x002D413D, { 3292649, 3320719, 3252859, }, },
	{ 0x0035D13F, { 3278748, 3306240, 3237634, }, },
	{ 0x00400000, { 3264413, 3291361, 3221908, }, },
	{ 0x004C1BF8, { 3249622, 3276065, 3205654, }, },
	{ 0x005A827A, { 3234351, 3260329, 3188845, }, },
	{ 0x006BA27E, { 3218576, 3244131, 3171449, }, },
	{ 0x00800000, { 3202268, 3227448, 3153434, }, },
	{ 0x009837F0, { 3185399, 3210255, 3134765, }, },
	{ 0x00B504F3, { 3167936, 3192523, 3115404, }, },
	{ 0x00D744FD, { 3149847, 3174223, 3095308, }, },
	{ 0x01000000, { 3131093, 3155322, 3074435, }, },
	{ 0x01306FE1, { 3111635, 3135786, 3052735, }, },
	{ 0x016A09E6, { 3091431, 3115578, 3030156, }, },
	{ 0x01AE89FA, { 3070432, 3094655, 3006641, }, },
	{ 0x02000000, { 3048587, 3072974, 2982127, }, },
	{ 0x0260DFC1, { 3025842, 3050485, 2956547, }, },
	{ 0x02D413CD, { 3002134, 3027135, 2929824, }, },
	{ 0x035D13F3, { 2977397, 3002865, 2901879, }, },
	{ 0x04000000, { 2951558, 2977611, 2872620, }, },
	{ 0x04C1BF83, { 2924535, 2951302, 2841948, }, },
	{ 0x05A8279A, { 2896240, 2923858, 2809753, }, },
	{ 0x06BA27E6, { 2866574, 2895192, 2775914, }, },
	{ 0x08000000, { 2835426, 2865207, 2740295, }, },
	{ 0x09837F05, { 2802676, 2833793, 2702744, }, },
	{ 0x0B504F33, { 2768187, 2800829, 2663094, }, },
	{ 0x0D744FCD, { 2731806, 2766175, 2621155, }, },
	{ 0x10000000, { 2693361, 2729675, 2576712, }, },
	{ 0x1306FE0A, { 2652659, 2691153, 2529527, }, },
	{ 0x16A09E66, { 2609480, 2650402, 2479324, }, },
	{ 0x1AE89F99, { 2563575, 2607191, 2425793, }, },
	{ 0x20000000, { 2514655, 2561246, 2368579, }, },
	{ 0x260DFC14, { 2462394, 2512251, 2307272, }, },
	{ 0x2D413CCD, { 2406412, 2459834, 2241403, }, },
	{ 0x35D13F32, { 2346266, 2403554, 2170425, }, },
	{ 0x40000000, { 2281441, 2342883, 2093706, }, },
	{ 0x4C1BF828, { 2211332, 2277183, 2010504, }, },
	{ 0x5A82799A, { 2135220, 2205675, 1919951, }, },
	{ 0x6BA27E65, { 2052250, 2127391, 1821028, }, },
	{ 0x80000000, { 1961395, 2041114, 1712536, }, },
	{ 0x9837F051, { 1861415, 1945288, 1593066, }, },
	{ 0xB504F333, { 1750800, 1837874, 1460986, }, },
	{ 0xD744FCCA, { 1627706, 1716150, 1314437, }, },
	{ 0xFFFFFFFF, { 1489879, 1576363, 1151415, }, },
};

static void reset_lcd(struct s5p_panel_data *pdata)
{
	gpio_direction_output(pdata->gpio_rst, 1);
	msleep(10);

	gpio_set_value(pdata->gpio_rst, 0);
	msleep(10);

	gpio_set_value(pdata->gpio_rst, 1);
	msleep(10);
}

static int configure_mtp_gpios(struct s5p_panel_data *pdata, bool enable)
{
	int i;
	int ret = 0;
	if (enable) {
		/* wrx and csx are already requested by the spi driver */
		ret = gpio_request(pdata->gpio_rdx, "tl2796_rdx");
		if (ret)
			goto err_rdx;
		ret = gpio_request(pdata->gpio_dcx, "tl2796_dcx");
		if (ret)
			goto err_dcx;
		ret = gpio_request(pdata->gpio_rst, "tl2796_rst");
		if (ret)
			goto err_rst;
		for (i = 0; i < 8; i++) {
			ret = gpio_request(pdata->gpio_db[i], "tl2796_dbx");
			if (ret)
				goto err_dbx;
		}
		for (i = 0; i < 8; i++) {
			s3c_gpio_cfgpin(S5PV210_GPF0(i), S3C_GPIO_OUTPUT);
			s3c_gpio_setpull(S5PV210_GPF0(i), S3C_GPIO_PULL_NONE);
			gpio_set_value(S5PV210_GPF0(i), 0);
		}
		for (i = 0; i < 8; i++) {
			s3c_gpio_cfgpin(S5PV210_GPF1(i), S3C_GPIO_OUTPUT);
			s3c_gpio_setpull(S5PV210_GPF1(i), S3C_GPIO_PULL_UP);
			gpio_set_value(S5PV210_GPF1(i), 0);
		}
		return 0;
	}

	for (i = 0; i < 8; i++) {
		s3c_gpio_cfgpin(S5PV210_GPF0(i), S3C_GPIO_SFN(2));
		s3c_gpio_setpull(S5PV210_GPF0(i), S3C_GPIO_PULL_NONE);
	}
	for (i = 0; i < 8; i++) {
		s3c_gpio_cfgpin(S5PV210_GPF1(i), S3C_GPIO_SFN(2));
		s3c_gpio_setpull(S5PV210_GPF1(i), S3C_GPIO_PULL_NONE);
	}

	reset_lcd(pdata);

	for (i = 7; i >= 0; i--) {
		gpio_free(pdata->gpio_db[i]);
err_dbx:
		;
	}
	gpio_free(pdata->gpio_rst);
err_rst:
	gpio_free(pdata->gpio_dcx);
err_dcx:
	gpio_free(pdata->gpio_rdx);
err_rdx:
	return ret;
}

struct s5p_panel_data wave_panel_data = {
	.seq_display_set = s6e63m0_SEQ_DISPLAY_SETTING,
	.seq_etc_set = s6e63m0_SEQ_ETC_SETTING,
	.standby_on = s6e63m0_SEQ_STANDBY_ON,
	.standby_off = s6e63m0_SEQ_STANDBY_OFF,
	.gpio_dcx = S5PV210_GPF0(0), /* H_SYNC pad */
	.gpio_rdx = S5PV210_GPF0(2), /* Enable */
	.gpio_csx = GPIO_DISPLAY_CS,
	.gpio_wrx = S5PV210_MP04(1), /* SCL pad */
	.gpio_rst = S5PV210_MP05(5),
	.gpio_db = {
		S5PV210_GPF0(4),
		S5PV210_GPF0(5),
		S5PV210_GPF0(6),
		S5PV210_GPF0(7),
		S5PV210_GPF1(0),
		S5PV210_GPF1(1),
		S5PV210_GPF1(2),
		S5PV210_GPF1(3),
	},
	.configure_mtp_gpios = configure_mtp_gpios,
	.factory_v255_regs = {
		0x0b9,
		0x0b8,
		0x0fc,
	},
	.gamma_adj_points = &gamma_adj_points,
	.gamma_table = gamma_table,
	.gamma_table_size = ARRAY_SIZE(gamma_table),
};

#endif
