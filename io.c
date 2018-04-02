#include "defines.h"
#include <fcntl.h>
#include <stdio.h>

#ifndef	__MINGW32__

#include <sys/ioctl.h>
#include <linux/soundcard.h>

#endif

/*
 *	I/O Port
 */

#define IO_REG          0177716         /* tape, speaker, memory */
#define IO_SIZE         1

extern flag_t nflag, fullspeed;
static unsigned io_sound_val = 0;
flag_t io_stop_happened = 0;
flag_t telegraph_enabled = 0; 	/* Default */

// In 'tty.c':
extern flag_t key_pressed;

// In 'sound.c':
bool spk_pulse ();

static void io_init() {
	tape_init();
}	// io_init

static CPU_RES io_read(c_addr addr, d_word *word) {
	int tape_bit;
	tape_bit = tape_read() << 5;

	/* the high byte is 0200 for BK-0010
	 * and 0300 for BK-0011
	 */
	*word = 0100000 | (bkmodel << 14) |
		(telegraph_enabled ? serial_read() : 0200) |
		(key_pressed ? 0 : 0100) |
		tape_bit |
		io_stop_happened;
	io_stop_happened = 0;
	return CPU_OK;
}	// io_read

#if 0

static CPU_RES io_write_common (d_word word) {
	unsigned oldval = io_sound_val;
	io_sound_val = word & 0300;
	if (io_sound_val != oldval)
		spk_pulse ();

	/* status, value */
	tape_write((word >> 7) & 1, (word >> 6) & 1);

	if (telegraph_enabled)
		serial_write(word);

	return CPU_OK;
}	// io_write_common

/* Include tape drive relay into sound as well */
static CPU_RES io_write(c_addr addr, d_word word) {
	if (bkmodel && word & 04000) {
		pagereg_write(word);
		return CPU_OK;
		}

	return io_write_common(word);
}	// io_write

static CPU_RES io_bwrite(c_addr addr, d_byte byte) {
	d_word offset = addr - IO_REG;

	if (offset && bkmodel && byte & 010) {
		pagereg_bwrite(byte);
		return CPU_OK;
		}

	return io_write_common(byte);
}	// io_bwrite

#endif

/* Include tape drive relay into sound as well */
CPU_RES io_write(c_addr addr, d_word word)
{
	unsigned oldval = io_sound_val;
	if (bkmodel && word & 04000) {
		pagereg_write(word);
		return CPU_OK;
	}
	io_sound_val = word & 0300;
	if (io_sound_val != oldval) {
			spk_pulse();
			}
	/* status, value */
	tape_write((word >> 7) & 1, (word >> 6) & 1);
	if (telegraph_enabled) {
		serial_write(word);
	}
	return CPU_OK;
}

CPU_RES io_bwrite(c_addr addr, d_byte byte) {
	d_word offset = addr - IO_REG;
	unsigned oldval = io_sound_val;
	if (offset == 0) {
	    io_sound_val = byte & 0300;
	    if (io_sound_val != oldval) {
			spk_pulse();
	    }
	    tape_write((byte >> 7) & 1, (byte >> 6) & 1);
	    if (telegraph_enabled) {
		    serial_write(byte);
	    }
	} else if (bkmodel && byte & 010) {
		pagereg_bwrite(byte);
	}
	return CPU_OK;
}

pdp_qmap q_io =  {
	"ioport", "I/O port",
	IO_REG, IO_SIZE, io_init, io_read, io_write, io_bwrite
	};

/*
 *	Line interface
 */

#define LINE_REG        0176560
#define LINE_SIZE       4

#define LINE_RST 0176560
#define LINE_RDT 0176562
#define LINE_WST 0176564
#define LINE_WDT 0176566

FILE * irpslog = 0;
enum { IdleL, NameL, HeaderL, BodyL, TailL } lstate = 0;
unsigned char rdbuf = 0;

static void line_init() {
	irpslog = fopen("irps.log", "w");
}	// line_init

