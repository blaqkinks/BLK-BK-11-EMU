
#include "defines.h"
#include <libintl.h>
#include <stdio.h>

extern FILE * tape_read_file, * tape_write_file;

static enum { Idle, Addr, Len, Name, Data, Checksum } fake_state = Idle;

extern char bk_filename[17];
extern char unix_filename[17];

extern flag_t hflag;

void fake_reset() {
fake_state = Idle;
}	// fake_reset

/* Sets word 0314 to the strobe length (say, 10) */
void fake_tuneup_sequence() {
	sc_word(0314, 10);
	rts_PC (&pdp);
}

/* Nothing to tune up, skip to reading without tune-up */
void fake_array_with_tuneup() {
	pdp.regs[PC] = 0117336;
}

/* To return an error on a non-existent file, we return addr 0,
 * length 1, and the actual contents of byte @ 0, but the checksum
 * is deliberately incorrect.
 */
void fake_read_strobe() {
	static unsigned bytenum;
	static unsigned checksum;
	static unsigned char curbyte, bmask;
	static unsigned curlen;
	static unsigned curaddr;

	int bit;

	if (fake_state == Idle) {
		/* First time here, find which file to open */
		get_emt36_filename();
		tape_read_file = fopen(unix_filename, "rb");
		logF(log_tape, "Will read unix file <%s> under BK name <%s>\n",
			unix_filename, bk_filename);

		if (tape_read_file == NULL)
			fprintf (stderr, "File not found!\n");

		fake_state = Addr;
		bytenum = 0;
		bmask = 0;
		checksum = 0;
		curlen = curaddr = 0;
		}

	if (! bmask) {
			if (! hflag) {

			if (fake_state == Name /* && !hflag */)
				curbyte = bk_filename[bytenum];
			else if (fake_state == Checksum /* TMP && !hflag */) {
				curbyte = bytenum ? (checksum >> 8) : (checksum & 0xFF);
				}
			else curbyte = tape_read_file ? fgetc(tape_read_file) : 0;
			}

			else curbyte = tape_read_file ? fgetc(tape_read_file) : 0;

			bmask = 1;

			switch (fake_state) {
			case Addr:
				switch (bytenum ++) {
					case 0:		curaddr = curbyte;
										break;
					case 1:		curaddr |= curbyte << 8;
										logF(log_tape, "File address == 0%o\n", curaddr);
										fake_state = Len;
										bytenum = 0;
										break;
					}
		    break;

			case Len:
				switch (bytenum ++) {
					case 0:		curlen = curbyte;
										break;
					case 1:		curlen |= curbyte << 8;
										logF(log_tape, "File length == 0%o\n", curlen);
										fake_state = Name;
										bytenum = 0;
										break;
					}
		    break;

			case Name:
				/* printf ("[%c] ", curbyte); */

				if (++ bytenum == 16) {
					fake_state = Data;
					// TMP
					/* fprintf (stderr, "starting Data... (%o, %o)\n", curaddr, curlen); */
					bytenum = 0;
					}
				break;

			case Data:
				checksum += curbyte;
				if (checksum & 0200000) {
					checksum = (checksum & 0xFFFF) + 1;
					}

				if (++ bytenum == curlen) {
				logF(log_tape, "File checksum == 0%06o\n", checksum);
				bytenum = 0;
				fake_state = Checksum;
					// TMP
					/* fprintf (stderr, "starting CS...\n"); */
				}

				if (! (bytenum % 16)) printf ("%u ", bytenum);
				break;

			case Checksum:
				if (bytenum ++ == 2) {
					if (tape_read_file) fclose(tape_read_file);
					tape_read_file = 0;
					fake_state = Idle;
					}
				break;
			default:;
		}	// switch
	}	// (! bmask)

	bit = (curbyte & bmask);
	bmask <<= 1;

  pdp.regs[4] = bit ? 15 : 5;
	rts_PC(&pdp);
}

void fake_write_file() {
	c_addr base;
	lc_word(0306, &base);
	get_emt36_filename();
	tape_write_file = fopen(unix_filename, "wb");
	fprintf(stderr, "Will%swrite BK file <%s> under unix file name <%s>\n",
		tape_write_file ? " " : " NOT ", bk_filename, unix_filename);
	if (tape_write_file) {
		d_word addr, length;
		lc_word(base + 2, &addr);
		fputc(addr & 0xff, tape_write_file);
		fputc(addr >> 8, tape_write_file);
		lc_word(base + 4, &length);
		fputc(length & 0xff, tape_write_file);
		fputc(length >> 8, tape_write_file);

		// TODO: if hflag, write file header
		for (; length; addr++, length--) {
			d_word w;
			lc_word(addr, &w);
			if (addr & 1) w >>= 8;
			fputc(w & 0xff, tape_write_file);
		}
		fclose(tape_write_file);
		tape_write_file = 0;
		sl_byte(&pdp, base+1, 0);
	} else {
		perror(unix_filename);
		sl_byte(&pdp, base+1, 1);
	}

	rts_PC(&pdp);
}

