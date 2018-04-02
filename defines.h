/*
 * This file is part of 'pdp', a PDP-11 simulator.
 *
 * For information contact:
 *
 *   Computer Science House
 *   Attn: Eric Edwards
 *   Box 861
 *   25 Andrews Memorial Drive
 *   Rochester, NY 14623
 *
 * Email:  mag@potter.csh.rit.edu
 * FTP:    ftp.csh.rit.edu:/pub/csh/mag/pdp.tar.Z
 * 
 * Copyright 1994, Eric A. Edwards
 *
 * Permission to use, copy, modify, and distribute this
 * software and its documentation for any purpose and without
 * fee is hereby granted, provided that the above copyright
 * notice appear in all copies.  Eric A. Edwards makes no
 * representations about the suitability of this software
 * for any purpose.  It is provided "as is" without expressed
 * or implied warranty.
 */

/*
 * defines.h
 */

typedef unsigned bool;
enum { false, true };

/*
 * Stuff to maintain compatibility across platforms.
 */

#ifndef DEFINES_INCLUDED
#define DEFINES_INCLUDED
#include <sys/types.h>
#include <sys/time.h>
#include <sys/stat.h>
#include <signal.h>
#include <stdio.h>
#include <stdlib.h>		/* COMMENT for vax-bsd */

#ifndef __MINGW32__

#include <sgtty.h>


#endif

#include <ctype.h>
#include <fcntl.h>
#include <string.h>

/*
 * Type definitions for PDP data types.
 */

typedef unsigned int c_addr;	/* core or BK Q-bus address (17 bit so far) */
typedef unsigned short l_addr;	/* logical address (16 bit) */
typedef unsigned short d_word;	/* data word (16 bit) */
typedef unsigned char d_byte;	/* data byte (8 bit) */
typedef unsigned char flag_t;	/* for boolean or small value flags */

/*
 * PDP processor defines.
 */

// CPU registers
enum CPU_regset {
	R0 = 0,
	R1 = 1,
	R2 = 2,
	R3 = 3,
	R4 = 4,
	R5 = 5,		/* return register for MARK */
	R6 = 6,
	SP = 6,		/* stack pointer */
	R7 = 7,
	PC = 7		/* program counter */
	};

enum CPU_CC {
	CC_C = 01,			/* carry */
	CC_V = 02,			/* overflow */
	CC_Z = 04,			/* zero/equal */
	CC_N = 010,			/* negative */
	CC_T = 020,			/* trap bit */
	};

/*
 * Definitions for the memory map and memory operations.
 */

/* memory and instruction results */

typedef enum _CPU_RES {
		CPU_OK = 0,
		CPU_ODD_ADDRESS = 1,
		CPU_BUS_ERROR = 2,
		CPU_MMU_ERROR = 3,
		CPU_ILLEGAL = 4,
		CPU_HALT = 5,
		CPU_WAIT = 6,
		CPU_NOT_IMPL = 7,
		CPU_TRAP = 8,
		CPU_EMT = 9,
		CPU_BPT = 10,
		CPU_IOT = 11,
		CPU_RTT = 12,
		CPU_RTI = 13,
		CPU_TURBO_FAIL = 14
		} CPU_RES;

typedef struct _pdp_regs {
	d_word regs[8];		/* general registers */
	d_byte psw;		/* processor status byte (LSI-11) */
	d_word ir;		/* current instruction register */
	unsigned long total;	/* count of instructions executed */
	unsigned long clock;	/* CPU ticks */
	} pdp_regs;

CPU_RES rts_PC (pdp_regs *p);

/*
 * Q-bus device addresses.
 */

#define BASIC           0120000
#define BASIC_SIZE      (24 * 512)      /* 24 Kbytes */
#define PORT_REG        0177714         /* printer, mouse, covox, ... */
#define PORT_SIZE       1

#define TERAK_BOOT	0173000
#define TERAK_BSIZE	0200

#define TERAK_DISK_REG 0177000
#define TERAK_DISK_SIZE 2

#define PDP_READABLE_MEM_SIZE   (63 * 512)  /* 0 - 175777 */
#define PDP_FULL_MEM_SIZE       (64 * 512)  /* 0 - 177777 */

extern d_word rom[4][8192], ram[8][8192], system_rom[8192];
extern void boot_init();
extern void scr_init(), scr_write(int, c_addr, d_word), scr_switch(int, int);
extern void disk_finish();
extern void tdisk_finish();
extern void io_read_start();
extern CPU_RES cpu_service(d_word interrupt);

/*
 * Defines for the event handling system.
 */

#define NUM_PRI         2

/* Timer interrupt has higher priority */
#define TIMER_PRI	0
#define TTY_PRI		1