static CPU_RES line_read(c_addr addr, d_word *word) {
	switch (addr) {
	// Always ready
	case LINE_RST:
	case LINE_WST:
		*word = 0200;
		break;
	case LINE_WDT:
		*word = 0;
		break;
	case LINE_RDT:
		*word = rdbuf;
		// rdbuf = next byte
		break;
	}
	return CPU_OK;
}	// line_read

int subcnt;
unsigned char fname[11];
unsigned short file_addr, file_len;

static CPU_RES line_bwrite(c_addr addr, d_byte byte) {
	fputc(byte, irpslog);
	switch (lstate) {
	case IdleL:
		switch (byte) {
		case 0: // stop
			logF(log_line, "Stop requested\n");
			break;
		case 1: // start
			logF(log_line, "Start requested\n");
			rdbuf = 1;
			break;
		case 2: // write
			logF(log_line, "File write requested\n");
			rdbuf = 2;
			lstate = NameL;
			subcnt = 0;
			break;
		case 3: // read
			logF(log_line, "File read requested\n");
			rdbuf = 3;
			break;
		case 4: // fake read
			logF(log_line, "Fake read requested\n");
			rdbuf = 4;
			break;
		default:
			logF(log_line, "Unknown op %#o\n", byte);
			rdbuf = 0377;
			break;
		}
		break;
	case NameL:
		fname[subcnt++] = byte;
		rdbuf = 0;
		if (subcnt == 10) {
			logF(log_line, " file name %s\n", fname);
			lstate = HeaderL;
			subcnt = 0;
		}
		break;
	case HeaderL:
		logF(log_line, "Got %#o\n", byte);
		switch (subcnt) {
		case 0:
			file_addr = byte;
			break;
		case 1:
			file_addr |= byte << 8;
			break;
		case 2:
			file_len = byte;
			break;
		case 3:
			file_len |= byte << 8;
			break;
		}
		if (++subcnt == 4) {
			logF(log_line, " file addr %#o, len %#o\n", file_addr, file_len);
			lstate = BodyL;
			subcnt = 0;
		}
		break;
	case BodyL:
		if (++subcnt == file_len) {
			lstate = IdleL;
			subcnt = 0;
			logF(log_line, "Finished\n");
		}
	case TailL:;
	}
	return CPU_OK;
}

static CPU_RES line_write(c_addr addr, d_word word) {
	switch (addr) {
	case LINE_WDT:
		return line_bwrite(addr, word);
	case LINE_RDT:
		// no effect
		break;
	case LINE_RST: case LINE_WST:
		// no effect yet
		break;
	}
	return CPU_OK;
}	// line_write

pdp_qmap q_line = {
	"line", "Line interface",
	LINE_REG, LINE_SIZE,
	line_init, line_read, line_write, line_bwrite
	};

/*
 * Secret register
 */

#define SECRET_REG	0177700
#define SECRET_SIZE	3

static void secret_init() {}

static CPU_RES secret_read(c_addr addr, d_word *word) {
        d_word offset = addr - SECRET_REG;
        switch(offset) {
        case 0: /* 177700 */
					logF(log_secret, "Reading 0177700\n");
          *word = 0177400;
          break;
        case 2: /* 177702 */
					logF(log_secret, "Reading 0177702\n");
          *word = 0177777;
          break;
        case 4: /* 177704 */
					logF(log_secret, "Reading 0177704\n");
					*word = 0;
					break;
        }
        return CPU_OK;
}

static CPU_RES secret_write(c_addr a, d_word d) {
	logF(log_secret, "Writing %o to %o\n", d, a);
	return CPU_OK;	/* goes nowhere */
}

static CPU_RES secret_bwrite(c_addr a, d_byte d) {
	logF(log_secret, "Writing %o to %o\n", d, a);
	return CPU_OK;	/* goes nowhere */
}

pdp_qmap q_secret = {
	"secret", "Secret register",
	SECRET_REG, SECRET_SIZE,
	secret_init, secret_read, secret_write, secret_bwrite
	};

