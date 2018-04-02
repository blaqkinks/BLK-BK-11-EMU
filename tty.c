/*
 * Originally
 * Copyright 1994, Eric A. Edwards
 * After very heavy modifications
 * Copyright 1995-2003 Allynd Dudnikov (blaqk)

 * Permission to use, copy, modify, and distribute this
 * software and its documentation for any purpose and without
 * fee is hereby granted, provided that the above copyright
 * notice appear in all copies.  Allynd Dudnikov (blaqk) makes no
 * representations about the suitability of this software
 * for any purpose.  It is provided "as is" without expressed
 * or implied warranty.
 */

/* 
 * tty.c - BK-0010/11M registers 177660-177664.
 */

#include "defines.h"
#include "kbd.h"

#include <ctype.h>
#include <libintl.h>
#define _(String) gettext (String)

#define TTY_REG         0177660
#define TTY_SIZE        3

#define TTY_VECTOR      060     /* standard vector for console */
#define TTY_VECTOR2     0274    /* AR2 (ALT) vector for console */

/*
 * Defines for the registers.
 */

#define TTY_IE          0100
#define TTY_DONE        0200

d_word tty_reg;
d_word tty_data;
d_word tty_scroll = 1330;

flag_t key_pressed = false;

flag_t timer_intr_enabled = 0;
int shifted[256];

static int tty_pending_int = 0;
unsigned long pending_interrupts;

void tty_open() {

		kbd_init ();

    int i;
    /* initialize the keytables */
    for (i = 0; i < 256; i++) {
			shifted[i] = i;
    }
    for (i = 'A'; i <= 'Z'; i++) {
			shifted[i] = i ^ 040;
			shifted[i ^ 040] = i;
    }
    shifted['1'] = '!';
    shifted['2'] = '@';
    shifted['3'] = '#';
    shifted['4'] = '$';
    shifted['5'] = '%';
    shifted['6'] = '^';
    shifted['7'] = '&';
    shifted['8'] = '*';
    shifted['9'] = '(';
    shifted['0'] = ')';
    shifted['-'] = '_';
    shifted['='] = '+';
    shifted['\\'] = '|';
    shifted['['] = '{';
    shifted[']'] = '}';
    shifted[';'] = ':';
    shifted['\''] = '"';
    shifted['`'] = '~';
    shifted[','] = '<';
    shifted['.'] = '>';
    shifted['/'] = '?';
}

/*
 * tty_init() - Initialize the BK-0010 keyboard
 */

static void tty_init()
{
	unsigned short old_scroll = tty_scroll;
	tty_reg = 0;
	tty_data = 0;
	tty_pending_int = 0;
	tty_scroll = 01330;
	timer_intr_enabled = 0;
	if (old_scroll != tty_scroll) {
		scr_dirty = 1;
	}
}

/*
 * tty_read() - Handle the reading of a "keyboard" register.
 */

static CPU_RES tty_read(c_addr addr, d_word *word ) {
	d_word offset = addr & 07;

	switch( offset ) {
	case 0:
		*word = tty_reg;
		break;
	case 2:
		*word = tty_data;
		tty_reg &= ~TTY_DONE;
		break;
	case 4:
		*word = tty_scroll;
		break;
	}
	return CPU_OK;
}	// tty_read

/*
 * tty_write() - Handle a write to one of the "keyboard" registers.
 */

static CPU_RES tty_write(c_addr addr, d_word word) {
	d_word offset = addr & 07;
	d_word old_scroll;

	switch( offset ) {
	case 0:
		/* only let them set IE */
		tty_reg = (tty_reg & ~TTY_IE) | (word & TTY_IE);
		break;
	case 2:
		if (bkmodel) {
			flag_t old_timer_enb = timer_intr_enabled;
			scr_param_change((word >> 8) & 0xF, word >> 15);
			timer_intr_enabled = (word & 040000) == 0;
			if (timer_intr_enabled != old_timer_enb) {
				fprintf(stderr, _("Timer %s\n"), timer_intr_enabled ? _("ON") : _("OFF"));
			}
			if (!timer_intr_enabled)
				pending_interrupts &= ~(1<<TIMER_PRI);
		} else {
			fprintf(stderr, _("Writing to kbd data register, "));
			return CPU_BUS_ERROR;
		}
		break;
	case 4:
		old_scroll = tty_scroll;
		tty_scroll = word & 01377;
		if (old_scroll != tty_scroll) {
			scr_dirty = 1;
		}
		break;
	}
	return CPU_OK;
}	// tty_write