typedef struct _event {
	int (*handler)();		/* handler function */
	d_word info;			/* info or vector number */
	double when;			/* when to fire this event */
} event;

/*
 * Global variables.
 */

extern int ui_done;
extern unsigned short last_branch;
extern pdp_regs pdp;
extern char *printer_file;
extern char *romdir;
extern char *rompath10, *rompath12, *rompath16;
extern char *bos11rom, *diskrom, *bos11extrom, *basic11arom, *basic11brom;
extern int TICK_RATE;

extern char *floppyA, *floppyB, *floppyC, *floppyD;
extern unsigned short tty_scroll;
extern unsigned scr_dirty;
extern flag_t key_pressed;
extern flag_t in_wait_instr;
extern flag_t io_stop_happened;
extern flag_t cflag, mouseflag, bkmodel, terak, nflag, fullspeed;
extern flag_t timer_intr_enabled, fullscreen, fake_disk;

/*
 * Inline defines.
 */

/* For BK-0010 */

#define ll_word(p, a, w) lc_word(a, w)
#define sl_word(p, a, w) sc_word(a, w)

#define LSBIT	1		/*  least significant bit */

#define	MPI	0077777		/* most positive integer */
#define MNI	0100000		/* most negative integer */
#define NEG_1	0177777		/* negative one */
#define SIGN	0100000		/* sign bit */
#define CARRY   0200000		/* set if carry out */

#define	MPI_B	0177			/* most positive integer (byte) */
#define MNI_B	0200			/* most negative integer (byte) */
#define NEG_1_B	0377		/* negative one (byte) */
#define SIGN_B	0200		/* sign bit (byte) */
#define CARRY_B	0400		/* set if carry out (byte) */

#define LOW16(data)	((data) & 0177777)	/* mask the lower 16 bits */
#define LOW8(data)	((data) & 0377)			/* mask the lower 8 bits */

/*
 * Device list
 */

typedef struct _pdp_qmap {
	char *s_name, *l_name;

	c_addr start;
	c_addr size;

	void (*dev_init) ();											// (init)
	CPU_RES (*dev_read) (c_addr, d_word*);		// (word read)
	CPU_RES (*dev_write) (c_addr, d_word);		// (word write)
	CPU_RES (*dev_bwrite)(c_addr, d_byte);		// (byte write)

	struct _pdp_qmap *next;					// (next in chain)
} pdp_qmap;

extern pdp_qmap q_printer;
extern pdp_qmap q_mouse;
extern pdp_qmap q_covox;
extern pdp_qmap q_synth;
extern pdp_qmap q_bkplip;
extern pdp_qmap q_port;
extern pdp_qmap q_tty;
extern pdp_qmap q_io;
extern pdp_qmap q_timer;
extern pdp_qmap q_line;
extern pdp_qmap q_basicmem;
extern pdp_qmap q_disk;
extern pdp_qmap q_secret;

/* Logging flags */
extern flag_t log_checkpt;

extern flag_t log_timer;
extern flag_t log_line;
extern flag_t log_tape;
extern flag_t log_secret;

extern flag_t log_EMT;
extern flag_t log_TRAP;

void logF (flag_t msg_class, char *format, ...);

/* BK models */
enum HW_enum {
	HW_BK0010 = 0,			/* Original BK0010 */
	HW_BK0010_01 = 1,		/* BK0010.01 */
	HW_BK0010_01F = 2,	/* BK0010.01+FDD */
  HW_BK0011 = 3,			/* BK-0011M */
	HW_BK0011S = 4,			/* Slow BK-0011M */

	HW_TERAK = 9				/* Terak 8510/a */
	};

/* Global functions */
extern char * state(pdp_regs * p);
extern c_addr disas(c_addr pc, char * dest);

#endif

int close();
unsigned covox_sample(d_byte);
void device_install();
void disk_open();
void ev_fire();
void ev_init();
void ev_register();
void fake_reset();
void get_emt36_filename();
unsigned get_ticks();
void init_config();
void input_processing();
void kbd_init();
CPU_RES lc_word();
CPU_RES ll_byte(pdp_regs *, d_word, d_byte *);
void mem_init();
void mouse_buttonevent();
void mouse_moveevent();
void pagereg_bwrite(d_byte);
void pagereg_write(d_word);
void q_reset();
void run();
void scr_flush();
void scr_param_change();
void scr_sync();
CPU_RES sc_word(c_addr, d_word);
d_word serial_read();
void serial_write(d_word);
void showbkhelp();
void showemuhelp();
CPU_RES sl_byte(pdp_regs *, d_word, d_byte);
void sound_init();
int synth_next();
void tape_init();
int tape_read();
void tape_write();
void timer_check();
void timer_setmode(d_byte);
void tty_keyevent();
void tty_open();
void ui();
void wait_ticks();
