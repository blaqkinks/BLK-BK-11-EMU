
#include "defines.h"

#define LOGARITHMIC

d_word mouse_button_state;
unsigned short mouse_up, mouse_right, mouse_down,
		 mouse_left, mouse_but0, mouse_strobe;

int relx, rely;

static void mouse_init() {
	switch (mouseflag) {
	case 1: /* mouse in lower byte */
		mouse_up = 1;
		mouse_right = 2;
		mouse_down = 4;
		mouse_left = 8;
		mouse_but0 = 16;
		mouse_strobe = 8;
		break;
	case 2: /* mouse in upper byte, OS-BK11 */
		mouse_up = 0x100;
		mouse_right = 0x200;
		mouse_down = 0x400;
		mouse_left = 0x800;
		mouse_but0 = 0x1000;
		mouse_strobe = 0x8000;
		break;
	}
}	// mouse_init

void mouse_buttonevent (int pressed, int button) {
if (pressed)
	mouse_button_state |= mouse_but0 << button;
else
	mouse_button_state &= ~(mouse_but0 << button);
}	// mouse_buttonevent

void mouse_moveevent (int xmove, int ymove) {
relx += xmove;
rely += ymove;
}	// mouse_moveevent

static CPU_RES mouse_read(c_addr addr, d_word *word) {
	*word = mouse_button_state;
	if (relx) {
		*word |= relx > 0 ? mouse_right : mouse_left;
	}
	if (rely) {
		*word |= rely > 0 ? mouse_down : mouse_up;
	}
	/* fprintf(stderr, "Mouse %03o\n", *word); */
	return CPU_OK;
}	// mouse_read

static CPU_RES mouse_write(c_addr addr, d_word word) {
	if (word & mouse_strobe)
		return CPU_OK;
	if (relx) {
#ifdef LOGARITHMIC
		relx = relx > 0 ? relx/2 : -(-relx/2);
#else
		relx = relx > 0 ? relx-1 : relx+1;
#endif
	}
	if (rely) {
#ifdef LOGARITHMIC
		rely = rely > 0 ? rely/2 : -(-rely/2);
#else
		rely = rely > 0 ? rely-1 : rely+1;
#endif
	}
	return CPU_OK;
}	// mouse_write

pdp_qmap q_mouse = {
	"mouse", "Mouse",
	PORT_REG, PORT_SIZE, mouse_init, mouse_read, mouse_write, 0
	};

