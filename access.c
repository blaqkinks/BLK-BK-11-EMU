/*
 * This file is part of a BK-0010/11M simulator.
 * Originally
 * Copyright 1994, Eric A. Edwards
 * After heavy modifications
 * Public Domain - Allynd Dudnikov 2018
 *
 * Permission to use, copy, modify, and distribute this
 * software and its documentation for any purpose and without
 * fee is hereby granted, provided that the above copyright
 * notices appear in all copies.  Allynd Dudnikov (blaqk) makes no
 * representations about the suitability of this software
 * for any purpose.  It is provided "as is" without expressed
 * or implied warranty.
 */

/*
 * access.c - Access routines to read and write locations on the QBUS
 * and main memory.  
 */

#include "defines.h"
#include <libintl.h>
#define _(String) gettext (String)

/* 
 * BK-0011 has 8 8Kw RAM pages and 4 8 Kw ROM pages.
 * RAM pages 1 and 7 are video RAM.
 */
d_word ram[8][8192];
d_word rom[4][8192];
d_word system_rom[8192];
unsigned char umr[65536];

/*
 * Page mapping, per 8 Kw page. Default is a mapping mimicking BK-0010
 */
d_word * pagemap[4] = { ram[6], ram[1], rom[0], system_rom };

#define mem(x) pagemap[(x)>>14][((x) & 037777) >> 1]

/*
 * Each bit corresponds to a Kword,
 * the lowest 8 Kwords are RAM, the next 8 are screen memory,
 * the rest is usually ROM.
 */
unsigned long pdp_ram_map = 0x0000ffff;
unsigned long pdp_mem_map;

#define IS_RAM_ADDRESS(x) ((pdp_ram_map >> ((x) >> 11)) & 1)
#define IS_VALID_ADDRESS(x) ((pdp_mem_map >> ((x) >> 11)) & 1)

/*
 * The QBUS memory map.
 */

/*
 * q_null() - Null QBUS device switch handler.
 */

void q_null()
{
}

/*
 * q_err() - Returns BUS error
 */

static CPU_RES q_err(c_addr x, d_word y) {
	logF(0, "Writing word to ROM addr %06o:", x);
	return CPU_BUS_ERROR;
}

static CPU_RES q_errb(c_addr x, d_byte y) {
	logF(0, "Writing byte to ROM addr %06o:", x);
	return CPU_BUS_ERROR;
}

static CPU_RES force_read(c_addr addr, d_word *word) {
	*word = mem(addr);
	return CPU_OK;
}	// force_read

#if 0
static CPU_RES terak_read(c_addr addr, d_word *word) {
	if (addr == 0173176) fprintf(stderr, "Reading serial num\n");
	if (addr >= 0173000 && addr < 0173200) {
        *word = mem(addr);
        return CPU_OK;
	}
	return CPU_BUS_ERROR;
}	// terak_read
#endif

void null_init () {
}	// null_init

/* When nothing is connected to the port */
static CPU_RES port_read(c_addr a, d_word *d) {
	*d = 0;		/* pulldown */
	fprintf(stderr, "Reading port %06o\n", a);
	return CPU_OK;
}	// port_read

static CPU_RES port_write(c_addr a, d_word d) {
	fprintf(stderr, "Writing %06o to port %06o\n", d, a);
	return CPU_OK;	/* goes nowhere */
}	// port_write

static CPU_RES port_bwrite(c_addr a, d_byte d) {
	fprintf(stderr, "Writing %03o to port %06o\n", d, a);
	return CPU_OK;	/* goes nowhere */
}	// port_bwrite

pdp_qmap q_port = {
	"ext_port", "External I/O port (unattached)",
	PORT_REG, PORT_SIZE, null_init, port_read, port_write, port_bwrite
	};

pdp_qmap q_basicmem = 
	/* BASIC memory is not transparently readable, thus force_read */
	{ "basic", "BASIC ROM", BASIC, BASIC_SIZE, null_init, force_read, q_err, q_errb };

// TODO
#if 0
pdp_qmap qmap_terak[] = {
	{ TERAK_DISK_REG, TERAK_DISK_SIZE, tdisk_init, tdisk_read,
	tdisk_write, tdisk_bwrite },
	{ 0177564, 4, q_null, tcons_read, tcons_write, tcons_write }, 
	{ 0177764, 4, q_null, tcons_read, tcons_write, tcons_write }, 
	{ 0177744, 2, q_null, port_read, port_write, port_bwrite },
	{ 0177560, 2, q_null, port_read, port_write, port_bwrite },
	{ 0173000, 0200, q_null, terak_read, q_err, q_errb },
	{ 0, 0, 0, 0, 0, 0 }
};
#endif

static pdp_qmap *q_head = 0;

