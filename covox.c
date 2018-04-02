
#include "defines.h"
#include <fcntl.h>
#include <stdio.h>

#include <libintl.h>
#define _(String) gettext (String)

static void covox_init() {
// TODO: init covox sound
}	// covox_init

static CPU_RES covox_read(c_addr addr, d_word *word) {
	*word = 0;	/* pulldown */
	return CPU_OK;
}	// covox_read

static CPU_RES covox_write(c_addr addr, d_word word) {
	covox_sample (word & 0xFF);
	return CPU_OK;
}	// covox_write

pdp_qmap q_covox = {
	"covox", "COVOX audio",
	PORT_REG, PORT_SIZE, covox_init, covox_read, covox_write, 0
};

