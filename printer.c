/*
 *
 *	"printer.c":
 *	BK printer interface.
 *
 *	ByAllynd Dudnikov (blaqk).
 *
 */

#include "defines.h"
#include <fcntl.h>
#include <stdio.h>

#ifndef	__MINGW32__

#include <sys/ioctl.h>

#endif

#include <libintl.h>
#define _(String) gettext (String)

FILE * io_printer_file = NULL;
#define STROBE 0400

static void printer_init() {
	if (NULL == io_printer_file && printer_file) {
	    io_printer_file = fopen(printer_file, "w");
	    if (NULL == io_printer_file) {
		perror(printer_file);
	    }
	}
}	// printer_init

static CPU_RES printer_read(c_addr addr, d_word *word) {
	static d_word ready = 0;
	if (io_printer_file) {
		*word = ready ^= STROBE;   /* flip-flop */
	} else {
		*word = 0;	/* pulldown */
	}
	return CPU_OK;
}	// printer_read

static CPU_RES printer_write(c_addr addr, d_word word) {
	/* To be exact, posedge strobe must be checked,
	 * but there is no use writing a new byte without
	 * bringing the strobe down first; so we do it level-based.
	 */
	if (io_printer_file && (word & STROBE)) {
		fputc(word & 0xFF, io_printer_file);
	}
	return CPU_OK;
}	// printer_write

pdp_qmap q_printer = {
	"printer", "Printer",
	PORT_REG, PORT_SIZE,
	printer_init, printer_read, printer_write, 0
	};

