
#include "defines.h"

static void tcons_init () {}

static CPU_RES tcons_read(c_addr a, d_word *d) {
	switch (a & 077) {
	case 064:
		*d = 0200;
		break; // done
	case 066:
		fprintf(stderr, "Reading %06o\n", a);
		*d = 0;
		break; // nothing
	}
	return CPU_OK;
}	// tcons_read

static CPU_RES tcons_write(c_addr a, d_word d) {
	switch (a & 077) {
	case 064:
		fprintf(stderr, "Writing %06o: %06o\n", a, d);
		break;
	case 066:
		// fprintf(stderr, "Writing %03o to console port %06o\n", d, a);
		if (d != '\n' && d < 32 || d >= 127)
		fprintf(stderr, "<%o>", d);
		else
		fprintf(stderr, "%c", d);
		break;
	}
	return CPU_OK;
}	// tcons_write

static CPU_RES tcons_bwrite(c_addr a, d_byte d) {
tcons_write (a, d);
}	// tcons_bwrite

pdp_qmap q_tcons_0 = {
	"tcons_0", "Terak console (0)",
	0177564, 4, tcons_init, tcons_read, tcons_write, tcons_bwrite 
};

pdp_qmap q_tcons_1 = {
	"tcons_1", "Terak console (1)",
	0177764, 4, tcons_init, tcons_read, tcons_write, tcons_bwrite 
};