void device_install (pdp_qmap *dev) {
dev->next = q_head;
q_head = dev;

logF (0, "Installed device: %s\n", dev->s_name);
}	// device_install

/*
 * lc_word() - Load a word from the given core address.
 */

CPU_RES lc_word (c_addr addr, d_word *word) {

	addr &= ~1;

	if ( IS_VALID_ADDRESS(addr)) {
		if (!umr[addr]) {
			// fprintf(stderr, "UMR @ %06o\n", addr);
		}
		*word = mem(addr);
		return CPU_OK;
		}

	pdp_qmap *q_ptr;

	for (q_ptr = q_head; q_ptr; q_ptr = q_ptr->next)
		if ((addr >= q_ptr->start) &&
		 	  (addr < (q_ptr->start + (q_ptr->size << 1))))
				return (q_ptr->dev_read) (addr, word);

	fprintf(stderr, _("Illegal read address %06o:"), addr);
	return CPU_BUS_ERROR;
}

/*
 * If the address corresponds to a video page (1 or 2)
 * returns 1 or 2, otherwise 0.
 */
unsigned char video_map[4] = { 0, 1, 0, 0 };
#define VIDEO_PAGE(x) video_map[(x)>>14]
/*
 * bits 14-12 - page mapped to addresses 040000-077777
 * bits 10-8 - page mapped to addresses 100000-137777
 * bit 0 - ROM 0 mapped to addresses 100000-137777
 * bit 1 - ROM 1 mapped to addresses 100000-137777
 * bit 3 - ROM 2 mapped to addresses 100000-137777
 * bit 4 - ROM 3 mapped to addresses 100000-137777
 */
static d_word oldmap;

void pagereg_write(d_word word) {
	if (oldmap == word) return;
	oldmap = word;
	pagemap[1] = ram[(word >> 12) & 7];
	pagemap[2] = ram[(word >> 8) & 7];
	switch (word & 033) {
	case 000:
		pdp_ram_map |= 0x00ff0000;
		pdp_mem_map |= 0x00ff0000;
		break;
	case 001:
		pagemap[2] = rom[0];
		pdp_ram_map &= 0xff00ffff;
		pdp_mem_map |= 0x00ff0000;
		break;
	case 002:
		pagemap[2] = rom[1];
		pdp_ram_map &= 0xff00ffff;
		pdp_mem_map |= 0x00ff0000;
		break;
	case 010:
		pagemap[2] = rom[2];
		pdp_ram_map &= 0xff00ffff;
		pdp_mem_map &= 0xff00ffff;
		break;
	case 020:
		pagemap[2] = rom[3];
		pdp_ram_map &= 0xff00ffff;
		pdp_mem_map &= 0xff00ffff;
		break;
	default: fprintf(stderr, "Bad ROM map %o\n", word & 033);
	}

	video_map[1] = video_map[2] = 0;
	if (pagemap[1] == ram[1]) video_map[1] = 1;
	else if (pagemap[1] == ram[7]) video_map[1] = 2;
	if (pagemap[2] == ram[1]) video_map[2] = 1;
	else if (pagemap[2] == ram[7]) video_map[2] = 2;

	// fprintf(stderr, "Pagemap = %06o\n", word);
}

void pagereg_bwrite(d_byte byte) {
	if (oldmap >> 8 == byte) return;
	oldmap = (oldmap & 0xff) | (byte << 8);
	pagemap[1] = ram[(byte >> 4) & 7];
	pagemap[2] = ram[byte & 7];

	video_map[1] = video_map[2] = 0;
	if (pagemap[1] == ram[1]) video_map[1] = 1;
	else if (pagemap[1] == ram[7]) video_map[1] = 2;
	if (pagemap[2] == ram[1]) video_map[2] = 1;
	else if (pagemap[2] == ram[7]) video_map[2] = 2;

	// fprintf(stderr, "Pagemap = %06o\n", byte);
}

/*
 * sc_word() - Store a word at the given core address.
 */

CPU_RES sc_word (c_addr addr, d_word word) {

	addr &= ~1;

	if (IS_RAM_ADDRESS(addr)) {
		if (VIDEO_PAGE(addr) && mem(addr) != word) {
			scr_write(VIDEO_PAGE(addr)-1, addr & 037777, word);
		}
		umr[addr] = 1;
		mem(addr) = word;
		return CPU_OK;
	}

	pdp_qmap *q_ptr;

	for (q_ptr = q_head; q_ptr; q_ptr = q_ptr->next)
		if ((addr >= q_ptr->start) &&
		 	  (addr < (q_ptr->start + (q_ptr->size << 1))))
				return (q_ptr->dev_write) (addr, word);

	fprintf(stderr, _("@%06o Illegal write address %06o:"), pdp.regs[PC], addr);
	return CPU_BUS_ERROR;
}

