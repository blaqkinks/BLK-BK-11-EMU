#include "defines.h"
#include <fcntl.h>
#include <stdio.h>

#ifndef __MINGW32__

#include <sys/ioctl.h>

#endif

#include <libintl.h>
#define _(String) gettext (String)

unsigned char tape_read_val = 1, tape_write_val = 0;
FILE * tape_read_file = NULL;
FILE * tape_write_file = NULL;
unsigned char tape_status = 1; /* 0 = tape moving, 1 = tape stopped */

unsigned char fake_tape = 1;	/* Default */

extern int tapeflag;
double tape_read_ticks, tape_write_ticks;
void tape_read_start(), tape_read_finish();

#define TAPE_RELAY_DELAY	10000

void tape_init() {
	if (tape_read_file) {
	    if (fake_tape) {
		fclose(tape_read_file);
		fake_reset();
	    } else {
		pclose(tape_read_file);
	    }
	    tape_read_file = NULL;
	}

	if (fake_tape) {
	    if (tape_write_file) {
		fclose(tape_write_file);
		tape_write_file = 0;
	    }
	} else if (tape_write_file == NULL) {
		tape_write_file = popen("readtape.exe", "w");
		if (tape_write_file) {
			fprintf(stderr, _("readtape open successful\n"));
		} else perror("readtape");
	}
}


void tape_write(unsigned status, unsigned val) {
    if (fake_tape) {
	if (status != tape_status) {
		pdp.clock += TAPE_RELAY_DELAY;
		tape_status = status;
	}
	if (tape_read_file && tape_status) {
		fclose(tape_read_file);
		tape_read_file = 0;
		fake_reset();
	}
	return;
    }
    if (status != tape_status) {
	pdp.clock += TAPE_RELAY_DELAY;
	tape_status = status;
	fprintf(stderr, _("Tape %s\n"), tape_status ? _("OFF") : _("ON"));
	if (!tape_status) {
	    tape_read_ticks = tape_write_ticks = pdp.clock;
	    tape_read_start();
	} else {
	    fflush(tape_write_file);
	    if(tape_read_file) tape_read_finish();
	}
    }
    if (!tape_status && val != tape_write_val) {
    	unsigned delta = pdp.clock - tape_write_ticks;
    	while (delta) {
		unsigned delta2 = (delta <= 32767) ? delta : 32767;

		fputc((delta2 >> 8) | (tape_write_val << 7), tape_write_file);
		fputc(delta2 & 0xFF, tape_write_file);
		delta -= delta2;
    	}
    	tape_write_ticks = pdp.clock;
	tape_write_val = val;	
    }
}

int
tape_read() {
    unsigned delta;
    if (fake_tape) {
	/* Random noise */
	tape_read_val = ((unsigned) (pdp.clock / 1001.0)) & 1;
	return tape_read_val;
    }
    if (tape_status || tape_read_file == NULL) {
	/* some programs want to see tape read bit changing if
	 * the tape motor is OFF. Why - I don't know.
	 */
	return tape_read_val = !tape_read_val;
    }
    while (tape_read_file && pdp.clock > tape_read_ticks) {
	int c1 = fgetc(tape_read_file);
	int c2 = fgetc(tape_read_file);
	if (c2 == EOF) {
		fprintf(stderr, _("End of tape\n"));
		pclose(tape_read_file);
		tape_read_file = 0;
	}
	delta = c1 << 8;
	delta |= c2;
	tape_read_val = delta >> 15;
	delta &= 0x7FFF;
	tape_read_ticks += delta;
    }
    return tape_read_val;
}

char bk_filename[17];
char unix_filename[17];

/* 
 * Returns the raw 16-byte file name in bk_filename,
 * and the name with trimmed trailing spaces in unix_filename.
 */
void
get_emt36_filename() {
	int i;
	d_word base;
	lc_word(0306, &base);
	for (i = 0; i < 8; i++) {
		d_word d;
		lc_word(base + 6 + 2*i, &d);
		bk_filename[2*i] = d & 0xff;
		bk_filename[2*i+1] = d >> 8;
	}
	bk_filename[16] = '\0';
	for (i = 15; i >= 0 && bk_filename[i] == ' '; unix_filename[i--]='\0');
	for (; i >= 0; i--) unix_filename[i] = bk_filename[i];
}

/*
 * When download is requested, attempt to find a file with
 * name specified in the tape drive control block and to
 * open it using maketape.
 */
void
tape_read_start() {
	static char buf[80];
	if (tapeflag) {
	/* attempts to manipulate the tape drive relay will be ignored */
		return;
	}
	get_emt36_filename();	
	sprintf(buf, "maketape.exe '%s' '%s'", bk_filename, unix_filename);
#ifdef VERBOSE_TAPE
	fprintf(stderr, _("Calling (%s)\n"), buf);
#endif
	tape_read_file = popen(buf, "r");
	if (tape_read_file) {
		tape_read_ticks = pdp.clock;
	} else perror(unix_filename);
}

void
tape_read_finish() {
	if (!tape_read_file) return;
	pclose(tape_read_file);
	tape_read_file = 0;
#ifdef VERBOSE_TAPE
	fprintf(stderr, _("Closed maketape\n"));
#endif
}