/*
 * tty_bwrite() - Handle a byte write.
 */

static CPU_RES tty_bwrite(c_addr addr, d_byte byte) {
	d_word offset = addr & 07;
	d_word old_scroll;

	switch( offset ) {
	case 0:
		/* only let them set IE */
		tty_reg = (tty_reg & ~TTY_IE) | (byte & TTY_IE);
		break;
	case 1:
		break;
	case 2:
		fprintf(stderr, _("Writing to kbd data register, "));
		return CPU_BUS_ERROR;
	case 3:
		if (bkmodel) {
			flag_t old_timer_enb = timer_intr_enabled;
			scr_param_change(byte & 0xF, byte >> 7);
			timer_intr_enabled = (byte & 0100) == 0;
			if (timer_intr_enabled != old_timer_enb) {
				fprintf(stderr, "Timer %s\n", timer_intr_enabled ? "ON" : "OFF");
			}
			if (!timer_intr_enabled)
				pending_interrupts &= ~(1<<TIMER_PRI);
		} else {
			fprintf(stderr, _("Writing to kbd data register, "));
			return CPU_BUS_ERROR;
		}
		break;
	case 4:
		old_scroll = tty_scroll;
		tty_scroll = (tty_scroll & 0xFF00) | (byte & 0377);
		if (old_scroll != tty_scroll) {
			scr_dirty = 1;
		}
		break;
	case 5:
		old_scroll = tty_scroll;
		tty_scroll = ((byte << 8) & 2) | (tty_scroll & 0377);
		if (old_scroll != tty_scroll) {
			scr_dirty = 1;
		}
		break;
	}
	return CPU_OK;
}	// tty_bwrite

/*
 * tty_finish()
 */

void tty_finish( unsigned char c ) {
	cpu_service(( c & 0200 ) ? TTY_VECTOR2 : TTY_VECTOR);
	tty_pending_int = 0;
}

void stop_key() {
    io_stop_happened = 4;
    cpu_service(04);
}

static int ar2 = 0;

void tty_keyevent(unsigned keysym, int pressed, unsigned keymod) {
	int c;
	if(! pressed) {
	    key_pressed = false;
	    if (keysym == KEY_AR2) ar2 = 0;
	    return;
			}

	/* modifier keys handling */
	if (!isascii (keysym)) {
	    switch (keysym) {
	    case KEY_STOP:
				stop_key();     /* STOP is NMI */
				return;

	    case KEY_NOTHING:
				return;

	    case KEY_AR2:
				ar2 = 1;
				return;

	    case KEY_RESET:
				if (ar2) {
					lc_word(0177716, &pdp.regs[PC]);
					pdp.regs[PC] &= 0177400;
			   	}
				return;

	    case KEY_SWITCH:
				scr_switch(0, 0);
				return;

	    default:
		c = keysym;
	    }
	} else {
	    // Keysym follows ASCII
	    c = keysym;
	    if ((keymod & KS_CAPS) && isalpha(c)) {
		c &= ~040;  /* make it capital */
	    }
	    if (keymod & KS_SHIFT) {
		c = shifted[c];
	    }
	    if ((keymod & KS_CTRL) && (c & 0100)) {
		c &= 037;
	    }
	}

	// Win is AP2
	if (ar2) {
	    c |= 0200;
	}
	tty_reg |= TTY_DONE;
	tty_data = c & 0177;
	key_pressed = true;
	if (!tty_pending_int && !(tty_reg & TTY_IE)) {
		ev_register(TTY_PRI, tty_finish, (unsigned long) 0, c);
		tty_pending_int = c & 0200 ? TTY_VECTOR2 : TTY_VECTOR;
	}
}

pdp_qmap q_tty = {
	"tty", "Keyboard",
	TTY_REG, TTY_SIZE, tty_init, tty_read, tty_write, tty_bwrite
	};