/*
 * ll_byte() - Load a byte from the given logical address.
 * The PDP can't really do byte reads, so do a word read and
 * get the proper piece.
 */

CPU_RES ll_byte(register pdp_regs *p, d_word baddr, d_byte *byte) {
	d_word word;
	d_word laddr;

	CPU_RES result;

	/*
	 * Get the word address.
	 */

	laddr = baddr & 0177776;

	/*
	 * Address translation.
	 */

	if ((result = ll_word(p, laddr, &word)) != CPU_OK)
		return result;

	if ( baddr & 1 ) {
		word = (word >> 8) & 0377;
	} else {
		word = word & 0377;
	}

	*byte = word;

	return CPU_OK;
}	// ll_byte

/*
 * sl_byte() - Store a byte at the given logical address.
 */

/* (device default "bwrite") */
static CPU_RES do_bwrite (pdp_qmap *q_ptr, d_word addr, d_byte byte) {
	d_word is_odd = addr & 1;
	d_word word;
	addr &= ~1;

	q_ptr->dev_read (addr, &word);

	if (!is_odd)
		word = (word & 0177400) | byte;
	else
		word = (byte << 8) | (word & 0377);

	return q_ptr->dev_write(addr, word);
}	// do_bwrite

CPU_RES sl_byte(register pdp_regs *p, d_word laddr, d_byte byte) {
	d_word t;

	if ( IS_RAM_ADDRESS(laddr)) {
		t = mem(laddr);
		if (laddr & 1) {
			t = (t & 0x00ff) | (byte << 8);
		} else {
			t = (t & 0xff00) | byte;
		}
		if (VIDEO_PAGE(laddr) && mem(laddr) != t) {
			scr_write(VIDEO_PAGE(laddr)-1, laddr & 037776, t);
		}
		mem(laddr) = t;
		return CPU_OK;
	}

	pdp_qmap *q_ptr;

	for (q_ptr = q_head; q_ptr; q_ptr = q_ptr->next)
		if ((laddr >= q_ptr->start) &&
		 	  (laddr < (q_ptr->start + (q_ptr->size << 1)))) {
		 	  if (q_ptr->dev_bwrite)
					return (q_ptr->dev_bwrite) (laddr, byte);
				else
					return do_bwrite (q_ptr, laddr, byte);
		}

	fprintf(stderr, _("Illegal byte write address %06o:"), laddr);
	return CPU_BUS_ERROR;
}	// sl_byte

/*
 * mem_init() - Initialize the memory.
 */

void mem_init() {
	int x;
	if (terak) {
			/* TODO:

			device_install ()...
	    qmap = qmap_terak;
			*/
	    pdp_mem_map = 0x0fffffff;
	    pdp_ram_map = 0x0fffffff;
	} else if (bkmodel == 0) {
	    pdp_mem_map = 0x7fffffff;
	    pdp_ram_map = 0x0000ffff;

			device_install (&q_secret);

			/* disk ports can only be available if there is no BASIC */
			device_install (&q_disk);

			device_install (&q_basicmem);

			/* Line registers are available even if BASIC is mapped */
			device_install (&q_line);
			device_install (&q_timer);
			device_install (&q_io);
			device_install (&q_tty);
			device_install (&q_port);

	    if (!rompath12 || !*rompath12) {
		/*
		 * This is BK-0010 with a disk controller and
		 * 16Kb of RAM @ 120000-157777.
		 * Correct the ROM entry in the bus map.
		 */

		q_basicmem.start = 0160000;
		q_basicmem.size = 2048; /* words */

		pdp_mem_map = 0x3fffffff;
		/* Blocks at 0120000 and 0140000 are RAM */
		pdp_ram_map |= 0x0ff00000;
	  }
	} else if (bkmodel == 1) {
		pdp_mem_map = 0x3fffffff;
		pdp_ram_map = 0x0000ffff;

		device_install (&q_secret);
		device_install (&q_disk);
		device_install (&q_basicmem);
		device_install (&q_line);
		device_install (&q_timer);
		device_install (&q_io);
		device_install (&q_tty);
		device_install (&q_port);

		/* Turn BK-0010 BASIC off */

		q_basicmem.start = 0160000;
		q_basicmem.size = 0; /* words */

		if (!diskrom || !*diskrom) {
		/*
		 * This is BK-0011M without a disk controller.
		 */
			pdp_mem_map = 0x0fffffff;
		}
	}

	for ( x = 0; x < PDP_FULL_MEM_SIZE; ++x ) {
		if (IS_RAM_ADDRESS(2*x))
			mem(x) = (d_word) 0xff00;
	}
}

/*
 * q_reset() - Reset the UNIBUS devices.
 */
void
q_reset()
{
	pdp_qmap *q_ptr;

	for (q_ptr = q_head; q_ptr; q_ptr = q_ptr->next) {
		(q_ptr->dev_init)();
	}
}

