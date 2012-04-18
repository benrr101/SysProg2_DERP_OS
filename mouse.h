#ifndef MOUSE_H
#define MOUSE_H

// Comm ports
#define PS2_PORT	0x60
#define PS2_STAT	0x64

// Command Set
// Source: http://www.computer-engineering.org/ps2mouse/
#define PS2_M_RST	0xFF
#define PS2_M_RSND	0xFE
#define PS2_M_SDEF	0xF6
#define PS2_M_DDR	0xF5
#define PS2_M_EDR	0xF4
#define PS2_M_SAMP	0xF3
#define PS2_M_DEVI	0xF2
#define PS2_M_SREM	0xF0
#define PS2_M_SWRP	0xEE
#define PS2_M_RWRP	0xFA
#define PS2_M_RDAT	0xEB
#define PS2_M_STRM	0xEA
#define PS2_M_STAT	0xE9
#define PS2_M_SRES	0xE8
#define PS2_M_SCA2	0xE7
#define PS2_M_SCA1	0xE6
#define PS2_M_SCOM	0xD4

// Response Set
#define PS2_M_ERR	0xFC
#define PS2_M_ACK	0xFA
#define PS2_M_BATC	0xAA

// IRQs
#define PS2_M_VEC	12

// Init Functions
void _ps2_init( void );
void _ps2_mouse_init( void );

// ISR Functions
void _ps2_mouse_isr( int vec, int code ); 

// Read/Write functions
char _ps2_read();
void _ps2_write(char b);

#endif
